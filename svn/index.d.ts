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
    textStatus: Client.StatusKind;
    propStatus: Client.StatusKind;
    copied: boolean;
    switched: boolean;
}

export type SvnStatusResult = SvnStatus[] & { version?: number };

export class Client {
    public constructor();

    public status(path: string): Promise<SvnStatusResult>;
    public cat(path: string): Promise<Buffer>;
}

export class SvnError extends Error {
    code: number;
    message: string;

    constructor(code: number, message: string);
}
