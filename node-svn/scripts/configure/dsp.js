const path = require("path");
const fs = require("fs");

const root = path.resolve(__dirname, "../..");

/**
 * @param {string} folder
 * @param {string} file
 * @param {string} condition
 */
module.exports = function parseDsp(folder, file, condition) {
    const includes = [];
    const defines = []
    const sources = [];

    const project = path.resolve(root, folder);
    const content = fs.readFileSync(path.resolve(project, file), "utf-8");

    const index = content.match(`!(?:ELSE)?IF  "\\$\\(CFG\\)" == "${condition}"`).index;

    const line = content.substring(index).match(`# ADD CPP .*`)[0];
    const includeRegex = /\/I "(.*?)"/g;
    let match;
    while ((match = includeRegex.exec(line))) {
        const value = path.resolve(project, match[1]);
        includes.push(path.relative(root, value));
    }

    const defineRegex = /\/D "(.*?)"/g;
    while ((match = defineRegex.exec(line))) {
        defines.push(match[1]);
    }

    const sourceRegex = /SOURCE=(.*?\.c)/g;
    while ((match = sourceRegex.exec(content))) {
        const value = path.resolve(project, match[1]);
        sources.push(path.relative(root, value));
    }

    return { includes, defines, sources };
};
