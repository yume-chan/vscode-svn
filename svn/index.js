// @ts-ignore

try {
    module.exports = require("./build/Debug/svn.node");
} catch (err) {
    module.exports = require("./build/Release/svn.node");
}
