import * as path from "path";

import {
    CancellationToken,
    Command,
    Disposable,
    ProgressLocation,
    ProviderResult,
    QuickDiffProvider,
    scm,
    SourceControl,
    SourceControlResourceGroup,
    SourceControlResourceState,
    Uri,
    window,
    workspace,
} from "vscode";

import { StatusKind, NodeStatus } from "node-svn";

import { client } from "./client";
import { svnTextDocumentContentProvider } from "./content-provider";
import { Throttler } from "./throttler";

const iconsRootPath = path.join(__dirname, "..", "resources", "icons");

const statusIcons = {
    [StatusKind.modified]: "modified",
    [StatusKind.unversioned]: "untracked",
    [StatusKind.added]: "added",
    [StatusKind.obstructed]: "conflict",
    [StatusKind.missing]: "deleted",
    [StatusKind.deleted]: "deleted",
};

export interface SvnResourceState extends NodeStatus, SourceControlResourceState {
    control: SvnSourceControl;
}

export class SvnSourceControl implements QuickDiffProvider {
    public readonly workspaces: Set<string> = new Set<string>();

    public stagedFiles: Set<string> = new Set<string>();

    private sourceControl: SourceControl;

    private staged: SourceControlResourceGroup;
    private changes: SourceControlResourceGroup;
    private ignored: SourceControlResourceGroup;

    private refreshThrottler: Throttler;

    private readonly disposable: Set<Disposable> = new Set();

    public constructor(public root: string) {
        this.refreshThrottler = new Throttler(this._refresh.bind(this), 500);

        this.sourceControl = scm.createSourceControl("svn", `${path.basename(root)} (Svn)`);
        this.sourceControl.acceptInputCommand = { command: "svn.commit", title: "Commit", arguments: [this.sourceControl] };
        this.sourceControl.quickDiffProvider = this;
        this.sourceControl.statusBarCommands = [
            {
                arguments: [this.sourceControl],
                command: "svn.update",
                title: "$(sync) Update",
                tooltip: "Update",
            },
        ];
        this.disposable.add(this.sourceControl);

        this.staged = this.sourceControl.createResourceGroup("staged", "Staged Changes");
        this.staged.hideWhenEmpty = true;
        this.disposable.add(this.staged);

        this.changes = this.sourceControl.createResourceGroup("changes", "Changes");
        this.disposable.add(this.changes);

        this.ignored = this.sourceControl.createResourceGroup("ignore", "Ignored");
        this.ignored.hideWhenEmpty = true;
        this.disposable.add(this.ignored);

        const watcher = workspace.createFileSystemWatcher("**");
        this.disposable.add(watcher);

        this.disposable.add(watcher.onDidChange(this.onDidChange, this));
        this.disposable.add(watcher.onDidCreate(this.onDidChange, this));
        this.disposable.add(watcher.onDidDelete(this.onDidChange, this));
    }

    public provideOriginalResource?(uri: Uri, token: CancellationToken): ProviderResult<Uri> {
        return uri.with({ scheme: "svn" });
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }

    public async update() {
        try {
            this.sourceControl.statusBarCommands = [
                {
                    arguments: [this.sourceControl],
                    command: "",
                    title: "$(sync~spin) Updating",
                    tooltip: "Updating",
                },
            ];

            await client.update(this.root);

            this.sourceControl.statusBarCommands = [
                {
                    arguments: [this.sourceControl],
                    command: "svn.update",
                    title: "$(sync) Update",
                    tooltip: "Update",
                },
            ];

            await this.refresh();
        } catch (err) {
            return;
        }
    }

    public refresh(): Promise<void> {
        return this.refreshThrottler.run();
    }

    public async commit(message?: string) {
        message = message || this.sourceControl.inputBox.value;
        if (message === undefined || message === "") {
            window.showErrorMessage("Please input commit message.");
            return;
        }

        if (this.stagedFiles.size === 0)
            return;

        window.withProgress({ location: ProgressLocation.SourceControl, title: "SVN Committing..." }, async (progress) => {
            try {
                await client.commit(Array.from(this.stagedFiles), message!, (info) => { });
                svnTextDocumentContentProvider.onCommit(this.stagedFiles);
            } catch (err) {
                // if (err instanceof SvnError)
                //     window.showErrorMessage(`Commit failed: E${err.code}: ${err.message}`);
                // else
                window.showErrorMessage(`Commit failed: ${err.message}`);
            } finally {
                this.sourceControl.inputBox.value = "";
                this.refresh();
            }
        });
    }

    private onDidChange(e: Uri) {
        if (e.path.includes("/.svn/"))
            return;

        this.refresh();
    }

    private async _refresh() {
        try {
            this.stagedFiles.clear();

            const stagedStates: SvnResourceState[] = [];
            const changedStates: SvnResourceState[] = [];
            const ignoredStates: SvnResourceState[] = [];

            await client.status(this.root, (path, info) => {
                if (info.changelist === "ignore-on-submit") {
                    ignoredStates.push(this.getResourceState(info));
                } else {
                    switch (info.node_status) {
                        case StatusKind.added:
                        case StatusKind.modified:
                        case StatusKind.obstructed:
                        case StatusKind.deleted:
                            this.stagedFiles.add(path);
                            stagedStates.push(this.getResourceState(info));
                            break;
                        default:
                            changedStates.push(this.getResourceState(info));
                            break;
                    }
                }
            });

            this.staged.resourceStates = stagedStates;
            this.changes.resourceStates = changedStates;
            this.ignored.resourceStates = ignoredStates;
        } catch (err) {
            return;
        }
    }

    private getResourceState(state: NodeStatus): SvnResourceState {
        const resourceUri = Uri.file(state.path);
        const filename: string = path.basename(state.path);
        const icon = state.versioned ? statusIcons[state.node_status] : statusIcons[StatusKind.unversioned];
        const command: Command = state.text_status === StatusKind.modified ?
            { command: "vscode.diff", title: "Diff", arguments: [resourceUri.with({ scheme: "svn" }), resourceUri, filename] } :
            { command: "vscode.open", title: "Open", arguments: [resourceUri] };
        return {
            control: this,
            ...state,
            resourceUri,
            command,
            decorations: {
                dark: { iconPath: Uri.file(path.resolve(iconsRootPath, "dark", `status-${icon}.svg`)) },
                light: { iconPath: Uri.file(path.resolve(iconsRootPath, "light", `status-${icon}.svg`)) },
                tooltip: `Node: ${StatusKind[state.node_status]}\nText: ${StatusKind[state.text_status]}\nProp: ${StatusKind[state.prop_status]}`,
            },
        };
    }
}
