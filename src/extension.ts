import { ExtensionContext } from "vscode";

import { showErrorMessage, writeError, writeTrace } from "./output";
import subscriptions from "./subscriptions";

import { initialize } from "./node-svn";

export async function activate(context: ExtensionContext) {
    context.subscriptions.push(subscriptions);

    try {
        if (!await initialize()) {
            return;
        }

        await import("./workspace-manager");
        await import("./content-provider");
        await import("./decoration-provider");
        await import("./command-center");

        writeTrace(`initialize()`, process.pid);
    } catch (err) {
        writeError(`initialize()`, err);
        showErrorMessage("Initialize");
    }
}
