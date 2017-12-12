import * as path from "path";

export default function isSamePath(a: string, b: string): boolean {
    return path.relative(a, b) === "";
}
