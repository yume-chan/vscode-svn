import {
    CancellationToken,
    DecorationData,
    DecorationProvider,
    Disposable,
    Event,
    EventEmitter,
    Uri,
    window,
} from "vscode";

import subscriptions from "./subscriptions";
import { SvnSourceControl } from "./source-control";

const enableProposedApi = false;

export class SvnDecorationProvider implements DecorationProvider, Disposable {
    private readonly onDidChangeDecorationsEvent: EventEmitter<Uri[]> = new EventEmitter<Uri[]>();

    private readonly disposable: Set<Disposable> = new Set();

    public readonly onDidChangeDecorations: Event<Uri[]> = this.onDidChangeDecorationsEvent.event;

    public constructor() {
        if (enableProposedApi)
            this.disposable.add(window.registerDecorationProvider(this));
    }

    public provideDecoration(uri: Uri, token: CancellationToken): DecorationData | undefined {
        const state = SvnSourceControl.cache.get(uri.fsPath);
        if (state === undefined)
            return undefined;
        return state.decorations;
    }

    public onDidChangeFiles(files: Uri[]): void {
        this.onDidChangeDecorationsEvent.fire(files);
    }

    public dispose(): void {
        for (const item of this.disposable)
            item.dispose();

        this.disposable.clear();
    }
}

export default subscriptions.add(new SvnDecorationProvider());
