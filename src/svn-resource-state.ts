import * as path from "path";

import {
    Command,
    SourceControlResourceDecorations,
    SourceControlResourceState,
    Uri,
} from "vscode";

import { NodeKind, NodeStatus, StatusKind } from "node-svn";

import { SvnSourceControl } from "./svn-source-control";

export class SvnResourceState implements SourceControlResourceState, NodeStatus {
    public readonly resourceUri: Uri;

    public path: string;
    public kind: NodeKind;
    public node_status: StatusKind;
    public text_status: StatusKind;
    public prop_status: StatusKind;
    public versioned: boolean;
    public changelist?: string | undefined;

    get command(): Command {
        if (this.text_status === StatusKind.modified) {
            const filename = path.basename(this.path);
            return { command: "vscode.diff", title: "Diff", arguments: [this.resourceUri.with({ scheme: "svn" }), this.resourceUri, filename] };
        } else {
            return { command: "vscode.open", title: "Open", arguments: [this.resourceUri] };
        }
    }

    get decorations(): SourceControlResourceDecorations {
        const strikeThrough = this.node_status === StatusKind.deleted;

        return { strikeThrough };
    }

    public constructor(public readonly control: SvnSourceControl, status: Readonly<NodeStatus>) {
        this.resourceUri = Uri.file(status.path);

        for (const key in status)
            this[key] = status[key];
    }
}
