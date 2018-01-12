import {
    CancellationToken,
    Disposable,
    ProgressLocation,
    ProviderResult,
    QuickDiffProvider,
    scm,
    SourceControl,
    SourceControlResourceGroup,
    Uri,
    window,
    workspace,
} from "vscode";

import { RevisionKind, StatusKind } from "node-svn";

import { client } from "./client";
import svnContentProvider from "./content-provider";
import svnDecorationProvider from "./decoration-provider";
import { showErrorMessage, writeError, writeTrace } from "./output";
import { SvnResourceState } from "./resource-state";
import { SvnUri } from "./svn-uri";
import { Throttler } from "./throttler";

export class SvnSourceControl implements QuickDiffProvider {
    public static readonly cache: Map<string, SvnResourceState> = new Map<string, SvnResourceState>();

    private sourceControl: SourceControl;

    private staged: SourceControlResourceGroup;
    private changes: SourceControlResourceGroup;
    private ignored: SourceControlResourceGroup;
    private conflicted: SourceControlResourceGroup;

    private refreshThrottler: Throttler;

    private stagedFiles: Set<string> = new Set<string>();

    private readonly disposable: Set<Disposable> = new Set();

    public readonly workspaces: Set<string> = new Set<string>();

    public constructor(public root: string) {
        this.refreshThrottler = new Throttler(this._refresh.bind(this), 500);

        this.sourceControl = scm.createSourceControl("svn", "Svn", Uri.file(root));
        this.sourceControl.acceptInputCommand = { command: "svn.commit", title: "Commit", arguments: [this.sourceControl] };
        this.sourceControl.quickDiffProvider = this;
        this.setUpdating(false);
        this.disposable.add(this.sourceControl);

        this.staged = this.sourceControl.createResourceGroup("staged", "Staged Changes");
        this.staged.hideWhenEmpty = true;
        this.disposable.add(this.staged);

        this.changes = this.sourceControl.createResourceGroup("changes", "Changes");
        this.disposable.add(this.changes);

        this.ignored = this.sourceControl.createResourceGroup("ignored", "Ignored");
        this.ignored.hideWhenEmpty = true;
        this.disposable.add(this.ignored);

        this.conflicted = this.sourceControl.createResourceGroup("conflicted", "Conficted");
        this.conflicted.hideWhenEmpty = true;
        this.disposable.add(this.conflicted);

        const watcher = workspace.createFileSystemWatcher("**");
        this.disposable.add(watcher);

        for (const event of [watcher.onDidChange, watcher.onDidCreate, watcher.onDidDelete])
            this.disposable.add(event(this.onDidChange, this));
    }

    private onDidChange(e: Uri) {
        if (e.path.includes("/.svn/"))
            return;

        this.refresh();
    }

    private setUpdating(value: boolean): void {
        if (value) {
            this.sourceControl.statusBarCommands = [
                {
                    arguments: [this.sourceControl],
                    command: "",
                    title: "$(sync~spin) Updating",
                    tooltip: "Updating",
                },
            ];
        } else {
            this.sourceControl.statusBarCommands = [
                {
                    arguments: [this.sourceControl],
                    command: "svn.update",
                    title: "$(sync) Update",
                    tooltip: "Update",
                },
            ];
        }
    }

    private _refresh() {
        return window.withProgress({ location: ProgressLocation.SourceControl, title: "Updating..." }, async () => {
            try {
                this.stagedFiles.clear();

                const stagedStates: SvnResourceState[] = [];
                const changedStates: SvnResourceState[] = [];
                const ignoredStates: SvnResourceState[] = [];

                const files: Uri[] = [];

                await client.status(this.root, (info) => {
                    const uri = Uri.file(info.path);
                    files.push(uri);

                    const state = new SvnResourceState(this, info);
                    SvnSourceControl.cache.set(uri.fsPath, state);

                    if (state.status.node_status === StatusKind.external ||
                        (state.status.node_status === StatusKind.normal && state.status.file_external))
                        return;

                    if (state.status.changelist === "ignore-on-commit") {
                        ignoredStates.push(state);
                    } else {
                        switch (state.status.node_status) {
                            case StatusKind.added:
                            case StatusKind.modified:
                            case StatusKind.obstructed:
                            case StatusKind.deleted:
                                this.stagedFiles.add(state.status.path);
                                stagedStates.push(state);
                                break;
                            default:
                                changedStates.push(state);
                                break;
                        }
                    }
                });

                this.staged.resourceStates = stagedStates;
                this.changes.resourceStates = changedStates;
                this.ignored.resourceStates = ignoredStates;

                svnDecorationProvider.onDidChangeFiles(files);
            } catch (err) {
                writeError(`refresh("${this.root}")`, err);
            }
        });
    }

    public provideOriginalResource(uri: Uri, token: CancellationToken): ProviderResult<Uri> {
        return new SvnUri(uri, RevisionKind.base).toTextUri();
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }

    public async update() {
        try {
            this.setUpdating(true);

            const revision = await client.update(this.root);
            writeTrace(`update("${this.root}") `, revision);

            await this.refresh();
        } catch (err) {
            writeError(`update("${this.root}") `, err);
            showErrorMessage("Update");
        } finally {
            this.setUpdating(false);
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

        if (this.stagedFiles.size === 0) {
            window.showErrorMessage(`There is nothing to commit. (Did you forget to stage changes ?) `);
            return;
        }

        window.withProgress({ location: ProgressLocation.SourceControl, title: "SVN Committing..." }, async (progress) => {
            try {
                await client.commit(Array.from(this.stagedFiles), message!, (info) => {
                    writeTrace(`commit("${info.repos_root}", "${message}") `, info.revision);
                });
                svnContentProvider.onCommit(this.stagedFiles);
                this.sourceControl.inputBox.value = "";
            } catch (err) {
                writeError(`commit("${this.root}", "${message}") `, err);
                showErrorMessage("Commit");

                // X if (err instanceof SvnError)
                // X     window.showErrorMessage(`Commit failed: E${ err.code }: ${ err.message }`);
                // X else
            } finally {
                this.refresh();
            }
        });
    }
}
