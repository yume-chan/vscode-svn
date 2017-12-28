import { window } from "vscode";

import { AsyncClient } from "node-svn";

export const config = {
    async getSimpleCredential(realm: string, username?: string) {
        username = await window.showInputBox({
            ignoreFocusOut: true,
            prompt: `Username for ${realm}:`,
            value: username,
        });
        if (username === undefined)
            return undefined;

        const password = await window.showInputBox({
            ignoreFocusOut: true,
            password: true,
            prompt: `Password for ${realm}:`,
        });
        if (password === undefined)
            return undefined;

        const yes = {
            description: "Save password unencrypted",
            label: "Yes",
        };
        const no = {
            description: "Don't save password",
            label: "No",
        };
        const save = (await window.showQuickPick([yes, no], {
            ignoreFocusOut: true,
            placeHolder: "Save password (unencrypted)?",
        })) === yes;

        return {
            username,
            password,
            save,
        };
    },
};

export let client: AsyncClient;

export function formatError(error: Error): string {
    let message = error.message;

    let child = (error as any).child;
    while (child !== undefined) {
        message += "\r\n" + child.message;
        child = child.child;
    }

    return message;
}

export async function initialize() {
    // tslint:disable-next-line:no-shadowed-variable
    const { AsyncClient } = await import("node-svn");
    client = new AsyncClient();
}
