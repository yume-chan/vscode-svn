import fs from "fs-extra";
import path from "path";
import request from "request";
import { Stream } from "stream";

import { window } from "vscode";

import { writeTrace } from "./output";

export * from "./node-svn.type";

function pipe(from: Stream, to: NodeJS.WritableStream) {
    return new Promise((resolve, reject) => {
        from.on("error", (err) => reject(err));
        from.on("finish", () => resolve());
        from.pipe(to);
    });
}

export async function initialize(): Promise<boolean> {
    const filepath = path.resolve(__dirname, "..", "native", "svn.node");

    const packageJson = await import("../package.json");
    const version = packageJson.version;

    await fs.mkdirp(path.dirname(filepath));

    try {
        if (!await fs.pathExists(filepath)) {
            const url = `https://chensi.moe/vscode-svn/${version}_${process.platform}-${process.arch}_electron-${process.versions.modules}.node`;
            writeTrace("downloading native dependencies...", url);
            const response = request(url);
            await pipe(response, fs.createWriteStream(filepath));
        }
    } catch (err) {
        await window.showErrorMessage(`vscode-svn: initialization failed: ${err}`);
    }

    try {
        const svn = await import(filepath);
        for (const key in svn) {
            module.exports[key] = svn[key];
        }
        return true;
    } catch (err) {
        if (await fs.pathExists(filepath)) {
            await fs.unlink(filepath);
        }

        await window.showErrorMessage(`vscode-svn: initialization failed: ${err}`);
    }

    return false;
}
