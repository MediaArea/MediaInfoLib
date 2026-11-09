{
  'targets': [
    {
        'target_name': 'mediainfolib',
        'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS', 'UNICODE', '_UNICODE' ],
        'sources': [
            'Source/Helpers.cpp',
            'Source/NodeMediaInfo.cpp',
            'Source/NodeMediaInfoList.cpp',
            'Source/Lib.cpp'
        ],
        'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")"],
        'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
    }
  ]
}
