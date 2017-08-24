{
    "targets": [
        {
            "target_name": "svn",
            "include_dirs": [
                "src/svn/apr/include",
                "src/svn/svn/include"
            ],
            "libraries": [
                "<(module_root_dir)/src/svn/apr/release_x64/libapr_tsvn.lib",
                "<(module_root_dir)/src/svn/svn/release_x64/libsvn_tsvn.lib"
            ],
            "sources": [
                "src/svn/export.cc",
                "src/svn/client.cc"
            ]
        }
    ]
}
