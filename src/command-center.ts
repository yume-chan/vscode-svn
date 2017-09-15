import { commands, Disposable, SourceControlResourceGroup } from "vscode";

import { Client } from "../svn";
import { client } from "./client";
import { SvnResourceState, SvnSourceControl } from "./svn-source-control";

class CommandCenter {
    private readonly disposables: Disposable[] = [];

    constructor() {
        this.disposables.push(commands.registerCommand("svn.update", (control: SvnSourceControl) => control.update()));
        this.disposables.push(commands.registerCommand("svn.commit", this.commit));

        this.disposables.push(commands.registerCommand("svn.stage", this.stage));

        this.disposables.push(commands.registerCommand("svn.stageAll", (group: SourceControlResourceGroup) => {
            return this.stage(...group.resourceStates as SvnResourceState[]);
        }));

        this.disposables.push(commands.registerCommand("svn.unstage", async (...resourceStates: SvnResourceState[]) => {
            if (resourceStates.length === 0)
                return;

            const control = resourceStates[0].control;

            for (const item of resourceStates) {
                switch (item.nodeStatus) {
                    case Client.StatusKind.added:
                        await client.revert(item.path);
                        break;
                    default:
                        await client.changelistAdd(item.path, "ignore-on-commit");
                        break;
                }
            }

            await control.refresh();
        }));
    }

    public dispose(): void {
        for (const item of this.disposables)
            item.dispose();
    }

    private commit(...args: any[]) {
        return;
    }

    private async stage(...resourceStates: SvnResourceState[]) {
        if (resourceStates.length === 0)
            return;

        const control = resourceStates[0].control;

        for (const item of resourceStates) {
            if (!item.versioned)
                await client.add(item.path);
            else
                await client.changelistRemove(item.path);
        }

        await control.refresh();
    }
}

export const commandCenter = new CommandCenter();
