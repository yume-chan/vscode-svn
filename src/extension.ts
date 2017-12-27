import { ExtensionContext } from "vscode";

import { initialize } from "./client";
import { writeTrace } from "./output";
import subscriptions from "./subscriptions";

export async function activate(context: ExtensionContext) {
    context.subscriptions.push(subscriptions);
    try {
        await initialize();

        await import("./workspace-manager");
        await import("./content-provider");
        await import("./svn-decoration-provider");
        await import("./command-center");

        writeTrace(`initialize()`, process.pid);
    } catch (err) {
        writeTrace(`initialize()`, err.toString());
    }
}
