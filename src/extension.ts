import { CancellationToken, commands, Disposable, ExtensionContext, ProviderResult, Uri, workspace, WorkspaceFoldersChangeEvent } from "vscode";

import { Client } from "../svn";
import { svnTextDocumentContentProvider } from "./svn-text-document-content-provider";

import { client } from "./client";
import { commandCenter } from "./command-center";
import { SvnResourceState, SvnSourceControl } from "./svn-source-control";

const controls: Set<SvnSourceControl> = new Set();

export function activate(context: ExtensionContext) {
    console.log(`Svn activated, process id: ${process.pid}`);

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
    context.subscriptions.push(commandCenter);
}
