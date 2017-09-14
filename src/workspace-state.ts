import * as vscode from "vscode";

export class WorkspaceState {
    public unstage: Set<string>;

    public constructor(private state: vscode.Memento) {
        this.unstage = new Set<string>(this.state.get<string[]>("svn.unstage"));
    }

    public save() {
        return this.state.update("svn.unstage", Array.from(this.unstage));
    }
}
