import * as path from "path";

import {
    commands,
    Disposable,
    ProgressLocation,
    SourceControl,
    SourceControlResourceGroup,
    window,
    workspace,
} from "vscode";

import { RevisionKind, StatusKind } from "./node-svn";

import Client from "./client";
import { showErrorMessage, writeError, writeTrace } from "./output";
import { SvnResourceState } from "./resource-state";
import isSamePath from "./same-path";
import subscriptions from "./subscriptions";
import { SvnUri } from "./svn-uri";
import workspaceManager from "./workspace-manager";

async function openUri(uri: SvnUri) {
    await window.withProgress({ location: ProgressLocation.Window, title: "Fetching file..." }, async () => {
        const resource = await uri.toResourceUri();
        await commands.executeCommand("vscode.open", resource);
    });
}

async function iterate<T>(iterable: AsyncIterable<T>): Promise<T | undefined> {
    let value: T | undefined;
    for await (const item of iterable) {
        value = item;
    }
    return value;
}

class CommandCenter {
    private readonly disposable: Set<Disposable> = new Set();

    constructor() {
        this.disposable.add(commands.registerCommand("svn.update", this.update));

        this.disposable.add(commands.registerCommand("svn.commit", this.commit));
        this.disposable.add(commands.registerCommand("svn.refresh", this.refresh));
        this.disposable.add(commands.registerCommand("svn.cleanup", this.cleanup));

        this.disposable.add(commands.registerCommand("svn.resolve", this.resolve));

        this.disposable.add(commands.registerCommand("svn.stage", this.stage));
        this.disposable.add(commands.registerCommand("svn.stageAll", (group: SourceControlResourceGroup) => {
            return this.stage(...group.resourceStates as SvnResourceState[]);
        }));

        this.disposable.add(commands.registerCommand("svn.unstage", this.unstage));
        this.disposable.add(commands.registerCommand("svn.unstageAll", (group: SourceControlResourceGroup) => {
            return this.unstage(...group.resourceStates as SvnResourceState[]);
        }));

        this.disposable.add(commands.registerCommand("svn.openDiff", async (uri: SvnUri) => {
            window.withProgress({ location: ProgressLocation.Window, title: "Fetching file..." }, async () => {
                try {
                    const left = await uri.toResourceUri();
                    const filename = path.basename(uri.file.fsPath);

                    await commands.executeCommand("vscode.diff", left, uri.file, filename);
                } catch (err) {
                    writeError(`openDiff(${uri.file.fsPath})`, err);
                }
            });
        }));

        this.disposable.add(commands.registerCommand("svn.openFile", async (uri: SvnUri | SvnResourceState) => {
            if (uri instanceof SvnResourceState) {
                switch (uri.status.node_status) {
                    case StatusKind.missing:
                    case StatusKind.deleted:
                        try {
                            await openUri(new SvnUri(uri.resourceUri, RevisionKind.base));
                        } catch (err) {
                            writeError(`openFile(${uri.resourceUri.fsPath})`, err);
                        }
                        break;
                    default:
                        await commands.executeCommand("vscode.open", uri.resourceUri);
                        break;
                }
                return;
            }

            openUri(uri);
        }));

        this.disposable.add(commands.registerCommand("svn.checkout", this.checkout));
    }

    private async checkout() {
        const url = await window.showInputBox({
            ignoreFocusOut: true,
            prompt: "Checkout url",
        });

        if (url === undefined)
            return;

        let target = await window.showInputBox({
            ignoreFocusOut: true,
            prompt: "Checkout path",
        });

        if (target === undefined)
            return;

        while (!path.isAbsolute(target)) {
            const folders = workspace.workspaceFolders;
            if (folders === undefined || folders.length !== 1) {
                const item = "Retry";
                const selected = await window.showErrorMessage(`Can't determinate absolute path from relative path ${target}`, item);
                switch (selected) {
                    case item:
                        target = await window.showInputBox({
                            ignoreFocusOut: true,
                            prompt: "Checkout path",
                        });

                        if (target === undefined)
                            return;

                        break;
                    default:
                        return;
                }
            } else {
                target = path.resolve(folders[0].uri.fsPath, target);
            }
        }

        const client = Client.get();
        try {
            const revision = await client.checkout(url, target);
            writeTrace(`checkout("${url}", "${target}")`, revision);
        } catch (err) {
            writeError(`checkout("${url}", "${target}")`, err);
        } finally {
            Client.release(client);
        }
    }

    private async update(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if ((control !== undefined))
            await control.update();
    }

    private async commit(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if (control !== undefined)
            await control.commit();
    }

    private async refresh(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if (control !== undefined)
            await control.refresh();
    }

    private async cleanup(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if (control !== undefined)
            await control.cleanup();
    }

    private async resolve() {
        const editor = window.activeTextEditor;
        if (typeof editor === "undefined") {
            return;
        }

        const client = Client.get();
        const target = editor.document.uri.fsPath;
        try {
            await client.resolve(target);

            const root = await client.get_working_copy_root(target);
            for (const control of workspaceManager.controls) {
                if (isSamePath(control.root, root)) {
                    await control.refresh();
                }
            }
        } catch (err) {
            writeError(`resolve(${target})`, err);
            showErrorMessage(`Stage`);
        } finally {
            Client.release(client);
        }
    }

    private async stage(...resourceStates: SvnResourceState[]) {
        if (resourceStates.length === 0)
            return;

        const control = resourceStates[0].control;

        const client = Client.get();
        try {
            for (const item of resourceStates) {
                if (!item.status.versioned)
                    await client.add(item.status.path);
                else if (item.status.node_status === StatusKind.missing)
                    for await (const info of client.remove(item.status.path)) {
                        console.log(info);
                    }
                else
                    await client.remove_from_changelists(item.status.path);
            }
        } catch (err) {
            writeError(`stage`, err);
            showErrorMessage(`Stage`);
        } finally {
            Client.release(client);

            await control.refresh();
        }
    }

    private async unstage(...resourceStates: SvnResourceState[]) {
        if (resourceStates.length === 0)
            return;

        const control = resourceStates[0].control;

        const client = Client.get();
        try {
            for (const item of resourceStates) {
                switch (item.status.node_status) {
                    case StatusKind.added:
                        await iterate(client.remove(item.status.path, { keep_local: true }));
                        break;
                    case StatusKind.deleted:
                        await client.revert(item.status.path);
                        break;
                    default:
                        await client.add_to_changelist(item.status.path, "ignore-on-commit");
                        break;
                }
            }
        } catch (err) {
            writeError(`unstage`, err);
            showErrorMessage(`Unstage`);
        } finally {
            Client.release(client);

            await control.refresh();
        }
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }
}

export default subscriptions.add(new CommandCenter());
