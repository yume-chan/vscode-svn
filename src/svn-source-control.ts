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
};

export type SvnResourceState = SvnStatus & SourceControlResourceState;

export class SvnSourceControl implements QuickDiffProvider {
    public static async detect(folder: string) {
        try {
            const result = await client.status(folder, {
                depth: Client.Depth.empty,
                getAll: true,
            });
            const item = result[0];

            let root = path.normalize(item.path);
            root = root.substring(0, root.length - path.normalize(item.relativePath).length);
            return new SvnSourceControl(root);
        } catch (err) {
            return undefined;
        }
    }

    public stagedFiles: Set<string> = new Set<string>();

    private sourceControl: SourceControl;

    private staged: SourceControlResourceGroup;
    private changes: SourceControlResourceGroup;
    private ignored: SourceControlResourceGroup;

    private disposable: Disposable[] = [];

    private constructor(public root: string) {
        this.sourceControl = scm.createSourceControl("svn", `${path.basename(root)} (Svn)`);
        this.sourceControl.acceptInputCommand = { command: "svn.commit", title: "Commit" };
        this.sourceControl.quickDiffProvider = this;
        this.disposable.push(this.sourceControl);

        this.staged = this.sourceControl.createResourceGroup("staged", "Staged Changes");
        this.staged.hideWhenEmpty = true;
        this.disposable.push(this.staged);

        this.changes = this.sourceControl.createResourceGroup("changes", "Changes");
        this.disposable.push(this.changes);

        this.ignored = this.sourceControl.createResourceGroup("ignore", "Ignored");
        this.ignored.hideWhenEmpty = true;
        this.disposable.push(this.ignored);

        const watcher = workspace.createFileSystemWatcher("**");
        this.disposable.push(watcher);

        this.disposable.push(watcher.onDidChange(this.refresh, this));
        this.disposable.push(watcher.onDidCreate(this.refresh, this));
        this.disposable.push(watcher.onDidDelete(this.refresh, this));
    }

    public provideOriginalResource?(uri: Uri, token: CancellationToken): ProviderResult<Uri> {
        return uri.with({ scheme: "svn" });
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();
    }

    @throttle
    private async refresh() {
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
        } catch (err) {
            return;
        }
    }

    private async commit(message?: string) {
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
