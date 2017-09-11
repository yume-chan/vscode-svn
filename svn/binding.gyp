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
                "src/export.cpp",
                "src/svn_error.cpp",
                "src/utils.cpp",
                "src/client/init.cpp",
                "src/client/new.cpp",
                "src/client/add.cpp",
                "src/client/cat.cpp",
                "src/client/checkout.cpp",
                "src/client/commit.cpp",
                "src/client/status.cpp",
                "src/client/revert.cpp",
                "src/client/update.cpp"
            ]
        }
    ]
}
