import { ExtensionContext, window } from "vscode";

import opn = require("opn");

import { initialize } from "./client";
import { showErrorMessage, writeError, writeTrace } from "./output";
import subscriptions from "./subscriptions";

const platformName = {
    linux: "Linux",
    mac: "macOS",
    win32: "Windows",
};

export async function activate(context: ExtensionContext) {
    context.subscriptions.push(subscriptions);

    try {
        const configuration = require("../configuration.json");

        if (process.platform !== configuration.platform ||
            process.arch !== configuration.arch) {
            const more = "Learn more";
            const result = await window.showErrorMessage(`This package of vscode-svn only supports ${platformName[configuration.platform] || configuration.platform} ${configuration.arch}.`, more);
            switch (result) {
                case more:
                    opn("https://github.com/yume-chan/vscode-svn#platform");
                    break;
            }
            return;
        }
    } catch  {
        // Nothing
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
        showErrorMessage("Initialize");
    }
}
