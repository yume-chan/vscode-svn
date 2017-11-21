import {
    CancellationToken,
    Disposable,
    EventEmitter,
    TextDocumentContentProvider,
    Uri,
    workspace,
} from "vscode";

import { CatOptions, RevisionKind } from "node-svn";

import { client } from "./client";

class SvnTextDocumentContentProvider implements TextDocumentContentProvider {
    private readonly _onDidChange: EventEmitter<Uri> = new EventEmitter<Uri>();

    private readonly disposable: Set<Disposable> = new Set();

    public readonly onDidChange = this._onDidChange.event;

    public constructor() {
        this.disposable.add(this._onDidChange);
        this.disposable.add(workspace.registerTextDocumentContentProvider("svn", this));
    }

    public onCommit(states: Iterable<string>) {
        for (const value of states)
            this._onDidChange.fire(Uri.file(value).with({ scheme: "svn" }));
    }

    public async provideTextDocumentContent(uri: Uri, token: CancellationToken): Promise<string> {
        try {
            const options: CatOptions = { peg_revision: RevisionKind.base, revision: RevisionKind.base };
            const buffer = await client.cat(uri.fsPath, options);
            const content = buffer.toString("utf8");
            return content;
        } catch (err) {
            return "";
        }
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }
}

export const svnTextDocumentContentProvider = new SvnTextDocumentContentProvider();
