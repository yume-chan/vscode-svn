import { window } from "vscode";

import subscriptions from "./subscriptions";

const channel = window.createOutputChannel("Svn");
subscriptions.add(channel);

function padNumber(value: number): string {
    return value.toString().padStart(2, "0");
}

function formatTime(): string {
    const now = new Date();
    return `[${padNumber(now.getHours())}:${padNumber(now.getMinutes())}:${padNumber(now.getSeconds())}] `;
}

const indent = " ".repeat("[00:00:00] ".length + 4);

export function writeTrace(method: string, result: any) {
    channel.append(formatTime());
    channel.appendLine(method);

    channel.append(indent);
    if (result === undefined)
        channel.appendLine("undefined");
    else
        channel.appendLine(JSON.stringify(result).replace(/\r\n/g, "\r\n" + indent));
}

export function showOutput(): void {
    channel.show();
}
