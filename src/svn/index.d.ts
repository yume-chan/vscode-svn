declare module "svn" {
    import Buffer = require("buffer");

    export interface SvnVersion {
        major: number;
        minor: number;
        patch: number;
    }

    export function version(): SvnVersion;

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

    export interface SvnStatusResult {
        status: SvnStatus[];
        version: number;
    }

    export class Client {
        public constructor();

        public status(path: string): SvnStatusResult;
        public cat(path: string): Buffer;
    }
}
