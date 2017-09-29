import { ExtensionContext } from "vscode";

import { client } from "./client";
import { commandCenter } from "./command-center";
import { svnTextDocumentContentProvider } from "./content-provider";
import { SvnResourceState, SvnSourceControl } from "./svn-source-control";
import { workspaceManager } from "./workspace-manager";

export function activate(context: ExtensionContext) {
    console.log(`Svn activated, process id: ${process.pid}`);

    context.subscriptions.push(commandCenter);
    context.subscriptions.push(svnTextDocumentContentProvider);
    context.subscriptions.push(workspaceManager);
}
