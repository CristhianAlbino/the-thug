# UnrealProject — The Thug (M01 prototype)

Unreal Engine **5.8**, C++. The M01 slice (GDD §98, "10 excellent minutes") is fully code-driven:
the hospital wing, the Hunter, the sounds and the HUD are generated at runtime, so the only
binary asset is the empty map `Content/Maps/Hospital_M01.umap` (Git LFS).

## What is in M01

| System | Where |
|---|---|
| Joe: first person, sprint + stamina, crouch, flashlight, footstep noise, head bob | `TTPlayerCharacter` |
| Hunter: patrol, suspicion, hearing (approximate position only), investigate, search, pursue, capture, opens doors, locker memory (10/30/60/90) | `TTHunter` |
| Doors (crouch = slow and quiet), lockers with slats, keycard, bottles, service gate | `TTInteractables` |
| 4th-floor wing from an ASCII layout + grid path finding, lamps (some flicker), fog, post process | `TTHospital` |
| Procedural audio (Hunter steps, heartbeat, creaks, glass, drone) | `TTAudio` |
| Title / pause / captured / escaped flow, noise routing | `TTGameMode`, `TTHUD`, `TTPlayerController` |

Edit the map by changing `Layout` in `TTHospital.cpp` (legend in the comment above it).

## Controls

WASD move · Shift sprint · C toggle crouch / Ctrl hold crouch · F flashlight · E interact / hide · G or right mouse throw bottle · Esc pause (mouse sensitivity with + / −).

## Build

```powershell
& "C:\Users\Cristhian Albino\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" TheThugEditor Win64 Development "-Project=$PWD\TheThug.uproject" -WaitMutex
```

Requires Visual Studio 2022 Build Tools (C++ workload) **and the .NET Framework 4.8 SDK component**.
Close any open Unreal Editor first (Live Coding blocks command-line builds).

## Run without packaging

```powershell
& "C:\Users\Cristhian Albino\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\TheThug.uproject" -game -windowed -ResX=1600 -ResY=900
```

Debug flags: `-ttdebug` (Hunter state on screen), `-ttshots` (scripted screenshots to `Saved/Screenshots`, then quits),
`-ttai` (Joe waits in the corridor, logs Hunter states until capture), `-ttev=<bias>` (exposure), `-ttsens=<x>` (mouse).

## Package the .exe

```powershell
& "C:\Users\Cristhian Albino\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$PWD\TheThug.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive "-archivedirectory=$PWD\..\Build\Windows" -unattended
```

Output: `Build/Windows/Windows/TheThug.exe` (the `Build/` folder is git-ignored).

## Do not commit

`Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `*.log`.
