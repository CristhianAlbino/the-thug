# UnrealProject — The Thug (prototype: M01–M06)

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
| Companions Harley, Bartholomeu, Jack **from `/Tools/CompanionCore`**: own perception and knowledge, the core chooses hide / search resources / help Joe / share route / escape alone / use Joe as a distraction; ammo handoff through `/Tools/IntegrationCore` (+20 trust, +10 loyalty, +1 ammo); abandonment when Joe watches the Hunter chase them and does nothing | `TTCompanion` + `Core/` |
| Hunter vs companions: spots, chases, takes them (also out of lockers) and interrogates them; they only give away what they actually saw | `TTHunter` |
| Window route (GDD §56): boarded window, crowbar, loud prying; a companion escaping through it leaves it open for Joe | `TTInteractables` (`ATTWindow`) |
| Doors (crouch = slow and quiet), lockers with slats, keycard, bottles, service gate | `TTInteractables` |
| **Whole hospital (M05 blockout)**: garage, B1, floors 1-4 and roof stacked, two switchback stairwells (real ramps), service shaft hatches Joe can crawl through (the Hunter cannot), security camera room with night-vision feeds (some dead), heavy-door isolation room the Hunter only enters after seeing Joe there, elevator (no power yet), city skyline and moonlit roof, lights only on the floors around the view | `TTHospital` + `TTHospitalLayout.inl` |
| **Escape routes (M06) from `/Tools/HunterCore/RunRules`**: every step is a RunRules event and the ending comes from it. **Garage**: three code clues (4th floor nurse, 2nd floor archive, B1) → safe on the 2nd floor (keypad, random code each run) → garage key → generator in B1 (power: garage lights, elevator) → gate control → gate. **Roof**: radio on the 1st floor needs batteries (2nd floor servers) → interference panel at the cameras → call the boss → helicopter arrives (countdown on screen), lands on the helipad, is loud and leaves if Joe takes too long. **Tunnels**: grate in B1 (or the garage) opens the storm drain maze one floor below, exit at the far end; the Hunter climbs the ladders after Joe. **Window**: crowbar and prying; if the Hunter sees it open he seals it. Night clock (00:00 → 03:30 dawn ends the run), hiding in the isolation room long enough also ends it, elevator rides when powered | `TTGameMode` (run section), `TTInteractables` (`ATTStation`, `ATTHelicopter`, `ATTExitGate`) |
| Procedural audio (Hunter steps, heartbeat, creaks, glass, drone) | `TTAudio` |
| Title / pause / captured / escaped flow, noise routing | `TTGameMode`, `TTHUD`, `TTPlayerController` |

The map is authored in `Tools/MapCore/blockout_floors.py` (legend at the top). Edit it, run
`python Tools/MapCore/blockout_floors.py` to validate (widths, stairwells, hatch partners, every cell reachable from Joe's start),
then `python Tools/MapCore/blockout_floors.py --emit` and paste the output into `Source/TheThug/TTHospitalLayout.inl`.
Cells use one combined grid: floor N occupies columns `N*32 .. N*32+23`.

Companion starting profiles (trust, fear, stress, loyalty, self-preservation, relationship) are **balance hypotheses**
in `ProfileFor` (`TTCompanion.cpp`); the GDD defines their personalities but not the numbers.

`Source/TheThug/Core/*.cpp` compile `Tools/HunterCore` and `Tools/PerceptionCore` directly (single source of truth;
their standalone tests still run outside Unreal). The module builds with exceptions on and unity off for that reason.

## Controls

WASD move · Shift sprint · C toggle crouch / Ctrl hold crouch · F flashlight · E interact / hide · Right mouse aim · Left mouse fire · R reload · G bottle · T smoke · Q grenade · H bandage/medkit · hold Tab inventory, notes and clock (+ 2-9 drop) · safe keypad: 0-9 type, Backspace erase, Enter confirm, E/Esc leave · looking at a companion: E talk, Z give magazine, X come with me / stay · Esc pause (mouse sensitivity with + / −).

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
`-ttsurvive` (injury, bandage, shot, smoke, grenade; logs and screenshots),
`-ttcomp` (talk, ammo handoff, Hunter takes a companion, Jack escapes through the window, Joe follows),
`-ttfloors` (service hatch crawl, stairwell, camera monitors, garage; screenshots),
`-ttroute=garage|roof|tunnel|window` (plays one escape route end to end and logs each RunRules event and the ending), `-ttfastheli=<s>` (helicopter delay), `-ttflash=<power>`, `-ttdiff=<0-3>`, `-ttev=<bias>` (exposure), `-ttsens=<x>` (mouse).

## Package the .exe

```powershell
& "C:\Users\Cristhian Albino\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$PWD\TheThug.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive "-archivedirectory=$PWD\..\Build\Windows" -unattended
```

Output: `Build/Windows/TheThug.exe` (the `Build/` folder is git-ignored).

## Do not commit

`Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `*.log`.
