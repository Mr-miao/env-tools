{
  "targets": [
    {
      "target_name": "service_tool",
      "sources": [ "src/service_tool.cpp", "src/common.h", "src/utils.h", "src/utils.cpp"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS']
    },
    {
      "target_name": "startup_tool",
      "sources": [ "src/startup_tool.cpp", "src/common.h", "src/utils.h", "src/utils.cpp"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS']
    },
    {
           "target_name": "task_tool",
           "sources": [ "src/task_tool.cpp", "src/common.h", "src/utils.h", "src/utils.cpp"],
           "include_dirs": [
             "<!@(node -p \"require('node-addon-api').include\")"
           ],
           "libraries": [
                "E:/work/git_repo/env-tools/include/windowsSDK/x64/taskschd.lib",
                "-lkernel32",
                "-luser32",
                "-ladvapi32",
                "-lcomdlg32",
                "-lole32",
                "-loleaut32",
                "-luuid",
                "-lgdi32",
                "-lwinspool",
                "-lshell32"
           ],
           'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
           'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS']
    },
    {
          "target_name": "exec_tool",
          "sources": [ "src/exec_tool.cpp", "src/common.h", "src/utils.h", "src/utils.cpp"],
          "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")"
          ],
          'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
          'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS']
        },
  ]
}