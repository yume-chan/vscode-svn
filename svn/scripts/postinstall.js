const fs = require("fs");
const path = require("path");

fs.readdirSync(path.resolve(__dirname, "../lib/win32_64"))
    .filter(x => x.endsWith(".dll"))
    .forEach(x => fs.linkSync(path.resolve(__dirname, "../lib/win32_64", x),
        path.resolve(__dirname, "../build", process.argv[2] || "Release", x)));
