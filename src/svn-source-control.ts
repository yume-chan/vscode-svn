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

import { Client, SvnError, SvnStatus, SvnStatusResult } from "../svn";

import { client } from "./client";
import { throttle } from "./git/decorators";
import { svnTextDocumentContentProvider } from "./svn-text-document-content-provider";

const iconsRootPath = path.join(__dirname, "..", "resources", "icons");

const statusIcons = {
    [Client.StatusKind.modified]: "modified",
    [Client.StatusKind.unversioned]: "untracked",
    [Client.StatusKind.added]: "added",
    [Client.StatusKind.obstructed]: "conflict",
    [Client.StatusKind.missing]: "deleted",
    [Client.StatusKind.deleted]: "deleted",
};

export interface SvnResourceState extends SvnStatus, SourceControlResourceState {
    control: SvnSourceControl;
}

export class SvnSourceControl implements QuickDiffProvider {
    public static async detect(file: string) {
        try {
            const workingCopy = (await client.info(file, {
                depth: Client.Depth.empty,
            }))[0].workingCopy;

            if (workingCopy === undefined)
                return;

            const result = new SvnSourceControl(workingCopy.rootPath, file);
            await result.refresh();
            return result;
        } catch (err) {
            return undefined;
        }
    }

    public stagedFiles: Set<string> = new Set<string>();

    private sourceControl: SourceControl;

    private staged: SourceControlResourceGroup;
    private changes: SourceControlResourceGroup;
    private ignored: SourceControlResourceGroup;

    private readonly disposable: Set<Disposable> = new Set();

    private constructor(public root: string, public file: string) {
        this.sourceControl = scm.createSourceControl("svn", `${path.basename(root)} (Svn)`);
        this.sourceControl.acceptInputCommand = { command: "svn.commit", title: "Commit", arguments: [this.sourceControl] };
        this.sourceControl.quickDiffProvider = this;
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

        this.disposable.add(watcher.onDidChange(this.refresh, this));
        this.disposable.add(watcher.onDidCreate(this.refresh, this));
        this.disposable.add(watcher.onDidDelete(this.refresh, this));
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
            await client.update(this.root);
            await this.refresh();
        } catch (err) {
            return;
        }
    }

    @throttle
    public async refresh(e?: Uri) {
        if (e !== undefined && e.fsPath.includes("/.svn/"))
            return;

        try {
            const states = await client.status(this.root);
            const ignored = (await client.status(this.root, { changelists: "ignore-on-commit" })).map((x) => x.path);

            this.stagedFiles.clear();

            const stagedStates: SvnResourceState[] = [];
            const changedStates: SvnResourceState[] = [];
            const ignoredStates: SvnResourceState[] = [];
            for (const item of states) {
                if (ignored.includes(item.path)) {
                    ignoredStates.push(this.getResourceState(item));
                } else {
                    switch (item.nodeStatus) {
                        case Client.StatusKind.added:
                        case Client.StatusKind.modified:
                        case Client.StatusKind.obstructed:
                        case Client.StatusKind.deleted:
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
        } catch (err) {
            return;
        }
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
                await client.commit(Array.from(this.stagedFiles), message!);
                svnTextDocumentContentProvider.onCommit(this.stagedFiles);
            } catch (err) {
                if (err instanceof SvnError)
                    window.showErrorMessage(`Commit failed: E${err.code}: ${err.message}`);
                else
                    window.showErrorMessage(`Commit failed: ${err.message}`);
            } finally {
                this.sourceControl.inputBox.value = "";
                this.refresh();
            }
        });
    }

    private getResourceState(state: SvnStatus): SvnResourceState {
        const resourceUri = Uri.file(state.path);
        const filename: string = path.basename(state.path);
        const icon = state.versioned ? statusIcons[state.nodeStatus] : statusIcons[Client.StatusKind.unversioned];
        const command: Command = state.textStatus === Client.StatusKind.modified ?
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
                tooltip: `Node: ${Client.StatusKind[state.nodeStatus]}\nText: ${Client.StatusKind[state.textStatus]}\nProp: ${Client.StatusKind[state.propStatus]}`,
            },
        };
    }
}
