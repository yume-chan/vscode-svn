import { Disposable, SourceControl, window, workspace, WorkspaceFoldersChangeEvent } from "vscode";

import { SvnSourceControl } from "./svn-source-control";

import { client } from "./client";

class WorkspaceManager {
    public readonly controls: Set<SvnSourceControl> = new Set();

    private readonly disposable: Set<Disposable> = new Set();

    public constructor() {
        this.disposable.add(workspace.onDidChangeWorkspaceFolders(this.onDidChangeWorkspaceFolders));
        this.onDidChangeWorkspaceFolders({ added: workspace.workspaceFolders || [], removed: [] });
    }

    public async find(hint?: SourceControl): Promise<SvnSourceControl | undefined> {
        if (hint !== undefined)
            return hint.quickDiffProvider as SvnSourceControl;

        if (this.controls.size === 1)
            return this.controls.values().next().value;

        // TODO: Show Workspace Picker
    }

    public dispose(): void {
        for (const item of this.controls)
            item.dispose();

        this.controls.clear();

        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }

    private async detect(file: string): Promise<void> {
        try {
            const root = await client.get_working_copy_root(file);

            // tslint:disable-next-line:no-shadowed-variable
            for (const control of this.controls) {
                if (control.root === root) {
                    control.workspaces.add(file);
                    return;
                }
            }

            const control = new SvnSourceControl(root);
            control.workspaces.add(file);
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
            const file = item.uri.fsPath;

            for (const control of this.controls) {
                if (control.workspaces.has(file)) {
                    control.workspaces.delete(file);

                    if (control.workspaces.size === 0) {
                        control.dispose();
                        this.controls.delete(control);
                    }
                    break;
                }
            }
        }
    }
}

export const workspaceManager = new WorkspaceManager();
