import { Disposable } from "vscode";

class Subscriptions implements Disposable {
    private disposables: Set<Disposable> = new Set<Disposable>();

    public add<T extends Disposable>(item: T): T {
        this.disposables.add(item);
        return item;
    }

    public dispose() {
        for (const item of this.disposables)
            item.dispose();

        this.disposables.clear();
    }
}

export default new Subscriptions();
