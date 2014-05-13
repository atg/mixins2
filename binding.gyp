{
  "targets": [
    {
      "target_name": "bindings",
      "sources": [ "src/bindings.cc", "src/mixins.cc" ],
      "conditions": [
        ['OS == "mac"', {
          "defines": [ "__MACOSX_CORE__" ],
          "include_dirs": [
              "<!(node -e \"require('nan')\")",
              "System/Library/Frameworks/CoreFoundation.Framework/Headers",
            ],
          "ldflags": [
            "-framework CoreFoundation",
            "-L<!(pwd)/build/Release"
          ],
          "xcode_settings": {
            "OTHER_LDFLAGS": [
              "-framework CoreFoundation"
            ]
          }
        }]
      ]
    }
  ]
}
