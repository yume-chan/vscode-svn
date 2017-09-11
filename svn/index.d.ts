import Buffer = require("buffer");

export interface SvnVersion {
    major: number;
    minor: number;
    patch: number;
}

export var version: SvnVersion;

export namespace Client {
    export enum Kind {
        none,
        file,
        dir,
        unknown,
    }

    export enum StatusKind {
        none,
        unversioned,
        normal,
        added,
        missing,
        deleted,
        replaced,
        modified,
        conflicted,
        ignored,
        obstructed,
        external,
        incomplete,
    }
}

export interface SvnStatus {
    path: string;
    kind: Client.Kind;
    nodeStatus: Client.StatusKind;
    textStatus: Client.StatusKind;
    propStatus: Client.StatusKind;
    versioned: boolean;
    conflicted: boolean;
    copied: boolean;
    switched: boolean;
}

export type SvnStatusResult = SvnStatus[] & { version?: number };

export interface SvnNotify {

}

export class Client {
    public constructor();

    public add(path: string): Promise<void>;
    public cat(path: string): Promise<Buffer>;
    public checkout(url: string, path: string): Promise<void>;
    public commit(path: string | string[], message: string): Promise<void>;
    public status(path: string): Promise<SvnStatusResult>;
    public revert(path: string | string[]): Promise<void>;
    public update(path: string | string[]): Promise<void>;
}

export class SvnError extends Error {
    code: number;
    message: string;

    constructor(code: number, message: string);
}
