import { Disposable, SourceControl, window, workspace, WorkspaceFoldersChangeEvent } from "vscode";

import { SvnSourceControl } from "./svn-source-control";

import { client } from "./client";

class WorkspaceManager {
    private readonly disposable: Set<Disposable> = new Set();

    public readonly controls: Set<SvnSourceControl> = new Set();

    public constructor() {
        this.disposable.add(workspace.onDidChangeWorkspaceFolders(this.onDidChangeWorkspaceFolders));
        this.onDidChangeWorkspaceFolders({ added: workspace.workspaceFolders || [], removed: [] });
    }

    private async detect(workspaceRoot: string): Promise<void> {
        try {
            const root = await client.get_working_copy_root(workspaceRoot);

            // tslint:disable-next-line:no-shadowed-variable
            for (const control of this.controls) {
                if (control.root === root) {
                    control.workspaces.add(workspaceRoot);
                    return;
                }
            }

            const control = new SvnSourceControl(root);
            control.workspaces.add(workspaceRoot);
            await control.refresh();
            this.controls.add(control);
        } catch (err) {
            return;
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

export const workspaceManager = new WorkspaceManager();
