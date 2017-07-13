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

export class SvnClient {
    constructor(public base: string, public filePath: string) { }

    private exec(command: string, ...args: string[]): Promise<string> {
        return new Promise<string>((resolve, reject) => {
            execFile("svn", [command, ...args], { cwd: this.base }, (err, stdout, stderr) => {
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

                            const id = parseInt(line.substring(0, 6), 10);
                            const message = line.substring(8);
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

    public add(...files: string[]): Promise<SvnItemState[]> {
        return this.exec("add", ...files).then(splitLines).then(parseItemStates);
    }

    public cat(filePath: string): Promise<string> {
        return this.exec("cat", filePath).catch(() => "");
    }

    public status(): Promise<SvnItemState[]> {
        return this.exec("status").then(splitLines).then(parseItemStates);
    }

    public commit(message: string, ...files: string[]): Promise<string> {
        return this.exec("commit", "--message", message, ...files);
    }

    public revert(...files: string[]): Promise<SvnItemState[]> {
        return this.exec("revert", "--recursive", ...files).then(splitLines).then(parseItemStates);
    }
}
