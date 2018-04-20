import { window } from "vscode";

import { Client } from "./node-svn";

async function simple_auth_provider(realm: string, username: string | undefined, may_save: boolean) {
    // ask username
    username = await window.showInputBox({
        ignoreFocusOut: true,
        prompt: `Username for ${realm}:`,
        value: username,
    });
    if (username === undefined)
        return undefined;

    // ask password
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

const pool: Set<Client> = new Set();
const size = 10;

export default {
    get(): Client {
        // create new
        if (pool.size === 0) {
            const client = new Client();
            client.add_simple_auth_provider(simple_auth_provider);
            return client;
        }

        // get from pool
        for (const client of pool) {
            pool.delete(client);
            return client;
        }

        // make compiler happy
        throw new Error("unreachable");
    },
    release(client: Client) {
        // throw away
        if (pool.size >= size) {
            client.dispose();
            return;
        }

        // put into pool
        pool.add(client);
    },
};
