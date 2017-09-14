import * as fs from "fs-extra";
import * as path from "path";

import { CancellationToken, commands, Disposable, ExtensionContext, ProviderResult, Uri, workspace, WorkspaceFoldersChangeEvent } from "vscode";

import { Client, SvnError, SvnStatus, SvnStatusResult } from "../svn";
import { svnTextDocumentContentProvider } from "./svn-text-document-content-provider";

import { client } from "./client";
import { SvnResourceState, SvnSourceControl } from "./svn-source-control";

const controls: Set<SvnSourceControl> = new Set();

export function activate(context: ExtensionContext) {
    console.log("SVN activated");

    context.subscriptions.push(new Disposable(() => {
        for (const item of controls)
            item.dispose();
        controls.clear();
    }));

    async function onDidChangeWorkspaceFolders(e: WorkspaceFoldersChangeEvent) {
        for (const item of e.added) {
            const control = await SvnSourceControl.detect(item.uri.fsPath);
            if (control !== undefined)
                controls.add(control);
        }

        for (const item of e.removed) {
            for (const control of controls) {
                if (control.root === item.uri.fsPath)
                    controls.delete(control);
                break;
            }
        }
    }

    context.subscriptions.push(workspace.onDidChangeWorkspaceFolders(onDidChangeWorkspaceFolders));

    onDidChangeWorkspaceFolders({ added: workspace.workspaceFolders || [], removed: [] });

    context.subscriptions.push(svnTextDocumentContentProvider);

    context.subscriptions.push(commands.registerCommand("svn.commit", client.commit, client));

    context.subscriptions.push(commands.registerCommand("svn.stage", async (...resourceStates: SvnResourceState[]) => {
        for (const value of resourceStates) {
            const filePath = value.path;

            if (!value.versioned)
                await client.add(filePath);
            else
                await client.changelistRemove(filePath);
        }
    }));

    context.subscriptions.push(commands.registerCommand("svn.unstage", async (...resourceStates: SvnResourceState[]) => {
        for (const value of resourceStates) {
            const filePath = value.path;
            switch (value.nodeStatus) {
                case Client.StatusKind.added:
                    await client.revert(filePath);
                    break;
                default:
                    await client.changelistAdd(filePath, "ignore-on-commit");
                    break;
            }
        }
    }));
}
