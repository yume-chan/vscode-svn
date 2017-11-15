import { ExtensionContext } from "vscode";

import { commandCenter } from "./command-center";
import { svnTextDocumentContentProvider } from "./content-provider";
import { workspaceManager } from "./workspace-manager";

export function activate(context: ExtensionContext) {
    console.log(`Svn activated, process id: ${process.pid}`);

    context.subscriptions.push(commandCenter);
    context.subscriptions.push(svnTextDocumentContentProvider);
    context.subscriptions.push(workspaceManager);
}
