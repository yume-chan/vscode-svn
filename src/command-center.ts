import { commands, Disposable, SourceControl, SourceControlResourceGroup, window, workspace } from "vscode";

import { Client } from "../svn";
import { client } from "./client";
import { SvnResourceState, SvnSourceControl } from "./svn-source-control";
import { workspaceManager } from "./workspace-manager";

class CommandCenter {
    private readonly disposable: Set<Disposable> = new Set();

    constructor() {
        this.disposable.add(commands.registerCommand("svn.update", this.update));

        this.disposable.add(commands.registerCommand("svn.commit", this.commit));
        this.disposable.add(commands.registerCommand("svn.refresh", this.refresh));

        this.disposable.add(commands.registerCommand("svn.stage", this.stage));
        this.disposable.add(commands.registerCommand("svn.stageAll", (group: SourceControlResourceGroup) => {
            return this.stage(...group.resourceStates as SvnResourceState[]);
        }));

        this.disposable.add(commands.registerCommand("svn.unstage", this.unstage));
        this.disposable.add(commands.registerCommand("svn.unstageAll", (group: SourceControlResourceGroup) => {
            return this.unstage(...group.resourceStates as SvnResourceState[]);
        }));

        this.disposable.add(commands.registerCommand("svn.openFile", async (...resourceStates: SvnResourceState[]) => {
            for (const item of resourceStates) {
                const document = await workspace.openTextDocument(item.resourceUri);
                await window.showTextDocument(document);
            }
        }));
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }

    private async update(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if ((control !== undefined))
            await control.update();
    }

    private async commit(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if (control !== undefined)
            await control.commit();
    }

    private async refresh(e?: SourceControl) {
        const control = await workspaceManager.find(e);
        if (control !== undefined)
            await control.refresh();
    }

    private async stage(...resourceStates: SvnResourceState[]) {
        if (resourceStates.length === 0)
            return;

        const control = resourceStates[0].control;

        for (const item of resourceStates) {
            if (!item.versioned)
                await client.add(item.path);
            else if (item.nodeStatus === Client.StatusKind.missing)
                await client.delete(item.path);
            else
                await client.changelistRemove(item.path);
        }

        await control.refresh();
    }

    private async unstage(...resourceStates: SvnResourceState[]) {
        if (resourceStates.length === 0)
            return;

        const control = resourceStates[0].control;

        for (const item of resourceStates) {
            switch (item.nodeStatus) {
                case Client.StatusKind.added:
                case Client.StatusKind.deleted:
                    await client.revert(item.path);
                    break;
                default:
                    await client.changelistAdd(item.path, "ignore-on-commit");
                    break;
            }
        }

        await control.refresh();
    }
}

export const commandCenter = new CommandCenter();
