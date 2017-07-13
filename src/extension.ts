import { execFile } from "child_process";
import * as path from "path";

import * as vscode from "vscode";

import { SvnClient, SvnError, SvnErrorCode, SvnFileChange, SvnItemState, SvnLockOwner, SvnPropertyChange } from "./svn-client";

type SvnResourceState = SvnItemState & vscode.SourceControlResourceState;

export function activate(context: vscode.ExtensionContext) {
    const base = vscode.workspace.rootPath;
    if (base === undefined)
        return;

    const client = new SvnClient(base, "svn");

    const iconsRootPath = path.join(path.dirname(__dirname), "..", 'resources', 'icons');
    const statusIcons = {
        [SvnFileChange.Modified]: "modified",
        [SvnFileChange.Untracked]: "untracked",
        [SvnFileChange.Added]: "added",
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

    let staged: vscode.SourceControlResourceGroup;
    let changes: vscode.SourceControlResourceGroup;
    let ignored: vscode.SourceControlResourceGroup;

    let stagedFiles: string[];
    let unstagedFiles: string[];

    async function onWorkspaceChange() {
        let status: SvnItemState[];
        try {
            status = await client.status();
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

            context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider("svn", {
                provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): vscode.ProviderResult<string> {
                    return client.cat(uri.fsPath);
                },
            }));

            context.subscriptions.push(vscode.commands.registerCommand("svn.commit", async function () {
                const message = vscode.scm.inputBox.value;
                if (message === undefined || message === "")
                    return;

                if (stagedFiles.length == 0)
                    return;

                vscode.window.withProgress({ location: vscode.ProgressLocation.SourceControl, title: "SVN Committing..." }, (progress) => {
                    return client.commit(message, ...stagedFiles);
                });
            }));

            unstagedFiles = context.workspaceState.get<string[]>("svn.unstage") || [];

            context.subscriptions.push(vscode.commands.registerCommand("svn.stage", async function (...resourceStates: SvnResourceState[]) {
                for (const value of resourceStates) {
                    const filePath = value.relativePath;
                    switch (value.fileChange) {
                        case SvnFileChange.Modified:
                            const index = unstagedFiles.indexOf(filePath);
                            if (index !== -1) {
                                unstagedFiles.splice(index, 1);
                                stagedFiles.push(filePath);
                            }
                            break;
                        case SvnFileChange.Untracked:
                            await client.add(filePath);
                            break;
                    }
                }
                context.workspaceState.update("svn.unstage", unstagedFiles);
                onWorkspaceChange();
            }));
            context.subscriptions.push(vscode.commands.registerCommand("svn.unstage", async function (...resourceStates: SvnResourceState[]) {
                for (const value of resourceStates) {
                    const filePath = value.relativePath;
                    switch (value.fileChange) {
                        case SvnFileChange.Modified:
                            if (!unstagedFiles.includes(filePath))
                                unstagedFiles.push(filePath);
                            break;
                        case SvnFileChange.Added:
                            await client.revert(filePath);
                            break;
                    }
                }
                context.workspaceState.update("svn.unstage", unstagedFiles);
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
            context.subscriptions.push(staged);

            changes = scm.createResourceGroup("changes", "Changes");
            changes.hideWhenEmpty = true;
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
        const stagedStates: SvnResourceState[] = [];
        const changedStates: SvnResourceState[] = [];
        const ignoredStates: SvnResourceState[] = [];
        for (const state of status) {
            if (unstagedFiles.includes(state.relativePath)) {
                ignoredStates.push(getResourceState(state));
            } else if (state.fileChange == SvnFileChange.Added ||
                state.fileChange == SvnFileChange.Deleted ||
                state.fileChange == SvnFileChange.Modified ||
                state.fileChange == SvnFileChange.Replaced) {
                stagedStates.push(getResourceState(state));
                stagedFiles.push(state.relativePath);
            } else {
                changedStates.push(getResourceState(state));
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
