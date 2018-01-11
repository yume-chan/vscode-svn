import { ConfigurationChangeEvent, Disposable, SourceControl, Uri, window, workspace, WorkspaceFoldersChangeEvent } from "vscode";

import { SvnSourceControl } from "./source-control";

import { client } from "./client";
import { writeError, writeTrace } from "./output";
import isSamePath from "./same-path";
import subscriptions from "./subscriptions";

class WorkspaceManager {
    private readonly disposable: Set<Disposable> = new Set();

    public readonly controls: Set<SvnSourceControl> = new Set();

    public constructor() {
        this.disposable.add(workspace.onDidChangeConfiguration(this.onDidChangeConfiguration, this));

        this.disposable.add(workspace.onDidChangeWorkspaceFolders(this.onDidChangeWorkspaceFolders, this));
        this.onDidChangeWorkspaceFolders({ added: workspace.workspaceFolders || [], removed: [] });
    }

    private async detect(workspaceRoot: string): Promise<void> {
        try {
            const configuration = workspace.getConfiguration("svn", Uri.file(workspaceRoot));
            // tslint:disable-next-line:prefer-const
            let enabled = configuration.get<boolean | undefined>("enabled", undefined);
            if (enabled === false) {
                writeTrace(`configuration.get("enabled", "${workspaceRoot}")`, false);
                return;
            }

            const root = await client.get_working_copy_root(workspaceRoot);
            writeTrace(`get_working_copy_root("${workspaceRoot}")`, root);

            let show_changes_from = configuration.get<string | undefined>("show_changes_from", undefined);

            // tslint:disable:comment-format
            // if (!samePath(root, workspaceRoot)) {
            //     const root_configuration = workspace.getConfiguration("svn", Uri.file(root));

            //     enabled = root_configuration.get<boolean | undefined>("enabled", undefined);
            //     if (enabled === false)
            //         return;

            //     show_changes_from = root_configuration.get<string | undefined>("show_changes_from", undefined);
            // }
            // tslint:enable:comment-format

            if (show_changes_from === undefined)
                show_changes_from = "working_copy";

            const controlRoot = show_changes_from === "working_copy" ? root : workspaceRoot;

            // tslint:disable-next-line:no-shadowed-variable
            for (const control of this.controls) {
                if (isSamePath(control.root, controlRoot)) {
                    writeTrace(`control.workspaces.add(${workspaceRoot})`, control.root);
                    control.workspaces.add(workspaceRoot);
                    return;
                }
            }

            writeTrace(`new SvnSourceControl("${controlRoot}")`, undefined);
            const control = new SvnSourceControl(controlRoot);
            control.workspaces.add(workspaceRoot);
            await control.refresh();
            this.controls.add(control);
        } catch (err) {
            writeError(`get_working_copy_root("${workspaceRoot}")`, err);
            return;
        }
    }

    private reload() {
        for (const item of this.controls)
            item.dispose();
        this.controls.clear();

        this.onDidChangeWorkspaceFolders({ added: workspace.workspaceFolders || [], removed: [] });
    }

    private onDidChangeConfiguration(e: ConfigurationChangeEvent) {
        if (e.affectsConfiguration("svn")) {
            writeTrace(`e.affectsConfiguration("svn")`, true);
            this.reload();
            return;
        }

        for (const control of this.controls) {
            if (e.affectsConfiguration("svn", Uri.file(control.root))) {
                writeTrace(`e.affectsConfiguration("svn", "${control.root}")`, true);
                this.reload();
                return;
            }
        }
    }

    private async onDidChangeWorkspaceFolders(e: WorkspaceFoldersChangeEvent) {
        for (const item of e.added)
            await this.detect(item.uri.fsPath);

        for (const item of e.removed) {
            const workspaceRoot = item.uri.fsPath;

            for (const control of this.controls) {
                if (control.workspaces.has(workspaceRoot)) {
                    control.workspaces.delete(workspaceRoot);

                    if (control.workspaces.size === 0) {
                        control.dispose();
                        this.controls.delete(control);
                    }
                    break;
                }
            }
        }
    }

    public async find(hint?: SourceControl): Promise<SvnSourceControl | undefined> {
        if (hint !== undefined)
            return hint.quickDiffProvider as SvnSourceControl;

        if (this.controls.size === 1)
            for (const control of this.controls)
                return control;

        const selected = await window.showWorkspaceFolderPick({ ignoreFocusOut: true, placeHolder: "Select a workspace to continue" });
        if (selected === undefined)
            return;

        const workspaceRoot = selected.uri.fsPath;
        for (const control of this.controls)
            if (control.workspaces.has(workspaceRoot))
                return control;

        return undefined;
    }

    public dispose(): void {
        for (const item of this.controls)
            item.dispose();
        this.controls.clear();

        for (const item of this.disposable)
            item.dispose();
        this.disposable.clear();
    }
}

export default subscriptions.add(new WorkspaceManager());
