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

import Client from "./client";
import svnContentProvider from "./content-provider";
import svnDecorationProvider from "./decoration-provider";
import { showErrorMessage, writeError, writeTrace } from "./output";
import { SvnResourceState } from "./resource-state";
import { SvnUri } from "./svn-uri";

export class SvnSourceControl implements QuickDiffProvider {
    public static readonly cache: Map<string, SvnResourceState> = new Map<string, SvnResourceState>();

    private sourceControl: SourceControl;

    private staged: SourceControlResourceGroup;
    private changes: SourceControlResourceGroup;
    private ignored: SourceControlResourceGroup;
    private conflicted: SourceControlResourceGroup;

    private stagedFiles: Set<string> = new Set<string>();

    private readonly disposable: Set<Disposable> = new Set();

    public readonly workspaces: Set<string> = new Set<string>();

    public constructor(public root: string) {
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

    public async cleanup(): Promise<void> {
        const client = Client.get();
        try {
            await client.cleanup(this.root);
            await this.refresh();
            writeTrace(`cleanup(${this.root})`, "ok");
        } catch (err) {
            writeError(`cleanup(${this.root})`, err);
        } finally {
            Client.release(client);
        }
    }

    public refresh() {
        return window.withProgress({ location: ProgressLocation.SourceControl, title: "Refreshing..." }, async () => {
            this.stagedFiles.clear();

            const staged: SvnResourceState[] = [];
            const changed: SvnResourceState[] = [];
            const ignored: SvnResourceState[] = [];
            const conflicted: SvnResourceState[] = [];

            const files: Uri[] = [];

            const client = Client.get();
            try {
                for await (const status of client.status(this.root)) {
                    const uri = Uri.file(status.path);
                    files.push(uri);

                    const state = new SvnResourceState(this, status);
                    SvnSourceControl.cache.set(uri.fsPath, state);

                    // ignore external folders
                    if (status.node_status === StatusKind.external)
                        continue;

                    if (status.changelist === "ignore-on-commit") {
                        ignored.push(state);
                    } else {
                        switch (status.node_status) {
                            case StatusKind.added:
                            case StatusKind.modified:
                            case StatusKind.obstructed:
                            case StatusKind.deleted:
                                this.stagedFiles.add(status.path);
                                staged.push(state);
                                break;
                            case StatusKind.conflicted:
                                conflicted.push(state);
                                break;
                            case StatusKind.normal:
                                // maybe it's totally untouched, but
                                //     a. it's an external file
                                //     b. it belongs to some changelist
                                // there files will also get interest of svn.
                                if (status.prop_status !== StatusKind.normal ||
                                    status.node_status !== StatusKind.normal) {
                                    changed.push(state);
                                }
                                break;
                            default:
                                changed.push(state);
                                break;
                        }
                    }
                }

                this.staged.resourceStates = staged;
                this.changes.resourceStates = changed;
                this.ignored.resourceStates = ignored;
                this.conflicted.resourceStates = conflicted;

                svnDecorationProvider.onDidChangeFiles(files);
            } catch (err) {
                writeError(`refresh("${this.root}")`, err);
            } finally {
                Client.release(client);
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
        const client = Client.get();
        try {
            this.setUpdating(true);

            const revision = await client.update(this.root);
            writeTrace(`update("${this.root}") `, revision);

            await this.refresh();
        } catch (err) {
            writeError(`update("${this.root}") `, err);
            showErrorMessage("Update");
        } finally {
            Client.release(client);

            this.setUpdating(false);
        }
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

        await window.withProgress({ location: ProgressLocation.SourceControl, title: "Committing..." }, async (progress) => {
            const client = Client.get();
            try {
                for await (const info of client.commit(Array.from(this.stagedFiles), message!)) {
                    writeTrace(`commit("${info.repos_root}", "${message}") `, info.revision);
                }

                svnContentProvider.onCommit(this.stagedFiles);
                this.sourceControl.inputBox.value = "";
            } catch (err) {
                writeError(`commit("${this.root}", "${message}") `, err);
                showErrorMessage("Commit");
            } finally {
                Client.release(client);

                await this.refresh();
            }
        });
    }
}
