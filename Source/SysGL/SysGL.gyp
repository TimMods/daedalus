 {
    'includes': [
      '../common.gypi',
    ],
    'targets': [
      {
        'target_name': 'SysGL',
        'type': 'static_library',
        'include_dirs': [
          '../',
        ],
        'dependencies': [
          '../third_party/glew/glew.gyp:glew',
          '../third_party/glfw/glfw.gyp:glfw',
          '../third_party/libpng/libpng.gyp:libpng',
        ],
        'sources': [
          'Graphics/GraphicsContextGL.cpp',
          'Graphics/NativeTextureGL.cpp',
          'HLEGraphics/GraphicsPluginGL.cpp',
          'HLEGraphics/RendererGL.cpp',
          'Interface/UI.cpp',
        ],
        'conditions': [
          ['OS=="win"', {
            'include_dirs': [
              '../SysW32/Include',
            ],
          }],
          ['OS=="mac"', {
            'include_dirs': [
              '../SysOSX/Include',
            ],
          }],
          ['OS=="linux"', {
            'include_dirs': [
              '../SysLinux/Include',
            ],
          }],
        ],
        'copies': [
          {
            'destination': '<(PRODUCT_DIR)/',
            'files': [
              '../SysGL/HLEGraphics/n64.psh',
            ],
          },
        ],
      },
    ],
  }
