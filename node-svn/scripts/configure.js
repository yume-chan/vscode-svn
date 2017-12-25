const path = require("path");
const fs = require("fs");
const util = require("util");

const vcxproj = require("./configure/vcxproj");
const dsp = require("./configure/dsp");

function find(folder, ext) {
    const result = [];
    for (const name of fs.readdirSync(folder)) {
        const full = path.resolve(folder, name);
        if (fs.statSync(full).isDirectory()) {
            result.push(...find(full, ext));
        } else if (path.extname(full) === ext) {
            result.push(full);
        }
    }
    return result;
}

const platform = process.argv[2] || process.platform;
const arch = process.argv[3] || process.arch;
const root = path.resolve(__dirname, "..");

try {
    fs.mkdirSync("dependencies/include");
} catch (err) { }

function configure_expat() {
    return vcxproj("dependencies/libexpat/expat/lib", "expat_static.vcxproj", "Release|Win32");
}

function configure_apr() {
    fs.copyFileSync(path.resolve(root, "dependencies/apr-gen-test-char", platform, "apr_escape_test_char.h"), path.resolve(root, "dependencies/include/apr_escape_test_char.h"));

    const h = path.resolve(root, "dependencies/include/apr.h");

    // Modify APR_HAVE_IPV6 to 0
    // Or it cannot find `IF_NAMESIZE`
    fs.copyFileSync(path.resolve(root, "dependencies/apr", "include/apr.hw"), h);
    let content = fs.readFileSync(h, "utf-8");
    content = content.replace(/APR_HAVE_IPV6\s+1/, "APR_HAVE_IPV6 0");
    fs.writeFileSync(h, content);

    fs.copyFileSync(path.resolve(root, "dependencies/apr", "include/apu_want.hw"), path.resolve(root, "dependencies/include/apu_want.h"));
    fs.copyFileSync(path.resolve(root, "dependencies/apr", "include/private/apu_select_dbm.hw"), path.resolve(root, "dependencies/include/apu_select_dbm.h"));
    return dsp("dependencies/apr", "apr.dsp", `apr - ${arch === "x64" ? "x64" : "Win32"} Release`);
}

function configure_apr_iconv() {
    return dsp("dependencies/apr-iconv", "apriconv.dsp", `apriconv - ${arch === "x64" ? "x64" : "Win32"} Release`);
}

function configure_svn(name) {
    fs.copyFileSync(path.resolve(root, "dependencies/subversion", "subversion/svn_private_config.hw"), path.resolve(root, "dependencies/include/svn_private_config.h"));

    const svn = "dependencies/subversion/build/win32/vcnet-vcproj";
    return vcxproj(svn, `libsvn_${name}.vcxproj`, `Release|${arch === "x64" ? "x64" : "Win32"}`);
}

async function main() {
    const expat = await configure_expat();
    expat.includes.push("dependencies/libexpat/expat/lib");

    const apr = configure_apr();
    apr.includes.push("dependencies/include");

    const apr_iconv = configure_apr_iconv();

    const client = await configure_svn("client");
    const diff = await configure_svn("diff");
    const delta = await configure_svn("delta");
    const svn_fs = await configure_svn("fs");
    const fs_util = await configure_svn("fs_util");
    const fs_fs = await configure_svn("fs_fs");
    const fs_x = await configure_svn("fs_x");
    const ra = await configure_svn("ra");
    const ra_local = await configure_svn("ra_local");
    const ra_svn = await configure_svn("ra_svn");
    const repos = await configure_svn("repos");
    const subr = await configure_svn("subr");
    subr.defines.push("XML_STATIC");
    const wc = await configure_svn("wc");

    const configuration = {
        "targets": [
            {
                "target_name": "apr",
                "type": "static_library",
                "include_dirs": apr.includes.concat(expat.includes),
                "defines": apr.defines,
                "sources": apr.sources
            },
            {
                "target_name": "apr-iconv",
                "type": "static_library",
                "include_dirs": apr_iconv.includes.concat(apr.includes),
                "defines": apr_iconv.defines,
                "sources": apr_iconv.sources
            },
            {
                "target_name": "expat",
                "type": "static_library",
                "include_dirs": expat.includes,
                "defines": expat.defines,
                "sources": expat.sources
            },
            {
                "target_name": "libsvn_client",
                "type": "static_library",
                "include_dirs": client.includes.concat(apr.includes),
                "defines": client.defines,
                "sources": client.sources
            },
            {
                "target_name": "libsvn_diff",
                "type": "static_library",
                "include_dirs": diff.includes.concat(apr.includes),
                "defines": diff.defines,
                "sources": diff.sources
            },
            {
                "target_name": "libsvn_delta",
                "type": "static_library",
                "include_dirs": delta.includes.concat(apr.includes),
                "defines": delta.defines,
                "sources": delta.sources
            },
            {
                "target_name": "libsvn_fs",
                "type": "static_library",
                "include_dirs": svn_fs.includes.concat(apr.includes),
                "defines": svn_fs.defines,
                "sources": svn_fs.sources
            },
            {
                "target_name": "libsvn_fs_fs",
                "type": "static_library",
                "include_dirs": fs_fs.includes.concat(apr.includes),
                "defines": fs_fs.defines,
                "sources": fs_fs.sources
            },
            {
                "target_name": "libsvn_fs_util",
                "type": "static_library",
                "include_dirs": fs_util.includes.concat(apr.includes),
                "defines": fs_util.defines,
                "sources": fs_util.sources
            },
            {
                "target_name": "libsvn_fs_x",
                "type": "static_library",
                "include_dirs": fs_x.includes.concat(apr.includes),
                "defines": fs_x.defines,
                "sources": fs_x.sources
            },
            {
                "target_name": "libsvn_ra",
                "type": "static_library",
                "include_dirs": ra.includes.concat(apr.includes),
                "defines": ra.defines,
                "sources": ra.sources
            },
            {
                "target_name": "libsvn_ra_local",
                "type": "static_library",
                "include_dirs": ra_local.includes.concat(apr.includes),
                "defines": ra_local.defines,
                "sources": ra_local.sources
            },
            {
                "target_name": "libsvn_ra_svn",
                "type": "static_library",
                "include_dirs": ra_svn.includes.concat(apr.includes),
                "defines": ra_svn.defines,
                "sources": ra_svn.sources
            },
            {
                "target_name": "libsvn_repos",
                "type": "static_library",
                "include_dirs": repos.includes.concat(apr.includes),
                "defines": repos.defines,
                "sources": repos.sources
            },
            {
                "target_name": "libsvn_subr",
                "type": "static_library",
                "include_dirs": subr.includes.concat(expat.includes, apr.includes),
                "defines": subr.defines,
                "sources": subr.sources
            },
            {
                "target_name": "libsvn_wc",
                "type": "static_library",
                "include_dirs": wc.includes.concat(apr.includes),
                "defines": wc.defines,
                "sources": wc.sources
            },
            {
                "target_name": "svn",
                "dependencies": [
                    "apr",
                    "expat",
                    "libsvn_client",
                    "libsvn_diff",
                    "libsvn_delta",
                    "libsvn_fs",
                    "libsvn_fs_fs",
                    "libsvn_fs_util",
                    "libsvn_fs_x",
                    "libsvn_ra",
                    "libsvn_ra_local",
                    "libsvn_ra_svn",
                    "libsvn_repos",
                    "libsvn_subr",
                    "libsvn_wc",
                ],
                "include_dirs": [
                    ...apr.includes,
                    ...client.includes,
                    "src"
                ],
                "defines": [
                    "APR_DECLARE_STATIC"
                ],
                "sources": [
                    "src/cpp/client.cpp",
                    "src/cpp/svn_error.cpp",
                    "src/node/async_client.cpp",
                    "src/node/depth.cpp",
                    "src/node/export.cpp",
                    "src/node/node_client.cpp",
                    "src/node/node_kind.cpp",
                    "src/node/revision_kind.cpp",
                    "src/node/status_kind.cpp"
                ],
                "libraries": [
                    "ws2_32.lib",
                    "Mincore.lib"
                ],
                "cflags_cc": [
                    "-std=gnu++17",
                    "-fexceptions"
                ],
                "cflags_cc!": [
                    "-fno-rtti"
                ],
                "xcode_settings": {
                    "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                    "CLANG_CXX_LIBRARY": "libc++",
                    "MACOSX_DEPLOYMENT_TARGET": "10.7"
                },
                "msvs_settings": {
                    "VCCLCompilerTool": {
                        "AdditionalOptions": [
                            "/std:c++17"
                        ],
                        "DisableSpecificWarnings": [
                            "4005"
                        ],
                        "ExceptionHandling": 1
                    }
                }
            }
        ]
    };

    fs.writeFileSync("binding.gyp", JSON.stringify(configuration, undefined, 4));
}
main();
