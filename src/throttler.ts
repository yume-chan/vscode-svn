export class Throttler {
    public static delay(timeout: number): Promise<void> {
        return new Promise((resolve) => setTimeout(resolve, timeout));
    }

    private id: number = 0;
    private working: boolean = false;
    private notify: Set<() => void> = new Set();

    public constructor(public method: () => Promise<void>, public timeout: number) {

    }

    public async run(): Promise<void> {
        if (this.working) {
            return await new Promise<void>((resolve) => {
                this.notify.add(resolve);
            });
        }

        const myId = this.id + 1;
        this.id = myId;

        await Throttler.delay(this.timeout);

        if (this.id !== myId) {
            return await new Promise<void>((resolve) => {
                this.notify.add(resolve);
            });
        }

        await this.method();

        for (const item of this.notify)
            item();
        this.notify.clear();
    }
}
