{
    "targets": [
        {
            "target_name": "svn",
            "include_dirs": [
                "include/apr",
                "include/svn"
            ],
            "libraries": [
                "<(module_root_dir)/lib/win32_64/libapr_tsvn.lib",
                "<(module_root_dir)/lib/win32_64/libsvn_tsvn.lib"
            ],
            "sources": [
                "src/uv/async.hpp",
                "src/uv/semaphore.hpp",
                "src/export.cpp",
                "src/svn_error.hpp",
                "src/svn_error.cpp",
                "src/utils.hpp",
                "src/client.hpp",
                "src/client/auth/simple.hpp",
                "src/client/client.hpp",
                "src/client/init.cpp",
                "src/client/info.cpp",
                "src/client/new.cpp",
                "src/client/add.cpp",
                "src/client/cat.cpp",
                "src/client/changelistAdd.cpp",
                "src/client/changelistRemove.cpp",
                "src/client/checkout.cpp",
                "src/client/commit.cpp",
                "src/client/delete.cpp",
                "src/client/status.cpp",
                "src/client/revert.cpp",
                "src/client/update.cpp"
            ]
        }
    ]
}
