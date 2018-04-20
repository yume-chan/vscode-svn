import * as iconv from "iconv-lite";
import {
    CancellationToken,
    Disposable,
    EventEmitter,
    TextDocumentContentProvider,
    Uri,
    workspace,
} from "vscode";

import { Revision } from "./node-svn";

import { writeError, writeTrace } from "./output";
import subscriptions from "./subscriptions";
import { SvnUri } from "./svn-uri";

class SvnContentProvider implements TextDocumentContentProvider {
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
        let file: Uri;
        let revision: Revision;
        try {
            ({ file, revision } = SvnUri.parse(uri));
        } catch (err) {
            writeError(`provideTextDocumentContent()`, new Error(`Can't parse Uri ${uri.toString()}`));
            return "";
        }

        const fsPath = file.fsPath;
        try {
            const result = await SvnUri.provideResource(uri);

            const config = workspace.getConfiguration("files", file);
            const encoding = config.get<string>("encoding", "utf8");

            const content = iconv.decode(result.content, iconv.encodingExists(encoding) ? encoding : "utf8");
            writeTrace(`provideTextDocumentContent("${fsPath}")`, { text: content.replace(/\r/g, "\\r").replace(/\n/g, "\\n").substring(0, 50), length: content.length });
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

export default subscriptions.add(new SvnContentProvider());
