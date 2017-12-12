import { window } from "vscode";

import subscriptions from "./subscriptions";

const channel = window.createOutputChannel("Svn");
subscriptions.add(channel);

function padNumber(value: number): string {
    return value.toString().padStart(2, "0");
}

function formatTime(): string {
    const now = new Date();
    return `[${padNumber(now.getHours())}:${padNumber(now.getMinutes())}:${padNumber(now.getSeconds())}]`;
}

const newLine = "\n" + " ".repeat("[00:00:00] ".length);

function processContent(content: string): string {
    return content.replace(/\n/g, newLine).replace(/\t/g, "    ");
}

export function writeOutput(content: string): void {
    channel.appendLine(`${formatTime()} ${processContent(content)}`);
}

export function showOutput(): void {
    channel.show();
}
