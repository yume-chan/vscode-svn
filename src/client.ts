import { window } from "vscode";

import { AsyncClient } from "node-svn";

async function simple_auth_provider(realm: string, username: string | undefined, may_save: boolean) {
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
        prompt: `Password for ${username}:`,
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
    may_save = (await window.showQuickPick([yes, no], {
        ignoreFocusOut: true,
        placeHolder: "Save password (unencrypted)?",
    })) === yes;

    return {
        password,
        username,
        may_save,
    };
}

export let client: AsyncClient;

export async function initialize() {
    // tslint:disable-next-line:no-shadowed-variable
    const { AsyncClient } = await import("node-svn");
    client = new AsyncClient();
    client.add_simple_auth_provider(simple_auth_provider);
}
