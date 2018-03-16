import { window } from "vscode";

import { Client } from "node-svn";

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
        may_save,
        password,
        username,
    };
}

export let client: Client;

export async function initialize() {
    // tslint:disable-next-line:no-shadowed-variable
    const { Client } = await import("node-svn");
    client = new Client();
    client.add_simple_auth_provider(simple_auth_provider);
}
