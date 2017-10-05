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
                "src/uv/future.hpp",
                "src/uv/semaphore.hpp",
                "src/node/export.cpp",
                "src/node/v8.hpp",
                "src/node/utils.hpp",
                "src/node/svn_error.hpp",
                "src/node/svn_error.cpp",
                "src/node/client.hpp",
                "src/node/client.cpp"
            ]
        }
    ]
}
