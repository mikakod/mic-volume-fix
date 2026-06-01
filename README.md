# Mic Volume Fix

Mic Volume Fix is a lightweight Windows desktop app created to lock your specified devices microphone volume level and mute state. 
Prevent annoying Razer, Logitech, or Windows software updates from automatically turning down your headsets mic volume at random.

TL;dr it keeps your **microphone capture volume and mute state** pinned when other software (OEM drivers, Synapse, etc.) keeps resetting them.

Built with **C++**, **Qt 6**, and the **Windows Core Audio** API (WASAPI).

## Features

- List capture devices and lock the default mic or a specific endpoint
- Lock capture volume level and/or mute state
- Configurable poll interval and volume epsilon
- Background guardian thread that reconciles drift via Core Audio
- Session persistence under `%LOCALAPPDATA%\mic-volume-fix\`
- Optional run at Windows startup (current user, `HKCU\Run\MicVolumeFixGui`)
- Import and export settings as JSON


## Requirements

| Tool | Version |
|------|---------|
| Windows | 10 or 11 |
| MSVC | 2019+ (Visual Studio or Build Tools with **Desktop development with C++**) |
| CMake | 3.21+ |
| Qt | 6.5+ **Widgets**, **MSVC 2022 64-bit** kit |

## Quick start

### Download (recommended)

1. Open [Releases](../../releases)
2. Download `MicVolumeFix-win64.zip`
3. Extract the folder
4. Run `MicVolumeFix.exe`

Keep all files in the extracted folder together, the app needs the bundled Qt DLLs.

### Build from source

```powershell
git clone https://github.com/YOUR_USER/mic-volume-fix.git
cd mic-volume-fix
```

If Qt is not auto-detected under `C:\Qt`, copy the example path file and edit it:

```powershell
copy scripts\qt-path.bat.example scripts\qt-path.bat
```

Set your kit path, for example:

```bat
set CMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64
```

Build:

```bat
scripts\build.bat
```

Run:

```bat
build\Release\MicVolumeFix.exe
```

`build.bat` compiles Release, runs `windeployqt`, and copies Qt runtime DLLs next to the executable.

## Scripts

| Script | Description |
|--------|-------------|
| [`scripts/build.bat`](scripts/build.bat) | Configure, build, and deploy Qt DLLs |
| [`scripts/deploy.bat`](scripts/deploy.bat) | Run `windeployqt` on an existing build |
| [`scripts/package.bat`](scripts/package.bat) | Copy a portable folder to `dist/MicVolumeFix/` |

## Configuration

Settings are stored in:

```
%LOCALAPPDATA%\mic-volume-fix\mic-volume-fix_gui_session.json
```

You can also import/export a standalone `config.json`. See [`config.example.json`](config.example.json) for the format.

## Project layout

```
mic-volume-fix/
├── CMakeLists.txt
├── config.example.json
├── scripts/          # Windows build helpers
└── src/
    ├── audio/        # WASAPI / endpoint volume
    ├── config/       # JSON settings
    ├── guardian/     # Background reconcile loop
    ├── gui/          # Qt main window
    └── win/          # Startup registry helper
```

## License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE).


### Third-party

Release builds bundle **Qt 6** runtime libraries (via `windeployqt`). Qt is licensed separately under the [GNU Lesser General Public License (LGPL) v3](https://www.gnu.org/licenses/lgpl-3.0.html). See [Qt Licensing](https://www.qt.io/licensing/) for details.
