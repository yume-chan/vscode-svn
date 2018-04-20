import filetype from "file-type";
import path from "path";

import { Uri } from "vscode";

import {
    CatOptions,
    CatResult,
    Revision,
    RevisionKind,
} from "./node-svn";

import Client from "./client";
import { writeError, writeTrace } from "./output";

const revive: (data: any) => Uri = (Uri as any).revive;

export enum Encoding {
    UTF8 = "utf8",
    UTF16be = "utf16be",
    UTF16le = "utf16le",
}

export function detectUnicodeEncoding(buffer: Buffer): Encoding | null {
    if (buffer.length < 2)
        return null;

    const b0 = buffer.readUInt8(0);
    const b1 = buffer.readUInt8(1);

    if (b0 === 0xFE && b1 === 0xFF)
        return Encoding.UTF16be;

    if (b0 === 0xFF && b1 === 0xFE)
        return Encoding.UTF16le;

    if (buffer.length < 3)
        return null;

    const b2 = buffer.readUInt8(2);

    if (b0 === 0xEF && b1 === 0xBB && b2 === 0xBF)
        return Encoding.UTF8;

    return null;
}

const ImageMimetypes = [
    "image/png",
    "image/gif",
    "image/jpeg",
    "image/webp",
    "image/tiff",
    "image/bmp",
];

export class SvnUri {
    public static parse(uri: Uri): SvnUri {
        const config = JSON.parse(uri.query);
        return new SvnUri(revive(config.file), config.revision);
    }

    public static async provideResource(uri: Uri | SvnUri): Promise<CatResult> {
        let file: Uri;
        let revision: Revision;

        if (uri instanceof Uri) {
            try {
                ({ file, revision } = SvnUri.parse(uri));
            } catch (err) {
                writeError(`provideResource()`, new Error(`Can't parse Uri ${uri.toString()}`));
                throw err;
            }
        } else {
            ({ file, revision } = uri);
        }

        const fsPath = file.fsPath;
        const client = Client.get();
        try {
            const options: CatOptions = { peg_revision: RevisionKind.base, revision };
            const result = await client.cat(fsPath, options);

            writeTrace(`provideResource("${fsPath}")`, result.content.length);
            return result;
        } catch (err) {
            writeError(`provideResource("${fsPath}")`, err);
            throw err;
        } finally {
            Client.release(client);
        }
    }

    public constructor(public readonly file: Uri, public readonly revision: Revision) { }

    public toTextUri(): Uri {
        // keep file name to enable syntax highlighting
        return Uri.parse(`svn://content/${this.file.path}?${JSON.stringify(this)}`);
    }

    public async detectObjectType(): Promise<{ resource: CatResult, mimetype: string, encoding?: string }> {
        const resource = await SvnUri.provideResource(this);
        const buffer = resource.content.slice(0, 4096);

        const encoding = detectUnicodeEncoding(buffer);
        let isText = true;

        if (encoding !== Encoding.UTF16be && encoding !== Encoding.UTF16le) {
            for (let i = 0; i < buffer.length; i++) {
                if (buffer.readInt8(i) === 0) {
                    isText = false;
                    break;
                }
            }
        }

        if (!isText) {
            const result = filetype(buffer);

            if (!result)
                return { resource, mimetype: "application/octet-stream" };
            else
                return { resource, mimetype: result.mime };
        }

        if (encoding)
            return { resource, mimetype: "text/plain", encoding };
        else
            return { resource, mimetype: "text/plain" };
    }

    public async toResourceUri(): Promise<Uri> {
        const { resource, mimetype } = await this.detectObjectType();

        if (mimetype === "text/plain")
            return this.toTextUri();

        if (ImageMimetypes.indexOf(mimetype) > -1)
            return Uri.parse(`data:${mimetype};label:${path.basename(this.file.fsPath)};base64,${resource.content.toString("base64")}`);

        return Uri.parse(`data:;label:${path.basename(this.file.fsPath)},`);
    }
}
