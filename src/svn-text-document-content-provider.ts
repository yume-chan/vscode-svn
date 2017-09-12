import * as vscode from "vscode";

import { Client, SvnError } from "svn";

export class SvnTextDocumentContentProvider implements vscode.TextDocumentContentProvider, vscode.Disposable {
    private onDidChangeEvent: vscode.EventEmitter<vscode.Uri> = new vscode.EventEmitter<vscode.Uri>();

    constructor(private client: Client) { }

    get onDidChange() { return this.onDidChangeEvent.event; }

    onCommit(states: vscode.SourceControlResourceState[]) {
        for (const value of states)
            this.onDidChangeEvent.fire(value.resourceUri.with({ scheme: "svn" }));
    }

    async provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): Promise<string> {
        try {
            const buffer = await this.client.cat(uri.fsPath);
            const content = buffer.toString("utf8");
            return content;
        } catch (err) {
            if (err instanceof SvnError)
                return "";
            throw err;
        }
    }

    dispose() {
        this.onDidChangeEvent.dispose();
    }
}
