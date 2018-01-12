const { mkdirSync } = require("fs");
const { resolve } = require("path");

const vsce = require("vsce");

const package = require(resolve(__dirname, "..", "package.json"));

try {
    mkdirSync(resolve(__dirname, "..", "vsix"));
} catch (err) {
    if (err.code !== "EEXIST")
        throw err;
}

vsce.createVSIX({
    packagePath: resolve(__dirname, "..", "vsix", `${package.name}-${package.version}-${process.arch}.vsix`)
}).catch(function(err) {
    console.error(err);
    process.exit(-1);
});
