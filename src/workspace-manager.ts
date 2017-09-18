import { Disposable, SourceControl, workspace, WorkspaceFoldersChangeEvent } from "vscode";

import { SvnSourceControl } from "./svn-source-control";

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

    private async onDidChangeWorkspaceFolders(e: WorkspaceFoldersChangeEvent) {
        for (const item of e.added) {
            const control = await SvnSourceControl.detect(item.uri.fsPath);
            if (control !== undefined)
                this.controls.add(control);
        }

        for (const item of e.removed) {
            for (const control of this.controls) {
                if (control.file === item.uri.fsPath) {
                    control.dispose();
                    this.controls.delete(control);
                }
                break;
            }
        }
    }
}

export const workspaceManager = new WorkspaceManager();
