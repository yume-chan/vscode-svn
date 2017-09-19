import Buffer = require("buffer");

export interface SvnVersion {
    major: number;
    minor: number;
    patch: number;
}

export var version: SvnVersion;

export namespace Client {
    enum Kind {
        none,
        file,
        dir,
        unknown,
    }

    enum StatusKind {
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

    enum Depth {
        unknown,
        exclude,
        empty,
        files,
        immediates,
        infinity,
    }

    enum RevisionKind {
        unspecified,
        committed,
        previous,
        base,
        working,
        head,
    }
}

export interface SvnStatusOptions {
    depth: Client.Depth;
    getAll: boolean;
    checkOutOfDate: boolean;
    checkWorkingCopy: boolean;
    noIgnore: boolean;
    ignoreExternals: boolean;
    depthAsSticky: boolean;
    changelists: string | string[];
}

export interface SvnStatus {
    kind: Client.Kind;
    path: string;
    filesize: number | string;
    versioned: boolean;
    conflicted: boolean;
    nodeStatus: Client.StatusKind;
    textStatus: Client.StatusKind;
    propStatus: Client.StatusKind;
    copied: boolean;
    switched: boolean;
    repositoryUrl: string;
    relativePath: string;
}

export type SvnStatusResult = SvnStatus[] & { version?: number };

export interface SvnNotify {

}

export interface SimpleCredential {
    username: string;
    password: string;
    save?: boolean;
}

export interface ClientOptions {
    getSimpleCredential(realm: string | undefined, username: string | undefined): Promise<SimpleCredential | undefined>;
}

export interface SvnWorkingCopyInfo {
    rootPath: string;
}

export interface SvnInfo {
    path: string;
    workingCopy: SvnWorkingCopyInfo | undefined;
}

export type SvnRevision = Client.RevisionKind | { date: number } | { number: number };

export interface SvnCatOptions {
    pegRevision: SvnRevision;
    revision: SvnRevision;
    expandKeywords: boolean;
}

export interface SvnInfoOptions {
    pegRevision: SvnRevision;
    revision: SvnRevision;
    depth: Client.Depth;
    fetchExcluded: boolean;
    fetchActualOnly: boolean;
    includeExternals: boolean;
    changelists: string | string[];
}

export class Client {
    public constructor(options?: Partial<ClientOptions>);

    public add(path: string): Promise<void>;
    public cat(path: string, options?: Partial<SvnCatOptions>): Promise<Buffer>;
    public changelistAdd(path: string | string[], changelist: string): Promise<void>;
    public changelistRemove(path: string | string[]): Promise<void>;
    public checkout(url: string, path: string): Promise<void>;
    public commit(path: string | string[], message: string): Promise<void>;
    public delete(path: string | string[]): Promise<void>;
    public info(path: string | string[], options?: Partial<SvnInfoOptions>): Promise<SvnInfo[]>;
    public status(path: string, options?: Partial<SvnStatusOptions>): Promise<SvnStatusResult>;
    public revert(path: string | string[]): Promise<void>;
    public update(path: string | string[]): Promise<void>;
}

export class SvnError extends Error {
    code: number;
    message: string;

    constructor(code: number, message: string);
}
