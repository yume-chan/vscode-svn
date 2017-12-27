import { Uri } from "vscode";

import { Revision } from "node-svn";

const revive: (data: any) => Uri = (Uri as any).revive;

export class SvnUri {
    public static parse(uri: Uri): SvnUri {
        const config = JSON.parse(uri.query);
        return new SvnUri(revive(config.file), config.revision);
    }

    public constructor(public readonly file: Uri, public readonly revision: Revision) { }

    public toUri(): Uri {
        return Uri.parse(`svn://conent/?${JSON.stringify(this)}`);
    }
}
