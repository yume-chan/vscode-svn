
const path = require("path");
const fs = require("fs");
const util = require("util");

const xml2js = require('xml2js');
const parseXml = util.promisify(xml2js.parseString);

const root = path.resolve(__dirname, "../..");

/**
 * @param {string} folder
 * @param {string} file
 * @param {string} condition
 */
module.exports = async function vcxproj(folder, file, condition) {
    const includes = [];
    const defines = []
    const sources = [];

    const project = path.resolve(root, folder);
    const content = fs.readFileSync(path.resolve(project, file), "utf-8");

    const { Project } = await parseXml(content);
    const configuration = Project.ItemDefinitionGroup.find(x => x.$.Condition === `'$(Configuration)|$(Platform)'=='${condition}'`).ClCompile[0];

    if (configuration.AdditionalIncludeDirectories !== undefined) {
        const directories = configuration.AdditionalIncludeDirectories[0].split(";");
        for (const item of directories) {
            if (item.match(/^%\(.*?\)$/))
                continue;

            const value = path.resolve(project, item);
            includes.push(path.relative(root, value));
        }
    }

    const definitions = configuration.PreprocessorDefinitions[0].split(";");
    for (const item of definitions) {
        if (item.match(/^%\(.*?\)$/))
            continue;

        defines.push(item);
    }

    const compiles = Project.ItemGroup.find(x => x.ClCompile !== undefined).ClCompile;
    for (const item of compiles) {
        const value = path.resolve(project, item.$.Include);

        if (value.endsWith("inffas8664.c"))
            continue;

        sources.push(path.relative(root, value));
    }

    return { includes, defines, sources };
};
