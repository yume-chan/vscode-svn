const list = [
    "../build/Debug/svn.node",
    "../build/Release/svn.node",
];

(function() {
    let message = "Cannot load native module:\n";

    for (const item of list) {
        try {
            module.exports = require(item);
            return;
        } catch (err) {
            message += `Tried ${item}: ${err}\n`;
            continue;
        }
    }

    throw new Error(message);
})();
