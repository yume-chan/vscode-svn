import {
    Command,
    DecorationData,
    SourceControlResourceDecorations,
    SourceControlResourceState,
    ThemeColor,
    Uri,
} from "vscode";

import { NodeStatus, RevisionKind, StatusKind } from "node-svn";

import { SvnSourceControl } from "./source-control";
import { SvnUri } from "./svn-uri";

export class SvnResourceState implements SourceControlResourceState {
    public readonly resourceUri: Uri;

    get command(): Command {
        const node_status = this.status.node_status;
        switch (node_status) {
            case StatusKind.modified:
                return { command: "svn.openDiff", title: "Open Changes", arguments: [new SvnUri(this.resourceUri, RevisionKind.base)] };
            case StatusKind.missing:
            case StatusKind.deleted:
                return { command: "svn.openFile", title: "Open File", arguments: [new SvnUri(this.resourceUri, RevisionKind.base)] };
            default:
                return { command: "vscode.open", title: "Open File", arguments: [this.resourceUri] };
        }
    }

    get decorations(): SourceControlResourceDecorations & DecorationData {
        const abbreviation = StatusKind[this.status.node_status][0].toUpperCase();
        const bubble = true;
        const color = new ThemeColor("gitDecoration.modifiedResourceForeground");
        const letter = abbreviation;
        const priority = 1;
        const strikeThrough = this.status.node_status === StatusKind.deleted || this.status.node_status === StatusKind.missing;
        const tooltip = `\nNode: ${StatusKind[this.status.node_status]}\nText: ${StatusKind[this.status.text_status]}\nProps: ${StatusKind[this.status.prop_status]}`;

        return { abbreviation, bubble, color, letter, priority, strikeThrough, tooltip };
    }

    public constructor(public readonly control: SvnSourceControl, public readonly status: Readonly<NodeStatus>) {
        this.resourceUri = Uri.file(status.path);
    }
}
