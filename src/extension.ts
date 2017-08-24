import * as path from "path";
import * as fs from "fs-extra";

import * as vscode from "vscode";

import { SvnClient, SvnError, SvnErrorCode, SvnFileChange, SvnItemState, SvnLockOwner, SvnPropertyChange } from "./svn-client";
import { SvnTextDocumentContentProvider } from "./svn-text-document-content-provider";

type SvnResourceState = SvnItemState & vscode.SourceControlResourceState;

export function activate(context: vscode.ExtensionContext) {
    const base = vscode.workspace.rootPath;
    if (base === undefined)
        return;

    const client = new SvnClient("C:/Users/Simon/Documents/visual studio 2017/Projects/Svn/Svn/bin/Debug/svn.exe");

    const iconsRootPath = path.join(path.dirname(__dirname), "..", 'resources', 'icons');
    const statusIcons = {
        [SvnFileChange.Modified]: "modified",
        [SvnFileChange.Untracked]: "untracked",
        [SvnFileChange.Added]: "added",
        [SvnFileChange.TypeChanged]: "conflict",
    };
    function getResourceState(state: SvnItemState): SvnResourceState {
        const resourceUri = vscode.Uri.file(path.resolve(base, state.relativePath));
        const filename: string = path.basename(state.relativePath);
        const icon = statusIcons[state.fileChange];
        const command: vscode.Command = state.fileChange == SvnFileChange.Modified ?
            { command: "vscode.diff", title: "Diff", arguments: [resourceUri.with({ scheme: "svn" }), resourceUri, filename] } :
            { command: "vscode.open", title: "Open", arguments: [resourceUri] };
        return {
            ...state,
            resourceUri,
            command,
            decorations: {
                light: { iconPath: vscode.Uri.file(path.resolve(iconsRootPath, "light", `status-${icon}.svg`)), },
                dark: { iconPath: vscode.Uri.file(path.resolve(iconsRootPath, "dark", `status-${icon}.svg`)), },
            }
        };
    }

    let initialized = false;

    let contentProvider: SvnTextDocumentContentProvider;

    let stagedStates: SvnResourceState[];
    let changedStates: SvnResourceState[];
    let ignoredStates: SvnResourceState[];

    let staged: vscode.SourceControlResourceGroup;
    let changes: vscode.SourceControlResourceGroup;
    let ignored: vscode.SourceControlResourceGroup;

    let stagedFiles: string[];
    let ignoredFiles: string[];

    async function onWorkspaceChange() {
        let status: SvnItemState[];
        try {
            status = await client.status(base!);
        }
        catch (err) {
            if (err instanceof SvnError) {
                if (err.warning.find(item => item.id === SvnErrorCode.NotWorkingCopy) !== undefined)
                    vscode.commands.executeCommand("setContext", "svnState", "idle");
            }
            return;
        }

        if (!initialized) {
            initialized = true;

            contentProvider = new SvnTextDocumentContentProvider(client);
            context.subscriptions.push(contentProvider);
            context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider("svn", contentProvider));

            context.subscriptions.push(vscode.commands.registerCommand("svn.commit", async function() {
                const message = vscode.scm.inputBox.value;
                if (message === undefined || message === "")
                    return;

                if (stagedFiles.length == 0)
                    return;

                vscode.window.withProgress({ location: vscode.ProgressLocation.SourceControl, title: "SVN Committing..." }, async (progress) => {
                    try {
                        await client.commit({ isFile: false, message }, { isFile: false, files: stagedFiles });
                        ignoredFiles = [];
                        contentProvider.onCommit(stagedStates);
                    } catch (err) {
                        if (err instanceof SvnError) {
                            const first = err.error[0];
                            vscode.window.showErrorMessage(`Commit failed: E${first.id}: ${first.message}`);
                        }
                    }
                    finally {
                        vscode.scm.inputBox.value = "";
                        onWorkspaceChange();
                    }
                });
            }));

            ignoredFiles = context.workspaceState.get<string[]>("svn.unstage") || [];

            context.subscriptions.push(vscode.commands.registerCommand("svn.stage", async function(...resourceStates: SvnResourceState[]) {
                for (const value of resourceStates) {
                    const filePath = value.relativePath;
                    switch (value.fileChange) {
                        case SvnFileChange.Modified:
                        case SvnFileChange.TypeChanged:
                            const index = ignoredFiles.indexOf(filePath);
                            if (index !== -1) {
                                ignoredFiles.splice(index, 1);
                                stagedFiles.push(filePath);
                            }
                            break;
                        case SvnFileChange.Untracked:
                            await client.add(filePath);
                            break;
                    }
                }
                context.workspaceState.update("svn.unstage", ignoredFiles);
                onWorkspaceChange();
            }));
            context.subscriptions.push(vscode.commands.registerCommand("svn.unstage", async function(...resourceStates: SvnResourceState[]) {
                for (const value of resourceStates) {
                    const filePath = value.relativePath;
                    switch (value.fileChange) {
                        case SvnFileChange.Modified:
                        case SvnFileChange.TypeChanged:
                            if (!ignoredFiles.includes(filePath))
                                ignoredFiles.push(filePath);
                            break;
                        case SvnFileChange.Added:
                            await client.revert(filePath);
                            break;
                    }
                }
                context.workspaceState.update("svn.unstage", ignoredFiles);
                onWorkspaceChange();
            }));

            const scm = vscode.scm.createSourceControl("svn", "SVN");
            context.subscriptions.push(scm);
            scm.acceptInputCommand = { command: "svn.commit", title: "Commit" };
            scm.quickDiffProvider = {
                provideOriginalResource(uri: vscode.Uri, token: vscode.CancellationToken): vscode.ProviderResult<vscode.Uri> {
                    return uri.with({ scheme: "svn" });
                },
            };

            staged = scm.createResourceGroup("staged", "Staged Changes");
            staged.hideWhenEmpty = true;
            context.subscriptions.push(staged);

            changes = scm.createResourceGroup("changes", "Changes");
            context.subscriptions.push(changes);

            ignored = scm.createResourceGroup("ignore", "Ignored");
            ignored.hideWhenEmpty = true;
            context.subscriptions.push(ignored);

            vscode.commands.executeCommand("setContext", "svnState", "running");

            const watcher = vscode.workspace.createFileSystemWatcher("**");
            context.subscriptions.push(watcher);

            context.subscriptions.push(watcher.onDidChange(onWorkspaceChange));
            context.subscriptions.push(watcher.onDidCreate(onWorkspaceChange));
            context.subscriptions.push(watcher.onDidDelete(onWorkspaceChange));
        }

        stagedFiles = [];
        stagedStates = [];
        changedStates = [];
        ignoredStates = [];
        for (const state of status) {
            if (ignoredFiles.includes(state.relativePath)) {
                ignoredStates.push(getResourceState(state));
            } else {
                switch (state.fileChange) {
                    case SvnFileChange.Added:
                    case SvnFileChange.Deleted:
                    case SvnFileChange.Modified:
                    case SvnFileChange.TypeChanged:
                        stagedFiles.push(state.relativePath);
                        stagedStates.push(getResourceState(state));
                        break;
                    default:
                        changedStates.push(getResourceState(state));
                        break;
                }
            }
        }
        staged.resourceStates = stagedStates;
        changes.resourceStates = changedStates;
        ignored.resourceStates = ignoredStates;
    }
    onWorkspaceChange();
}

export function deactivate() {
}
