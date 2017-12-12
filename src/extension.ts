import { ExtensionContext } from "vscode";

import { initialize } from "./client";
import { writeOutput } from "./output";
import subscriptions from "./subscriptions";

export async function activate(context: ExtensionContext) {
    context.subscriptions.push(subscriptions);
    try {
        await initialize();

        await import("./workspace-manager");
        await import("./content-provider");
        await import("./svn-decoration-provider");
        await import("./command-center");

        writeOutput(`initialize()\n\t${process.pid}`);
    } catch (err) {
        writeOutput(`initialize()\n\t${err}`);
    }
}
