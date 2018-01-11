import { ExtensionContext, window } from "vscode";

import opn = require("opn");

import { initialize } from "./client";
import { writeError, writeTrace } from "./output";
import subscriptions from "./subscriptions";

export async function activate(context: ExtensionContext) {
    context.subscriptions.push(subscriptions);

    if (process.platform !== "win32") {
        const more = "Learn more";
        const result = await window.showErrorMessage("vscode-svn currently only supports Windows.", more);
        switch (result) {
            case more:
                opn("https://github.com/yume-chan/vscode-svn##platform");
                break;
        }
        return;
    }

    try {
        await initialize();

        await import("./workspace-manager");
        await import("./content-provider");
        await import("./decoration-provider");
        await import("./command-center");

        writeTrace(`initialize()`, process.pid);
    } catch (err) {
        writeError(`initialize()`, err);

        if (process.arch === "ia32") {
            const more = "Learn more";
            const result = await window.showErrorMessage("This package of vscode-svn only supports x64.", more);
            switch (result) {
                case more:
                    opn("https://github.com/yume-chan/vscode-svn##platform");
                    break;
            }
            return;
        }
    }
}
