# UnrealProject — The Thug (M01 prototype)

Unreal Engine **5.8**, C++. The M01 slice (GDD §98, "10 excellent minutes") is fully code-driven:
the hospital wing, the Hunter, the sounds and the HUD are generated at runtime, so the only
binary asset is the empty map `Content/Maps/Hospital_M01.umap` (Git LFS).

## What is in M01

| System | Where |
|---|---|
| Joe: first person, sprint + stamina, crouch, flashlight, footstep noise, head bob | `TTPlayerCharacter` |
| Hunter: patrol, suspicion, investigate, search, pursue, capture, opens and closes doors | `TTHunter` |
| Hunter memory and hearing **from `/Tools`**: `HunterCore` (observed routes, observed hiding spots 10/30/60/90, decay, difficulty tuning, weighted patrol choice), `PerceptionCore` (approximate hearing with uncertainty radius, occlusion), `EvidenceMemory` (doors he saw closed and later finds open) | `TTHunter` + `Core/` |
| Hunter adaptation (GDD §14, §15, §18): aggression rises when Joe escapes or fools him; blocks a door Joe keeps using (never one that would trap him), ambushes beside a learned doorway and kills the lights there | `TTHunter` |
| Difficulties Easy / Normal / Hard / Punitive (title screen 1-4) | `TTGameMode` |
| Joe's body and backpack **from `/Tools/SurvivalCore`**: injuries (light/moderate/severe), bleeding, collapse, death, stamina, slot-limited inventory; bandage stabilises, medkit treats one level | `TTPlayerCharacter` + `Core/` |
| Rifle (stuns the Hunter, gives Joe away, shoots out lamps), grenade (stuns, breaks lights, blows doors, hurts Joe if close), smoke (Hunter cannot see through it), bottles | `TTPlayerCharacter`, `TTInteractables` |
| Hunter blows instead of a capture on lower difficulties (Easy/Normal until Joe is badly hurt, Hard once, Punitive never); broken glass on the floor is loud and cuts when sprinting | `TTHunter`, `TTHospital` |
| Generated materials `M_TTSmoke` (lit translucent) and `M_TTEmissive` (glow) | `Scripts/create_materials.py` |
| Doors (crouch = slow and quiet), lockers with slats, keycard, bottles, service gate | `TTInteractables` |
| 4th-floor wing from an ASCII layout + grid path finding, lamps (some flicker), fog, post process | `TTHospital` |
| Procedural audio (Hunter steps, heartbeat, creaks, glass, drone) | `TTAudio` |
| Title / pause / captured / escaped flow, noise routing | `TTGameMode`, `TTHUD`, `TTPlayerController` |

Edit the map by changing `Layout` in `TTHospital.cpp` (legend in the comment above it).

`Source/TheThug/Core/*.cpp` compile `Tools/HunterCore` and `Tools/PerceptionCore` directly (single source of truth;
their standalone tests still run outside Unreal). The module builds with exceptions on and unity off for that reason.

## Controls

WASD move · Shift sprint · C toggle crouch / Ctrl hold crouch · F flashlight · E interact / hide · Right mouse aim · Left mouse fire · R reload · G bottle · T smoke · Q grenade · H bandage/medkit · hold Tab inventory (+ 2-7 drop) · Esc pause (mouse sensitivity with + / −).

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
`-ttai` (Joe waits in the corridor, logs Hunter states until capture),
`-ttlearn` (seeds route evidence for door 8,4 and aggression, hides Joe, logs traps/ambush),
`-ttsurvive` (injury, bandage, shot, smoke, grenade; logs and screenshots), `-ttdiff=<0-3>`, `-ttev=<bias>` (exposure), `-ttsens=<x>` (mouse).

## Package the .exe

```powershell
& "C:\Users\Cristhian Albino\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$PWD\TheThug.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive "-archivedirectory=$PWD\..\Build\Windows" -unattended
```

Output: `Build/Windows/TheThug.exe` (the `Build/` folder is git-ignored).

## Do not commit

`Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `*.log`.
