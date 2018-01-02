import * as path from "path";

import {
    Command,
    DecorationData,
    SourceControlResourceDecorations,
    SourceControlResourceState,
    ThemeColor,
    Uri,
} from "vscode";

import { NodeStatus, RevisionKind, StatusKind } from "node-svn";

import { SvnSourceControl } from "./svn-source-control";
import { SvnUri } from "./svn-uri";

export class SvnResourceState implements SourceControlResourceState {
    public readonly resourceUri: Uri;

    get command(): Command {
        if (this.status.text_status === StatusKind.modified) {
            const filename = path.basename(this.status.path);
            return { command: "vscode.diff", title: "Diff", arguments: [new SvnUri(this.resourceUri, RevisionKind.base).toUri(), this.resourceUri, filename] };
        } else {
            return { command: "vscode.open", title: "Open", arguments: [this.resourceUri] };
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
