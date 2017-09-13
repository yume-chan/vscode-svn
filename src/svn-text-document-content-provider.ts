import * as vscode from "vscode";

import { SvnError } from "../svn";

import { client } from "./client";

export class SvnTextDocumentContentProvider implements vscode.TextDocumentContentProvider, vscode.Disposable {
    private onDidChangeEvent: vscode.EventEmitter<vscode.Uri> = new vscode.EventEmitter<vscode.Uri>();

    get onDidChange() { return this.onDidChangeEvent.event; }

    public onCommit(states: vscode.SourceControlResourceState[]) {
        for (const value of states)
            this.onDidChangeEvent.fire(value.resourceUri.with({ scheme: "svn" }));
    }

    public async provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): Promise<string> {
        try {
            const buffer = await client.cat(uri.fsPath);
            const content = buffer.toString("utf8");
            return content;
        } catch (err) {
            if (err instanceof SvnError)
                return "";
            throw err;
        }
    }

    public dispose() {
        this.onDidChangeEvent.dispose();
    }
}
