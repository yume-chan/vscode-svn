import * as path from "path";

import {
    Command,
    DecorationData,
    SourceControlResourceDecorations,
    SourceControlResourceState,
    SourceControlResourceThemableDecorations,
    ThemeColor,
    Uri,
} from "vscode";

import { Depth, NodeKind, NodeStatus, StatusKind } from "node-svn";

import { SvnSourceControl } from "./svn-source-control";

const iconsRootPath = path.join(path.dirname(__dirname), "resources", "icons");

function getIconUri(iconName: string, theme: string): Uri {
    return Uri.file(path.join(iconsRootPath, theme, `${iconName}.svg`));
}

const status_to_icon_name = {
    [StatusKind.modified]: "status-modified",
    [StatusKind.added]: "status-added",
    [StatusKind.deleted]: "status-deleted",
    [StatusKind.replaced]: "status-renamed",
    [StatusKind.unversioned]: "status-untracked",
    [StatusKind.ignored]: "status-ignored",
    [StatusKind.conflicted]: "status-conflict",
};

const icons = { light: {}, dark: {} };
for (const key in status_to_icon_name) {
    icons.light[key] = getIconUri(status_to_icon_name[key], "light");
    icons.dark[key] = getIconUri(status_to_icon_name[key], "dark");
}

export class SvnResourceState implements SourceControlResourceState, NodeStatus {
    public readonly resourceUri: Uri;

    public path: string;
    public changed_author: string;
    public changed_date: number | string;
    public changed_rev: number;
    public conflicted: boolean;
    public copied: boolean;
    public depth: Depth;
    public kind: NodeKind;
    public node_status: StatusKind;
    public prop_status: StatusKind;
    public revision: number;
    public text_status: StatusKind;
    public versioned: boolean;
    public changelist?: string;

    get command(): Command {
        if (this.text_status === StatusKind.modified) {
            const filename = path.basename(this.path);
            return { command: "vscode.diff", title: "Diff", arguments: [this.resourceUri.with({ scheme: "svn" }), this.resourceUri, filename] };
        } else {
            return { command: "vscode.open", title: "Open", arguments: [this.resourceUri] };
        }
    }

    get decorations(): SourceControlResourceDecorations & DecorationData {
        const abbreviation = StatusKind[this.node_status][0].toUpperCase();
        const bubble = true;
        const color = new ThemeColor("gitDecoration.modifiedResourceForeground");
        const dark: SourceControlResourceThemableDecorations = {
            iconPath: icons.dark[this.node_status],
        };
        const letter = abbreviation;
        const light: SourceControlResourceThemableDecorations = {
            iconPath: icons.light[this.node_status],
        };
        const priority = 1;
        const strikeThrough = this.node_status === StatusKind.deleted;
        const tooltip = `\nNode: ${StatusKind[this.node_status]}\nText: ${StatusKind[this.text_status]}\nProps: ${StatusKind[this.prop_status]}`;

        return { abbreviation, bubble, color, dark, letter, light, priority, strikeThrough, tooltip };
    }

    public constructor(public readonly control: SvnSourceControl, status: Readonly<NodeStatus>) {
        this.resourceUri = Uri.file(status.path);

        for (const key in status)
            this[key] = status[key];
    }
}
