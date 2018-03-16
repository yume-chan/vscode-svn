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
        channel.appendLine(JSON.stringify(result).replace(/\r?\n/g, "\n" + indent));
}

export function showOutput(): void {
    channel.show();
}

export function writeError(method: string, error: Error): void {
    let message = error.message;

    if (error.name === "SvnError") {
        message = `E${(error as any).code} ${message}`;

        let child = (error as any).child;
        while (child !== undefined) {
            message += `\r\n${"    "}E${(error as any).code} ${error.message}`;
            child = child.child;
        }
    }

    channel.append(formatTime());
    channel.appendLine(method);

    channel.append(indent);
    channel.append(error.name);
    channel.append(": ");
    channel.appendLine(message.replace(/\r?\n/g, "\n" + indent + "    "));
}

export async function showErrorMessage(operation: string): Promise<void> {
    const item = "Show Detail";
    const selected = await window.showErrorMessage(`${operation} failed, check output for detail`, item);
    switch (selected) {
        case item:
            showOutput();
            break;
        default:
            return;
    }
}
