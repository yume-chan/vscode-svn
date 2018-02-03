const { mkdirSync, writeFileSync } = require("fs");
const { resolve } = require("path");

const vsce = require("vsce");

// @ts-ignore
const package = require("../package.json");

try {
    mkdirSync(resolve(__dirname, "..", "vsix"));
} catch (err) {
    if (err.code !== "EEXIST")
        throw err;
}

const { platform, arch } = process;
console.log(`Building for ${platform} ${arch}`);
writeFileSync(resolve(__dirname, "../configuration.json"), JSON.stringify({
    platform,
    arch
}));

vsce.createVSIX({
    packagePath: resolve(__dirname, "..", "vsix", `${package.name}-${package.version}-${platform}-${arch}.vsix`)
}).catch(function(err) {
    console.error(err);
    process.exit(-1);
});
