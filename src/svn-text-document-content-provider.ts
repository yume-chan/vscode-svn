import * as vscode from "vscode";
import { CancellationToken, Disposable, EventEmitter, TextDocumentContentProvider, Uri } from "vscode";

import { SvnError } from "../svn";

import { client } from "./client";

export class SvnTextDocumentContentProvider implements TextDocumentContentProvider {
    private onDidChangeEvent: EventEmitter<Uri> = new EventEmitter<Uri>();

    private disposable: Disposable;

    get onDidChange() { return this.onDidChangeEvent.event; }

    public constructor() {
        const registry = vscode.workspace.registerTextDocumentContentProvider("svn", this);

        this.disposable = Disposable.from(this.onDidChangeEvent, registry);
    }

    public onCommit(states: Iterable<string>) {
        for (const value of states)
            this.onDidChangeEvent.fire(Uri.file(value).with({ scheme: "svn" }));
    }

    public async provideTextDocumentContent(uri: Uri, token: CancellationToken): Promise<string> {
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

    public dispose(): void {
        this.disposable.dispose();
    }
}
