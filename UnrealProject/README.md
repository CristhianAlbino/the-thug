# UnrealProject — The Thug (prototype: M01–M08)

Unreal Engine **5.8**, C++. The M01 slice (GDD §98, "10 excellent minutes") is fully code-driven:
the hospital wing, the Hunter, the sounds and the HUD are generated at runtime, so the only
binary asset is the empty map `Content/Maps/Hospital_M01.umap` (Git LFS).

## What is in M01

| System | Where |
|---|---|
| Joe: first person, sprint + stamina, crouch, flashlight, footstep noise, head bob | `TTPlayerCharacter` |
| Hunter: patrol, suspicion, investigate, search, pursue, capture, opens and closes doors. With no lead he sweeps the whole hospital: each patrol leg stays on one floor, he finishes it before moving on and then goes to the floor he has neglected longest (a run down the stairwell does not count as searching a floor) | `TTHunter` |
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
| **Narrative (M07) from `/Tools/NarrativeCore`** (C++ port of the JSON contract, `Core/TTNarrativeCore.cpp`): playable opening on the 4th floor (N01 siege, N02 Bat-Signal at the observation window, N03 Thomas taken from above, the group scatters; `Enter` skips it, retries after a defeat skip it), five acts as narrative milestones with chapter cards, place/world beats N04–N34 played only when `Eligible()` says so, subtitles with speaker that Joe only hears when close, readable documents DOC06–DOC16 (Arkham hints, rules, routes), companion scenes when talking (Harley N07/N23 with Punitive reciprocity, Bartholomeu N15/N24, Jack N08/N25), radio intercepts | `TTNarrative` |
| Thomas (Hard/Punitive only): body in the B1 morgue + archived tape CAM_F4_WEST in the 2nd floor security room → N34; without them a coda shows the truth to the player only. Easy/Normal never spawn either | `TTNarrative` |
| Jack in Punitive: clothes change after the opening, J1/J2/J3 laughs that run too long, J4 reveal in act 5 (needs J3 + another clue) | `TTNarrative`, `TTCompanion` |
| Rare endings (GDD §59, RunRules): Easy **Confronto** (two following companions + the Hunter stunned), Punitive **Destruição** (explosives in the garage, three marked pillars, detonator in the tunnels). Robin's one-off episode in the Punitive tunnels (reports Joe, never captures) | `TTNarrative`, `RunRules` |
| Epilogues per person from verified evidence only (together / escaped alone / captured / unknown), Thomas block by difficulty, Jack's secret | `TTNarrative::OnRunEnded`, HUD |
| **Art (M08)**: procedural master material `M_TTSurface` (world-space tiles, grime, wet puddles; every block uses it) and a look per floor from the visual bible (linoleum 4th, white tile 3rd, carpet 2nd, terrazzo 1st, wet concrete B1, oily garage, rusty tunnels), lamp colours and colour grading per floor, red emergency lights on the 4th after N03, ceiling tiles, pipes/valves/cable trays on service floors, floor plates "4º ANDAR · SAÍDA" at every stairwell, room plates beside doors, clear windows (Bat-Signal visible) | `TTShapes`, `TTHospital`, `Scripts/create_surface_material.py` |
| Characters on the **UE5 Mannequin** with idle/walk/jog (`ABP_Unarmed`): the Hunter (2 m, charcoal suit, cape that trails with speed, cowl ears, belt, glowing eyes), Harley, Bartholomeu, Jack (outfit changes in Punitive), Thomas, Robin | `TTShapes::SetupMannequin` |
| **Horror Hospital kit** (Fab, free; not in the repo): the pack's floor/wall/ceiling materials on the hospital blocks, real beds with bedside tables, IV stands and clinic curtains, desks/shelves/cabinets/fridges/surgery machines by floor, double metal doors, notice boards/radiators/clocks/vents on walls, papers and rags on floors. Collision, navigation and the Hunter's sight stay on the code geometry | `TTKit`, `Scripts/build_kit.py` |
| Procedural audio (Hunter steps, heartbeat, creaks, glass, drone) | `TTAudio` |
| Title / pause / captured / escaped flow, noise routing | `TTGameMode`, `TTHUD`, `TTPlayerController` |

The map is authored in `Tools/MapCore/blockout_floors.py` (legend at the top). Edit it, run
`python Tools/MapCore/blockout_floors.py` to validate (widths, stairwells, hatch partners, every cell reachable from Joe's start),
then `python Tools/MapCore/blockout_floors.py --write` to regenerate `Source/TheThug/TTHospitalLayout.inl` and `TTHospitalZones.inl` (narrative zones and documents).
Narrative data: edit the JSON in `Tools/NarrativeCore`, then `python Tools/NarrativeCore/emit_cpp.py` (CI checks it is current).
Cells use one combined grid: floor N occupies columns `N*32 .. N*32+23`.

Companion starting profiles (trust, fear, stress, loyalty, self-preservation, relationship) are **balance hypotheses**
in `ProfileFor` (`TTCompanion.cpp`); the GDD defines their personalities but not the numbers.

`Source/TheThug/Core/*.cpp` compile `Tools/HunterCore` and `Tools/PerceptionCore` directly (single source of truth;
their standalone tests still run outside Unreal). The module builds with exceptions on and unity off for that reason.

## Controls

WASD move · Shift sprint · C toggle crouch / Ctrl hold crouch · F flashlight · E interact / hide · Right mouse aim · Left mouse fire · R reload · G bottle · T smoke · Q grenade · H bandage/medkit · hold Tab inventory, notes and clock (+ 2-9 drop) · safe keypad: 0-9 type, Backspace erase, Enter confirm, E/Esc leave · reading a document: E closes · Enter skips the opening · looking at a companion: E talk, Z give magazine, X come with me / stay · Esc pause (mouse sensitivity with + / −).

## Third-party art (local only)

The Mannequin and the Horror Hospital pack are **not committed** (licences, size): see `05_ART/ASSETS.md`.
Install "Templates and Feature Packs" and add *Horror Hospital* from Fab to the project in the Epic Launcher, then run
`powershell -ExecutionPolicy Bypass -File UnrealProject/Scripts/setup_art.ps1` (copies the Mannequin, runs `build_kit.py`,
which writes `Source/TheThug/TTKitAssets.inl` and the cook label). `build_kit.py --textures` (via `-run=pythonscript`
with `-script="build_kit.py --textures"`) caps the used textures at 2K, 40 per run on 8 GB RAM; repeat until 0 remain.
Without the assets the game builds and runs with the procedural art.

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
`-ttroute=garage|roof|tunnel|window` (plays one escape route end to end and logs each RunRules event and the ending), `-ttfastheli=<s>` (helicopter delay),
`-ttstory=intro|thomas|jack|destroy|confront|robin|art` (narrative: opening, N34, Jack J1–J4, rare endings, Robin; logs scenes and the epilogue, screenshots TT_30+), `-ttflash=<power>`, `-ttdiff=<0-3>`, `-ttev=<bias>` (exposure), `-ttsens=<x>` (mouse).

## Package the .exe

```powershell
& "C:\Users\Cristhian Albino\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$PWD\TheThug.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive "-archivedirectory=$PWD\..\Build\Windows" -unattended
```

Output: `Build/Windows/TheThug.exe` (the `Build/` folder is git-ignored).

## Do not commit

`Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `*.log`.
