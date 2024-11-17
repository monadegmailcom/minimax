# build (unsigned) dmg file

- generate `icon.icns` file with
```
rsvg-convert -h 16 tic-tac-toe.svg > icon.iconset/icon-16.png
% rsvg-convert -h 32 tic-tac-toe.svg > icon.iconset/icon-32.png
% rsvg-convert -h 64 tic-tac-toe.svg > icon.iconset/icon-64.png
% rsvg-convert -h 128 tic-tac-toe.svg > icon.iconset/icon-128.png
% rsvg-convert -h 256 tic-tac-toe.svg > icon.iconset/icon-256.png
% rsvg-convert -h 512 tic-tac-toe.svg > icon.iconset/icon-512.png
% rsvg-convert -h 1024 tic-tac-toe.svg > icon.iconset/icon-1024.png
% iconutil -c icns icon.iconset 
```

- create `Info.plist` file
```
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleIdentifier</key>
    <string>com.tschochner.Minimax</string>
    <key>CFBundleName</key>
    <string>Minimax</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>CFBundleIconFile</key>
    <string>icon</string>
</dict>
</plist>
```

- create this folder structure
``` 
Minimax.app
└── Contents
    ├── Info.plist
    ├── MacOS
    │   └── minimax
    └── Resources
        └── icon.icns
```

- create `Minimax.dmg` file (does not work yet, does not unpack on target machine)
```
% hdiutil create -volname Minimax -srcfolder Minimax.app -ov -format UDZO Minimax.dmg
```

- or create zip archive
```
% zip -r minimax.zip Minimax.app
```

