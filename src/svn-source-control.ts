import * as path from "path";

import * as vscode from "vscode";
import { CancellationToken, Command, Disposable, ProviderResult, SourceControl, SourceControlResourceGroup, SourceControlResourceState, Uri } from "vscode";

import { Client, SvnStatus, SvnStatusResult } from "../svn";

import { WorkspaceState } from "./workspace-state";

const iconsRootPath = path.join(__dirname, "..", "resources", "icons");

const statusIcons = {
    [Client.StatusKind.modified]: "modified",
    [Client.StatusKind.unversioned]: "untracked",
    [Client.StatusKind.added]: "added",
    [Client.StatusKind.obstructed]: "conflict",
};

export type SvnResourceState = SvnStatus & SourceControlResourceState;

export class SvnSourceControl {
    public stagedFiles: Set<string> = new Set<string>();

    private sourceControl: SourceControl;

    private staged: SourceControlResourceGroup;
    private changes: SourceControlResourceGroup;
    private ignored: SourceControlResourceGroup;

    private disposable: Disposable;

    public constructor(private state: WorkspaceState) {
        this.sourceControl = vscode.scm.createSourceControl("svn", "SVN");
        this.sourceControl.acceptInputCommand = { command: "svn.commit", title: "Commit" };
        this.sourceControl.quickDiffProvider = {
            provideOriginalResource(uri: Uri, token: CancellationToken): ProviderResult<Uri> {
                return uri.with({ scheme: "svn" });
            },
        };

        this.staged = this.sourceControl.createResourceGroup("staged", "Staged Changes");
        this.staged.hideWhenEmpty = true;

        this.changes = this.sourceControl.createResourceGroup("changes", "Changes");

        this.ignored = this.sourceControl.createResourceGroup("ignore", "Ignored");
        this.ignored.hideWhenEmpty = true;

        this.disposable = Disposable.from(this.sourceControl, this.staged, this.changes, this.ignored);
    }

    public update(state: SvnStatusResult) {
        const stagedStates = [];
        const changedStates = [];
        const ignoredStates = [];
        for (const item of state) {
            if (this.state.unstage.has(item.path)) {
                ignoredStates.push(this.getResourceState(item));
            } else {
                switch (item.nodeStatus) {
                    case Client.StatusKind.added:
                    case Client.StatusKind.deleted:
                    case Client.StatusKind.modified:
                    case Client.StatusKind.obstructed:
                        this.stagedFiles.add(item.path);
                        stagedStates.push(this.getResourceState(item));
                        break;
                    default:
                        changedStates.push(this.getResourceState(item));
                        break;
                }
            }
        }
        this.staged.resourceStates = stagedStates;
        this.changes.resourceStates = changedStates;
        this.ignored.resourceStates = ignoredStates;
    }

    public dispose(): void {
        this.disposable.dispose();
    }

    private getResourceState(state: SvnStatus): SvnResourceState {
        const resourceUri = Uri.file(state.path);
        const filename: string = path.basename(state.path);
        const icon = statusIcons[state.textStatus];
        const command: Command = state.textStatus === Client.StatusKind.modified ?
            { command: "vscode.diff", title: "Diff", arguments: [resourceUri.with({ scheme: "svn" }), resourceUri, filename] } :
            { command: "vscode.open", title: "Open", arguments: [resourceUri] };
        return {
            ...state,
            resourceUri,
            command,
            decorations: {
                dark: { iconPath: Uri.file(path.resolve(iconsRootPath, "dark", `status-${icon}.svg`)) },
                light: { iconPath: Uri.file(path.resolve(iconsRootPath, "light", `status-${icon}.svg`)) },
            },
        };
    }
}
