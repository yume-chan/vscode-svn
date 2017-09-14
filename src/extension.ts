import * as fs from "fs-extra";
import * as path from "path";

import * as vscode from "vscode";
import { CancellationToken, ProviderResult, Uri } from "vscode";

import { Client, SvnError, SvnStatus, SvnStatusResult } from "../svn";
import { SvnTextDocumentContentProvider } from "./svn-text-document-content-provider";

import { client, status, TaskCanceledError } from "./client";
import { SvnResourceState, SvnSourceControl } from "./svn-source-control";
import { WorkspaceState } from "./workspace-state";

export function activate(context: vscode.ExtensionContext) {
    console.log("SVN activated");

    const base = vscode.workspace.rootPath;
    if (base === undefined)
        return;

    let initialized = false;

    let contentProvider: SvnTextDocumentContentProvider;

    let state: WorkspaceState;
    let sourceControl: SvnSourceControl;

    function initialize() {
        if (initialized)
            return;

        initialized = true;

        state = new WorkspaceState(context.workspaceState);

        contentProvider = new SvnTextDocumentContentProvider();
        context.subscriptions.push(contentProvider);

        sourceControl = new SvnSourceControl(state);
        context.subscriptions.push(sourceControl);

        vscode.commands.executeCommand("setContext", "svnState", "running");

        const watcher = vscode.workspace.createFileSystemWatcher("**");
        context.subscriptions.push(watcher);

        context.subscriptions.push(watcher.onDidChange(onWorkspaceChange));
        context.subscriptions.push(watcher.onDidCreate(onWorkspaceChange));
        context.subscriptions.push(watcher.onDidDelete(onWorkspaceChange));

        context.subscriptions.push(vscode.commands.registerCommand("svn.commit", async () => {
            const message = vscode.scm.inputBox.value;
            if (message === undefined || message === "")
                return;

            if (sourceControl.stagedFiles.size === 0)
                return;

            vscode.window.withProgress({ location: vscode.ProgressLocation.SourceControl, title: "SVN Committing..." }, async (progress) => {
                try {
                    await client.commit(Array.from(sourceControl.stagedFiles), message);

                    state.unstage.clear();
                    await state.save();

                    contentProvider.onCommit(sourceControl.stagedFiles);
                } catch (err) {
                    if (err instanceof SvnError)
                        vscode.window.showErrorMessage(`Commit failed: E${err.code}: ${err.message}`);
                    else
                        vscode.window.showErrorMessage(`Commit failed: ${err.message}`);
                } finally {
                    vscode.scm.inputBox.value = "";
                    onWorkspaceChange();
                }
            });
        }));

        context.subscriptions.push(vscode.commands.registerCommand("svn.stage", async (...resourceStates: SvnResourceState[]) => {
            for (const value of resourceStates) {
                const filePath = value.path;
                if (!value.versioned) {
                    await client.add(filePath);
                    continue;
                }

                switch (value.textStatus) {
                    case Client.StatusKind.modified:
                    case Client.StatusKind.obstructed:
                        state.unstage.delete(filePath);
                        break;
                }
            }

            await state.save();
            onWorkspaceChange();
        }));

        context.subscriptions.push(vscode.commands.registerCommand("svn.unstage", async (...resourceStates: SvnResourceState[]) => {
            for (const value of resourceStates) {
                const filePath = value.path;
                switch (value.nodeStatus) {
                    case Client.StatusKind.added:
                        await client.revert(filePath);
                        break;
                    case Client.StatusKind.modified:
                    case Client.StatusKind.obstructed:
                        state.unstage.add(filePath);
                        break;
                }
            }

            await state.save();
            onWorkspaceChange();
        }));
    }

    async function onWorkspaceChange(file?: vscode.Uri) {
        if (file !== undefined && file.fsPath.includes(".svn"))
            return;

        try {
            const result = await status(base!);
            initialize();
            sourceControl.update(result);
        } catch (err) {
            if (err instanceof TaskCanceledError)
                return;

            vscode.commands.executeCommand("setContext", "svnState", "idle");
            return;
        }
    }
    onWorkspaceChange();
}
