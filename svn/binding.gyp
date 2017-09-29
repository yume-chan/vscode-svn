{
    "targets": [
        {
            "target_name": "svn",
            "include_dirs": [
                "include/apr",
                "include/svn",
                "src"
            ],
            "libraries": [
                "<(module_root_dir)/lib/win32_64/libapr_tsvn.lib",
                "<(module_root_dir)/lib/win32_64/libsvn_tsvn.lib"
            ],
            "sources": [
                "src/apr/apr.hpp",
                "src/apr/array.cpp",
                "src/apr/array.hpp",
                "src/apr/pool.cpp",
                "src/apr/pool.hpp",
                "src/svn/client.cpp",
                "src/svn/client.hpp",
                "src/uv/async.hpp",
                "src/uv/semaphore.hpp",
                "src/node/export.cpp",
                "src/node/v8.hpp",
                "src/node/utils.hpp",
                "src/node/svn_error.hpp",
                "src/node/svn_error.cpp",
                "src/node/client.hpp",
                "src/node/client/auth/simple.hpp",
                "src/node/client/client.hpp",
                "src/node/client/init.cpp",
                "src/node/client/info.cpp",
                "src/node/client/new.cpp",
                "src/node/client/add.cpp",
                "src/node/client/cat.cpp",
                "src/node/client/changelistAdd.cpp",
                "src/node/client/changelistGet.cpp",
                "src/node/client/changelistRemove.cpp",
                "src/node/client/checkout.cpp",
                "src/node/client/commit.cpp",
                "src/node/client/delete.cpp",
                "src/node/client/status.cpp",
                "src/node/client/revert.cpp",
                "src/node/client/update.cpp"
            ]
        }
    ]
}
