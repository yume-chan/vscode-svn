import { exec } from "child_process";

import { ExtensionContext, window } from "vscode";

import { initialize } from "./client";
import { writeError, writeTrace } from "./output";
import subscriptions from "./subscriptions";

export async function activate(context: ExtensionContext) {
    context.subscriptions.push(subscriptions);

    if (process.platform !== "win32") {
        const help = "I want to help";
        const result = await window.showErrorMessage("Svn only supports Windows.", help);
        switch (result) {
            case help:
                exec("https://github.com/yume-chan/node-svn#platform-table");
                break;
        }
        return;
    }

    if (process.arch === "ia32") {
        const download = "Download x86 version";
        const result = await window.showErrorMessage("This package of Svn only supports x64.", download);
        switch (result) {
            case download:
                exec("https://ci.appveyor.com/api/buildjobs/09qln5lubggj3w9r/artifacts/vsix%2Fvscode-svn-0.0.9-ia32.vsix");
                break;
        }
        return;
    }

    try {
        await initialize();

        await import("./workspace-manager");
        await import("./content-provider");
        await import("./svn-decoration-provider");
        await import("./command-center");

        writeTrace(`initialize()`, process.pid);
    } catch (err) {
        writeError(`initialize()`, err);
    }
}
