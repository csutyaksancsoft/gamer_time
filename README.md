# Gamer Time

A networked SDL3/Vulkan arena client plus a headless authoritative Linux
server. Each player controls one avatar with WASD; the server replicates all
positions, while the client owns its camera and cosmetic fog of war.

## Windows prerequisites

The supported Windows toolchain is Visual Studio 2026 on 64-bit Windows 10 or
11. Install:

1. **Visual Studio 2026 Community** with the **Desktop development with C++**
   workload. Keep the MSVC v145 x64/x86 tools, Windows 11 SDK, and C++ CMake
   tools selected.
2. **CMake 4.2 or newer**. Select the installer option that adds CMake to the
   system `PATH`.
3. The latest **Windows x64 Vulkan SDK** from LunarG. Install the SDK, not just
   the runtime.

Git is not required when the project folder is obtained another way. The build
script downloads the exact SDL and ENet revisions used by the project when
either dependency is missing.

Players need only a Vulkan-capable GPU with a current graphics driver. They do
not need Visual Studio, the Vulkan SDK, or the Visual C++ Redistributable.

## Automated Windows build

Open **Developer PowerShell for VS 2026**, change to the project directory, and
allow the repository's local scripts for this PowerShell process:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
```

Build the Release client and portable ZIP:

```powershell
.\scripts\build-windows.ps1
```

The script:

- verifies CMake 4.2+, Visual Studio 2026 C++ tools, and the Vulkan SDK;
- downloads the pinned SDL source if necessary;
- configures a Visual Studio 2026 x64 build;
- compiles `gamer_time.exe` in Release mode; and
- creates the portable Windows ZIP.

Outputs:

```text
build-windows\Release\gamer_time.exe
build-windows\gamer-time-windows-x64.zip
```

Extract the ZIP and launch the client with a LAN server address and name:

```powershell
.\gamer_time.exe --server 192.168.1.10:27020 --name YourName
```

Add the real `assets\audio\song.wav` before packaging. Its BPM, first beat,
subdivision, and duration are configured in `assets\audio\song.cfg`.

## Linux LAN server

Install a C++ compiler, CMake, and curl. SDL, Vulkan, and Git are not required:

```bash
bash scripts/build-server.sh
bash scripts/run-lan-server.sh
```

The server terminal accepts `status`, `start`, `stop`, `kick ID`, and `quit`.
`start` schedules the shared song three seconds in the future.

## Automatic BPM and beat-offset detection

Install the offline analyzer once:

```bash
python -m pip install essentia
```

Then analyze a song and update `assets/audio/song.cfg` automatically:

```bash
python detect_bpm.py --wav MEMECAR-001.wav
```

Use `--dry-run` to inspect the detected BPM, first-beat offset, duration, and
confidence without changing the configuration. The detector normalizes
half/double-tempo results into the 90–180 BPM gameplay range by default.

Run protocol/load-test clients from another terminal:

```bash
./build-server/gamer_time_bot --server 127.0.0.1:27020 --count 64
```

### Script options

Build a Debug client and ZIP:

```powershell
.\scripts\build-windows.ps1 -Configuration Debug
```

Build without creating the ZIP:

```powershell
.\scripts\build-windows.ps1 -SkipPackage
```

Use a different build directory:

```powershell
.\scripts\build-windows.ps1 -BuildDirectory out\windows
```

## Manual Windows build

The equivalent commands are:

```powershell
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64
cmake --build build-windows --config Release
cmake --build build-windows --config Release --target package
```

Shader sources are compiled into the build directory with the Vulkan SDK's
`glslangValidator`. Tracked shader files are never rewritten during a build.
