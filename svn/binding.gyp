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
                "src/client.cpp",
                "src/utils.cpp"
            ]
        }
    ]
}
