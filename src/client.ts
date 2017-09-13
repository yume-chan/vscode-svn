import * as vscode from "vscode";

import { Client, SvnError, SvnStatus, SvnStatusResult } from "../svn";

export const client = new Client({
    async getSimpleCredential(realm, username) {
        username = await vscode.window.showInputBox({
            ignoreFocusOut: true,
            prompt: `Username for ${realm}:`,
            value: username,
        });
        if (username === undefined)
            return undefined;

        const password = await vscode.window.showInputBox({
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
        const save = (await vscode.window.showQuickPick([yes, no], {
            ignoreFocusOut: true,
            placeHolder: "Save password (unencrypted)?",
        })) === yes;

        return {
            username,
            password,
            save,
        };
    },
});
