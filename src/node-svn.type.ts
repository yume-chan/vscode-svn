export interface CommitItem {
    author: string;
    date: string;
    repos_root: string;
    revision: number;
    post_commit_error: string | undefined;
}

export interface InfoItem {
    path: string;
    kind: NodeKind;
    last_changed_author: string;
    last_changed_date: number | string;
    last_changed_rev: number;
    repos_root_url: string;
    url: string;
}

export interface StatusItem {
    path: string;
    changed_author: string;
    changed_date: number | string;
    changed_rev: number;
    conflicted: boolean;
    copied: boolean;
    depth: Depth;
    file_external: boolean;
    kind: NodeKind;
    node_status: StatusKind;
    prop_status: StatusKind;
    revision: number;
    text_status: StatusKind;
    versioned: boolean;
    changelist?: string;
}

export type Revision = RevisionKind |
    { number: number } |
    { date: number };

export interface DepthOption {
    /** The depth of the operation. */
    depth: Depth;
}

export interface ChangelistsOption {
    changelists: string | string[];
}

export type AddToChangelistOptions = DepthOption & ChangelistsOption;

export type GetChangelistsOptions = DepthOption & ChangelistsOption;

export type RemoveFromChangelistsOptions = DepthOption & ChangelistsOption;

export type AddOptions = DepthOption;

export interface RevisionOption {
    /** The operative revision. */
    revision: Revision;
}

export interface PegRevisionOpitons extends RevisionOption {
    /** The peg revision. */
    peg_revision: Revision;
}

// tslint:disable-next-line
export interface UpdateOptions extends RevisionOption {

}

export interface CatOptions extends PegRevisionOpitons {
    /**
     * default values:
     * * `RevisionKind.head` for url
     * * `RevisionKind.working` for file
     */
    peg_revision: Revision;

    /**
     * default values:
     * * `peg_revision` if not `RevisionKind.unspecified`
     * * `RevisionKind.head` for url
     * * `RevisionKind.base` for file
     */
    revision: Revision;
}

export interface CatResult {
    content: Buffer;
    properties: { [key: string]: string };
}

export type CheckoutOptions = DepthOption & PegRevisionOpitons;

export type InfoOptions = DepthOption & PegRevisionOpitons;

export type StatusOptions = DepthOption & RevisionOption & {
    /** If true, don't process externals definitions as part of this operation. */
    ignore_externals: boolean;
};

interface SimpleAuth {
    username: string;
    password: string;
    may_save: boolean;
}

interface GetChangelistsItem {
    path: string;
    changelist: string;
}

interface BlameOptions extends PegRevisionOpitons {
    start_revision: Revision;
    end_revision: Revision;
}

interface BlameItem {
    start_revision: number;
    end_revision: number;
    line_number: number;
    revision: number | undefined;
    merged_revision: number | undefined;
    merged_path: string | undefined;
    line: string;
    local_change: boolean;
}

interface RevisionRange {
    start: Revision;
    end: Revision;
}

interface LogOptions extends PegRevisionOpitons {
    revision_ranges: RevisionRange | RevisionRange[];
    limit: number;
}

interface LogItem {
    revision: number;
    non_inheritable: boolean;
    subtractive_merge: boolean;
    author: string | undefined;
    date: string | undefined;
    message: string | undefined;
}

type AuthProviderResult<T> = undefined | T | Promise<undefined | T>;
type SimpleAuthProvider = (realm: string, username: string | undefined, may_save: boolean) => AuthProviderResult<SimpleAuth>;

export declare enum UpdateNotifyAction {
    delete,
    add,
    update,
    completed,
    external,
    replace,
    started,
    skip_obstruction,
    skip_working_only,
    skip_access_denied,
    external_removed,
    shadowed_add,
    shadowed_update,
    shadowed_delete,
}

interface Notify<T> {
    action: T;
    path: string;
}

interface UpdateProgressNotify extends Notify<UpdateNotifyAction> {
    revision?: number;
}

export declare enum CommitNotifyAction {
    modified,
    added,
    deleted,
    replaced,
    postfix_txdelta,
    finalize,
}

interface CommitProgressNotify extends Notify<CommitNotifyAction> {

}

interface CommitFinalizeNotify extends CommitProgressNotify {
    author: string;
    date: string;
    repos_root: string;
    revision: number;
    post_commit_error: string | undefined;
}

type CommitNotify = CommitProgressNotify | CommitFinalizeNotify;

export declare function is_commit_finalize_notify(value: CommitNotify): value is CommitFinalizeNotify;

interface RemoveOptions {
    force: boolean;
    keep_local: boolean;
}

export declare class Client {
    constructor(config_path?: string);

    public add_simple_auth_provider(provider: SimpleAuthProvider): void;
    public remove_simple_auth_provider(provider: SimpleAuthProvider): void;

    public add_to_changelist(path: string | string[], changelist: string, options?: Partial<AddToChangelistOptions>): Promise<void>;
    public get_changelists(path: string, options?: Partial<GetChangelistsOptions>): AsyncIterable<GetChangelistsItem>;
    public remove_from_changelists(path: string | string[], options?: Partial<RemoveFromChangelistsOptions>): Promise<void>;

    /**
     * Schedule a working copy path for addition to the repository.
     */
    public add(path: string, options?: Partial<AddOptions>): Promise<void>;
    public blame(path: string, options?: Partial<BlameOptions>): AsyncIterable<BlameItem>;
    public cat(path: string, options?: Partial<CatOptions>): Promise<CatResult>;
    /**
     * Check out a working copy from a repository.
     *
     * @param url The repository URL of the checkout source.
     * @param path The root of the new working copy.
     * @param options The options of the checkout.
     *
     * @returns The value of the revision checked out from the repository.
     */
    public checkout(url: string, path: string, options?: Partial<CheckoutOptions>): Promise<number>;
    public cleanup(path: string): Promise<void>;
    public commit(path: string | string[], message: string): AsyncIterable<CommitNotify>;

    public info(path: string, options?: Partial<InfoOptions>): AsyncIterable<InfoItem>;
    public log(path: string | string[], options?: Partial<LogOptions>): AsyncIterable<LogItem>;

    public remove(path: string | string[], options?: Partial<RemoveOptions>): AsyncIterable<CommitItem>;
    public resolve(path: string): Promise<void>;
    public revert(path: string | string[]): Promise<void>;

    public status(path: string, options?: Partial<StatusOptions>): AsyncIterable<StatusItem>;

    public update(path: string | string[], options?: Partial<UpdateOptions>): AsyncIterable<UpdateProgressNotify>;

    public get_working_copy_root(path: string): Promise<string>;

    public dispose(): void;
}

export declare enum ConflictChoose {
    postpone,
    base,
    theirs_full,
    mine_full,
    theirs_conflict,
    mine_conflict,
    merged,
    unspecified,
}

export declare enum Depth {
    unknown,
    empty,
    files,
    immediates,
    infinity,
}

export declare enum NodeKind {
    none,
    file,
    dir,
    unknown,
}

export declare enum RevisionKind {
    /** No revision information given */
    unspecified,
    /** revision given as number */
    number,
    /** revision given as date */
    date,
    /** rev of most recent change */
    committed,
    /** (rev of most recent change) - 1 */
    previous,
    /** .svn/entries current revision */
    base,
    /** current, plus local mods */
    working,
    /** repository youngest */
    head,
}

export declare enum StatusKind {
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

export declare function create_repos(path: string): void;
