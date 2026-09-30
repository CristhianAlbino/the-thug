# The Thug

> **You are not the hero. You are not the hunter. You are the prey.**

**The Thug** is a first-person survival horror / stealth game planned for Unreal Engine 5.

The player controls a Gotham criminal trapped inside an abandoned hospital while a highly intelligent predator hunts, studies, and adapts to the player's behavior.

## Project status

**Pre-Production — M00**

- Engine: Unreal Engine 5
- Core: C++ + Blueprint
- Genre: Survival Horror / Stealth
- Perspective: First Person
- Primary setting: Abandoned hospital
- Target experience: tension, paranoia, vulnerability, emergent decisions and systemic pursuit

## Design pillars

1. **Predator, not boss** — the Hunter is a systemic threat, not a conventional enemy.
2. **Information is a resource** — learning the hospital is as valuable as ammunition.
3. **Survival above combat** — tools create opportunities; they do not grant dominance.
4. **Tension above jumpscares** — uncertainty, sound and anticipation carry the horror.
5. **Systems above scripts** — AI, companions, resources and level design should interact.

## Repository structure

```text
01_PROJECT/       Production, roadmap, milestones and decisions
02_GAME_DESIGN/   GDD, mechanics, systems, levels and difficulty
03_NARRATIVE/     Story, characters, dialogue, endings and lore
04_TECHNICAL/     Architecture, AI, C++, Blueprint and performance
05_ART/           Visual direction and asset planning
06_AUDIO/         Music, SFX, ambience and audio systems
07_LEVELS/        Level-design documentation
08_PROTOTYPES/    Prototype notes and experiments
09_TESTING/       Test plans, QA and performance
10_BUILD/         Build and release documentation
Tools/            Project utilities
UnrealProject/    Unreal Engine project root (to be created in the editor)
```

## Development philosophy

Build **10 excellent minutes first**. Prove that walking through a corridor while the Hunter searches for the player is already tense and fun. Only then expand the systems into the full hospital.

## Milestones

- M00 — Pre-Production
- M01 — Core Prototype
- M02 — Hunter
- M03 — Stealth
- M04 — Companions
- M05 — Hospital Blockout
- M06 — Vertical Slice
- M07 — Alpha
- M08 — Beta
- M09 — Release

## Engine-independent systems

The [C++17 game rules](Tools/HunterCore/README.md) now implement a testable Hunter decision core and campaign escape conditions. Their [integration contract](04_TECHNICAL/Architecture/CONTRATO_SISTEMAS.md) defines what the Unreal project must provide. The [CompanionCore](Tools/CompanionCore/README.md) adds individual companion knowledge, relationships and conditional goals. [SurvivalCore](Tools/SurvivalCore/README.md) covers injuries, bleeding, stamina and a capacity-limited inventory. [PerceptionCore](Tools/PerceptionCore/README.md) turns sounds and inspected environmental changes into fair, uncertain Hunter observations. These modules compile outside Unreal; no editor integration has been claimed.

## Current next step

Follow the [24-day pre-Unreal plan](01_PROJECT/Roadmap/PLANO_PRE_UNREAL_24_DIAS.md) through October 23, 2026. The [Day 1 Unreal setup](01_PROJECT/Roadmap/DIA_1_UNREAL.md) applies once the editor is available. Open design questions are tracked in [Decisions](01_PROJECT/Decisions/DECISOES_ABERTAS.md). The [complete master GDD](02_GAME_DESIGN/GDD/GDD_MASTER.md) is the design source of truth. GitHub updates require a commit and push; editor saves are local.

---
**Working title:** The Thug
