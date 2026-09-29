# The Thug — Master Game Design Document

## 1. Vision

**The Thug** is a first-person survival horror / stealth game set primarily inside an abandoned Gotham hospital.

The player is not the hero and not the hunter. The player is the prey.

The central emotion is not simply fear of death; it is **fear of being found**.

### Core fantasy
> You are a criminal. You know you did something wrong. Now something far more dangerous is inside the building with you.

### Design pillars
1. Predator, not boss.
2. Information is a resource.
3. Survival above combat.
4. Tension above jumpscares.
5. Systems above scripts.

---

## 2. Premise

Joe Chill and four other criminals flee police after a bank robbery and hide on the fourth floor of an abandoned hospital.

From a window, Joe sees the Bat-Signal.

Soon after, Thomas Terrence is violently pulled into the darkness above the group by a massive bat-like figure. The survivors panic and scatter.

Gameplay begins with Joe alone inside the hospital.

### Initial group
- Joe Chill — protagonist
- Harley Quinzel — principal companion / emotional connection
- Bartholomeu Kent — frightened, pragmatic survivor
- Jack Oswald White — observant, suspicious survivor
- Thomas Terrence — first victim / inciting event

---

## 3. Player objective

The primary objective is:

> **Survive the Hunter.**

Possible victory states emerge naturally from play rather than being selected from a menu.

### Conventional victories
- Escape
- Hide successfully until the search is abandoned
- Survive until dawn where the difficulty permits it

### Escape routes
1. **Garage** — information-heavy, safer but laborious.
2. **Roof / helicopter** — radio assembly followed by a timed run.
3. **Tunnels** — dangerous labyrinthine route.
4. **Window / adjacent building** — stealth- and tool-driven escape.

### Exceptional endings
Rare, difficulty-specific routes may allow direct confrontation or destruction of the hospital. These are not the normal combat loop.

---

## 4. Target duration by difficulty

### Easy
~30 minutes.

### Normal
~1h30.

### Hard
~3 hours and the intended primary experience.

### Punitive
No guaranteed time-limit victory. The player must escape or cause the Hunter to abandon the search.

Duration comes from:
- exploration
- route choices
- dynamic events
- companion behavior
- Hunter behavior
- scarce resources
- information gathering
- multiple escape possibilities
- consequences
- unpredictable encounters

---

## 5. Hospital structure

```text
ROOF
 │
4TH FLOOR — START / CHAOS
 │
3RD FLOOR — CLINICAL
 │
2ND FLOOR — ADMINISTRATION / CAMERAS
 │
1ST FLOOR — EMERGENCY / MEDICAL STORAGE
 │
BASEMENT — TECHNICAL / GENERATORS / TUNNELS
 │
GARAGE
 │
EXTERIOR
```

The hospital should be dense and interconnected rather than artificially huge.

### 4th floor
- starting area
- wards
- patient rooms
- bathrooms
- nurse station
- side corridors
- first Hunter encounter

### 3rd floor
- surgery
- ICU
- recovery
- examination rooms
- wards
- technical access
- strong hiding opportunities

### 2nd floor
- administration
- archives
- security
- servers
- meeting rooms
- camera room
- storage

### 1st floor
- reception
- emergency
- waiting room
- pharmacy
- main medical storage
- external doors

### Basement
- generators
- maintenance
- pipes
- laundry
- morgue
- technical corridors
- hidden routes
- tunnel access

### Garage
- one main escape route
- vehicles
- dark sectors
- mechanical systems
- gate
- basement and tunnel links

---

## 6. Camera room

The camera room is a strategic information tool, not a free map.

It can:
- show selected corridors
- reveal movement
- help locate companions
- expose route changes
- reveal clues

Cameras may be:
- broken
- unpowered
- delayed
- noisy
- misaligned
- disabled
- manipulated by the Hunter

The room itself is not safe.

---

## 7. Core gameplay loop

```text
EXPLORE
  ↓
FIND INFORMATION
  ↓
PLAN
  ↓
MOVE
  ↓
PERCEIVE THREAT
  ↓
CHOOSE
 ├─ HIDE
 ├─ FLEE
 ├─ DISTRACT
 ├─ FIGHT
 └─ WAIT
  ↓
SURVIVE
  ↓
CONTINUE EXPLORING
```

Psychological loop:

```text
"I need to get there."
↓
"Is he nearby?"
↓
"I heard something."
↓
"I'll wait."
↓
"It's quiet..."
↓
"Did he leave?"
↓
"I'll move."
↓
"MERDA."
```

---

## 8. Exploration

Progression should often be driven by information rather than simple key collection.

Instead of:
> Find the key.

Prefer:
1. Discover that a key exists.
2. Learn where it is.
3. Learn how to reach it.
4. Decide whether the risk is worth it.

The hospital becomes a tool as the player gains spatial mastery.

---

## 9. Stealth

Player states:
```text
UNDETECTED
↓
SUSPICIOUS
↓
INVESTIGATED
↓
DETECTED
↓
PURSUED
```

Stealth considers:
- lighting
- noise
- movement
- posture
- exposure
- line of sight
- distance
- environmental cover

Darkness reduces detection but never grants invisibility.

---

## 10. Noise

Example intensity model:

```text
Still              0
Slow steps         Low
Walking            Low
Running            Medium
Door               Medium
Broken object      Medium/High
Fall               High
Gunshot            Extreme
Explosion          Extreme
```

The Hunter receives an approximate source, intensity, event type and time — not magical exact player coordinates.

---

## 11. Combat

Combat is a last resort.

Hard-mode baseline equipment:
- 1 rifle
- 2 magazines
- 1 fragmentation grenade
- 2 smoke grenades

Weapons can:
- create distance
- interrupt
- destroy obstacles
- create noise
- create escape windows

They must not turn the game into a conventional shooter.

---

## 12. Injuries

Three primary states:

### Light
Small movement and stamina penalty.

### Medium
Restricted running, heavier breathing, slower recovery.

### Severe
Very limited running, reduced mobility, possible bleeding.

Medical items stabilize or partially recover the player instead of magically resetting them.

---

## 13. Inventory

Small and physical.

Categories:
- weapon / ammunition
- explosives
- medical
- utilities
- information

Some items compete for limited carrying space, forcing decisions.

---

## 14. Hiding

Categories:
- physical hiding spots
- environmental concealment
- emergency hiding
- technical hiding

Possible variables:
- HideoutRisk
- HideoutVisibility
- HideoutNoise
- HideoutDiscoveryChance
- HunterSuspicion

No hiding spot is universally safe.

Rare safehouses can exist, but only a few.

---

## 15. Companions

Companions are independent gameplay systems, not followers.

Core variables:
```text
Trust
Fear
Stress
Loyalty
SelfPreservation
Relationship
Knowledge
Injury
Resources
CurrentGoal
```

They may:
- help
- lie
- steal
- distract
- save Joe
- abandon Joe
- betray Joe
- reveal Joe's location
- open or close routes
- die

Easy mode is the major exception: companions are reliable.

---

## 16. Hunter / Batman

The Hunter is a systemic predator.

Desired player feeling:
> "He knows more than I do."

### Core states
```text
PATROL
↓
AWARE
↓
SUSPICIOUS
↓
INVESTIGATING
↓
SEARCHING
↓
DETECTED
↓
PURSUING
↓
LOST_TARGET
↓
SEARCHING
↓
REPOSITIONING
↓
PATROL
```

Special states:
- OBSERVING
- AMBUSH
- INTERROGATING
- TRAP_SETUP
- RETREAT / REPOSITION

---

## 17. Hunter perception

### Vision considers
- distance
- field of view
- lighting
- obstacles
- player movement
- posture
- speed
- partial exposure

### Hearing receives
- approximate source
- intensity
- event type
- age of event

### Environmental evidence
- opened doors
- changed lights
- manipulated cameras
- moved objects
- blood
- companion activity
- previously investigated events

---

## 18. Hunter memory

```text
HunterMemory
├── LastPlayerLocation
├── LastNoise
├── KnownPlayerRoutes
├── UsedHidingSpots
├── NoiseHistory
├── EncounterHistory
├── PlayerHabits
├── CompanionLocations
├── SuspiciousAreas
├── UsedItems
└── EscapePatterns
```

No machine learning is required.

Use memory + scores + probabilities + adaptation.

Example:
```text
Locker A
1st use → suspicion 10
2nd use → suspicion 30
3rd use → suspicion 60
4th use → suspicion 90
```

The Hunter must never achieve perfect certainty purely from this system.

---

## 19. Hunter golden rule

> **The Hunter never cheats to look intelligent.**

It must not:
- know the player's exact location without perception
- see through solid geometry
- know a chosen hiding spot magically
- teleport purely to block the player
- always predict correctly

Intelligence comes from:
> Perception + memory + information + prediction + behavior.

The Hunter must also make mistakes.

---

## 20. Hunter adaptation

The Hunter can:
- alter patrol priorities
- investigate frequently used areas
- predict common escape routes
- reposition
- prepare ambushes
- manipulate doors
- disable cameras
- alter lighting
- block selected routes

Ambushes should be rare enough to remain surprising.

---

## 21. Decisions

Important decisions are systemic, not limited to dialogue menus.

Examples:
- share ammunition
- hide resources
- save someone
- abandon someone
- reveal a route
- trust information
- consume a rare resource
- create a distraction
- attempt an early escape
- remain hidden
- return for another character

Consequences may affect:
- trust
- loyalty
- resources
- available routes
- companion survival
- Hunter knowledge
- endings

---

## 22. Difficulty

### Easy
- short time
- permissive AI
- reliable companions
- permanent map
- abundant resources
- strong listening aid
- lower adaptation

### Normal
- moderate time
- mostly reliable companions
- map available
- camera room gives extra information
- moderate resources
- listening aid available
- stronger Hunter behavior

### Hard
Primary intended experience.
- ~3h target
- adaptive Hunter
- possible betrayals
- no permanent HUD map
- camera room is important
- codes and information matter
- scarce resources
- limited listening aid

### Punitive
- no map
- no listening aid
- extreme scarcity
- unreliable companions
- highly adaptive Hunter
- hiding spots degrade
- Robin and secret content possible
- no automatic dawn victory

---

## 23. Audio

Audio is gameplay.

Important sources:
- footsteps
- doors
- breathing
- pipes
- elevators
- rain
- wind
- generators
- radios
- Hunter equipment
- companions

The player may survive because they heard something before seeing it.

### Adaptive music states
- Exploration
- Tension
- Investigation
- Hunter Near
- Pursuit
- Hiding
- Recovery

Silence is part of the score.

---

## 24. Secondary narrative

Optional narrative expands the world without being mandatory.

Content:
- medical files
- security reports
- evacuation orders
- police documents
- old maps
- recordings
- phone messages
- hidden rooms
- old tunnels
- staff notes
- secrets

Some clues can unlock:
- codes
- safes
- routes
- camera access
- escape opportunities

---

## 25. Punitive secrets

Possible rare content:
- Robin
- Alfred communications
- Gordon communications
- extremely rare Flash reference/communication
- disguised Joker event
- Hunter observation evidence

These remain optional and rare.

---

## 26. Campaign arc

### Act 1 — Entry and rupture
Bank robbery aftermath, hospital entry, Bat-Signal, Thomas disappears, group separates.

### Act 2 — Understanding
Exploration, first resources, companion contact, camera discovery, first major Hunter encounters.

### Act 3 — Spatial mastery
More floors open, route knowledge grows, relationships evolve, the Hunter starts adapting.

### Act 4 — Pressure
Scarcity, injuries, betrayal, route changes and rising Hunter aggression.

### Act 5 — Emergent resolution
The player's accumulated actions push the run toward an available escape, hiding victory, dawn or secret ending.

---

## 27. First playable target

Do not build the full three-hour game first.

Build **10 excellent minutes** containing:
- first-person player
- movement
- camera
- flashlight
- corridor
- rooms
- one door
- one hiding spot
- lighting
- noise
- one Hunter
- vision
- hearing
- investigation
- pursuit
- ability to lose the Hunter
- small escape objective

If this small slice is frightening and fun, expand it.

---

## 28. Non-negotiable rules

- Do not turn the Hunter into a conventional enemy.
- Do not turn the game into a shooter.
- Do not give exact Hunter position for free.
- Do not overload the HUD.
- Do not rely on constant jumpscares.
- Do not show the Hunter constantly.
- Do not inflate duration with an oversized map.
- Do not let AI cheat.
- Do not make every hiding spot safe.
- Do not reduce companions to followers.
- Do not reduce decisions to dialogue menus.
- Do not make ammunition abundant.
- Do not make the camera room a perfect omniscient map.
- Do not force every run through identical events.

---

## 29. Identity statement

**The Thug** is a horror game about experiencing a legendary predator from the perspective of the person being hunted.

The player enters a hospital believing it is a hiding place.

Over time, they understand:

> It was never their hiding place.

> They walked into someone else's hunting ground.
