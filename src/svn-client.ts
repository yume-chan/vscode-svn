import { execFile } from "child_process";

export enum SvnFileChange {
    None = " ",
    Added = "A",
    Deleted = "D",
    Modified = "M",
    Replaced = "R",
    Conflicted = "C",
    External = "X",
    Ignored = "I",
    Untracked = "?",
    Missing = "!",
    TypeChanged = "~",
}

export enum SvnPropertyChange {
    None = " ",
    Modified = "M",
    Conflicted = "C",
}

export enum SvnLockOwner {
    NotLocked = " ",
    Me = "K",
    OtherUser = "O",
    Stolen = "T",
    Broken = "B",
}

export interface SvnItemState {
    relativePath: string;
    fileChange: SvnFileChange;
    propertyChange: SvnPropertyChange;
    locked: boolean;
    history: boolean;
    switch: boolean;
    lockOwner: SvnLockOwner;
    treeConflict: boolean;
}

export enum SvnErrorCode {
    NotWorkingCopy = 155007
}

export interface SvnErrorItem {
    id: SvnErrorCode;
    message: string;
}

export class SvnError {
    constructor(public warning: SvnErrorItem[], public error: SvnErrorItem[]) { }
}

function splitLines(source: string): string[] {
    const list = source.split("\r\n");
    list.pop();
    return list;
}

function parseItemState(line: string): SvnItemState {
    return {
        relativePath: line.substring(8),
        fileChange: line[0] as SvnFileChange,
        propertyChange: line[1] as SvnPropertyChange,
        locked: line[2] != " ",
        history: line[3] != " ",
        switch: line[4] != " ",
        lockOwner: line[5] as SvnLockOwner,
        treeConflict: line[6] != " ",
    };
}

function parseItemStates(lines: string[]): SvnItemState[] {
    return lines.map(parseItemState);
}

interface SvnCommitTextMessage {
    isFile: false;
    message: string;
}

interface SvnCommitFileMessage {
    isFile: true;
    file: string;
}

type SvnCommitMessage = SvnCommitTextMessage | SvnCommitFileMessage;

interface SvnCommitListTargets {
    isFile: false;
    files: string[];
}

interface SvnCommitFileTargets {
    isFile: true;
    file: string;
}

type SvnCommitTargets = SvnCommitListTargets | SvnCommitFileTargets;

export class SvnClient {
    constructor(public svnPath: string = "svn") { }

    private execSvn(command: string, ...args: string[]): Promise<string> {
        return new Promise<string>((resolve, reject) => {
            execFile(this.svnPath, [command, ...args], (err, stdout, stderr) => {
                if (stderr !== undefined && stderr !== "") {
                    const lines = splitLines(stderr);

                    const warning: SvnErrorItem[] = [];
                    const error: SvnErrorItem[] = [];

                    for (let line of lines) {
                        if (line.startsWith("svn: ")) {
                            line = line.substring(5);

                            let level: SvnErrorItem[];
                            if (line.startsWith("warning: ")) {
                                line = line.substring(10);
                                level = warning;
                            } else {
                                line = line.substring(1);
                                level = error;
                            }

                            const message = line.substring(8);
                            if (message == "Commit failed (details follow):")
                                continue;

                            const id = parseInt(line.substring(0, 6), 10);
                            level.push({ id, message });
                        }
                    }

                    reject(new SvnError(warning, error));
                } else if (err !== null) {
                    reject(err);
                } else {
                    resolve(stdout);
                }
            });
        });
    }

    public add(...target: string[]): Promise<SvnItemState[]> {
        return this.execSvn("add", ...target).then(splitLines).then(parseItemStates);
    }

    public cat(target: string): Promise<string> {
        return this.execSvn("cat", target);
    }

    public status(target: string): Promise<SvnItemState[]> {
        return this.execSvn("status", target).then(splitLines).then(parseItemStates);
    }

    public commit(message: SvnCommitMessage, targets: SvnCommitTargets): Promise<string> {
        const messageArgs = message.isFile ? ["--encoding", "UTF-8", "--file", message.file] : ["--message", message.message];
        const targetsArgs = targets.isFile ? ["--targets", targets.file] : targets.files;
        return this.execSvn("commit", ...messageArgs, ...targetsArgs);
    }

    public revert(...files: string[]): Promise<SvnItemState[]> {
        return this.execSvn("revert", "--recursive", ...files).then(splitLines).then(parseItemStates);
    }
}
