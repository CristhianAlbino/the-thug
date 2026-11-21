"""Playable blockout of the hospital (M05) for the Unreal prototype.

Each floor is a 24x15 ASCII grid stacked vertically. This file is the authoring source;
`python blockout_floors.py` validates the layouts; `--write` regenerates
UnrealProject/Source/TheThug/TTHospitalLayout.inl and TTHospitalZones.inl (`--emit` prints the floors).

Legend (see also TTHospital.cpp):
  #  wall               .  floor              D  door               L  locker
  *  lamp               ~  flickering lamp    m  bed                t  desk / table / shelf
  r  car                h  roof machinery     S  stairwell (2x4 switchback, resolved per floor)
  V  service shaft hatch (F4..B1)             v  service hatch B1 <-> garage
  b  bottle  a bandage  M medkit  n magazine  s smoke  x grenade  c crowbar  K keycard
  g  broken glass       W  boarded window (exit)                 X  exit gate
  E  elevator door (no power yet)             I  heavy isolation door    i  isolation floor
  @  security camera    =  camera console     w  sealed window (city view)
  P  Joe                H  Hunter             Y Harley  U Bartholomeu  J Jack
  1-9  Hunter patrol points
  Escape routes (MapCore actions):
  A/B/C  safe code clue (F4 nurse / F2 archive / B1 generators)    N  guard log (F1: a garage key exists)
  O  security office note (F2: the key is in the safe)              Z  safe (F2)
  G  generator switch (B1: restores power)                          q  garage gate control (G)
  R  radio (F1)   F  batteries (F2 servers)   Q  interference panel (F2 cameras)   T  helipad (roof)
  %  tunnel grate / ladder (B1 and G down to the tunnels)          !  storm drain exit (tunnels)
  Narrative (M07):
  d  document (id in DOCS below)    o  morgue slab (Thomas, Hard/Punitive only)
  e  explosives  k  charge point  j  detonator  (Punitive "Destruicao" ending only)

Stairwells: west at columns 1-2, east at columns 21-22, rows 1-4 on every floor.
Even floors (G, F1, F3, R) leave the stairwell through row 4; odd floors (B1, F2, F4) through row 1.
"""

import sys

W, H = 24, 15

FLOORS = [
    # (id, display name, outdoor, z (cm), has stairwells, rows)
    ("G", "Garagem", False, -800, True, [
        "########################",
        "#SS..~....r.r.r......SS#",
        "#SS.r.r.r.........r..SS#",
        "#SS.............1....SS#",
        "#SS...r.r.r.r.r......SS#",
        "##.......*...........###",
        "#t.r.r..........r.r..t.#",
        "#t..........2.........v#",
        "#..r.r.r.*...r.r.r.r...#",
        "#...........k.........t#",
        "###D######..######D#####",
        "#tet.#..............#.t#",
        "#n%..D..@.....3.....D.q#",
        "#t*.c#......*.......#~.#",
        "###########X############",
    ]),
    ("B1", "Subsolo técnico", False, -400, True, [
        "########################",
        "#SS.....g.~....*.....SS#",
        "#SS####D####.####D###SS#",
        "#SS#t.t.t#.1.#G.t.C#.SS#",
        "#SS#d.*..D...D..~.k#.SS#",
        "####t.t.t#...#t...t#.###",
        "#.L#######...#######...#",
        "#V.D.....*.2.......D.Lv#",
        "#.L#######...#######...#",
        "####m.m.m#...#t.adt#####",
        "#..D..~..D.3.D..*..D...#",
        "#..#m.o.m#...#t.t%t#...#",
        "#.*#######.*.#######.n.#",
        "#.....................b#",
        "########################",
    ]),
    ("F1", "Emergência", False, 0, True, [
        "########################",
        "#SS..~....@........*.SS#",
        "#SS.####D#...#D#####.SS#",
        "#SS.#t...#d..#....N#.SS#",
        "#SS..t.*.D.1.D..*.L#.SS#",
        "###.#t...#...#....t#.###",
        "#M..#####D...#D#####..a#",
        "#V..D..*...2.....*.D..t#",
        "#...#####D...D######...#",
        "#.L.#m.m.#...#tRt..#...#",
        "#.*.D..~.D.3.D..*.tD.*.#",
        "#.L.#m.m.#...#t...d#..L#",
        "#...######..k########..#",
        "#.....b.....*........E.#",
        "########################",
    ]),
    ("F2", "Administração", False, 400, True, [
        "########################",
        "#SS.....@.....~......SS#",
        "#SS####D###...###D###SS#",
        "#SS#tBt.t#.1.#tdt.=#.SS#",
        "#SS#d.*..D...D..*..#.SS#",
        "####t.t.t#...#O.Qdt#.###",
        "#.t#######...#######.t.#",
        "#V.D.....*.2.......D.L.#",
        "#.L#######...#######...#",
        "####t.t.c#...#t.Z.t#####",
        "#t.D..~..D.3.D..*..D..t#",
        "#t.#tdF.t#...#t.t.t#.n.#",
        "#.*#######.*.#######...#",
        "#.....................E#",
        "########################",
    ]),
    ("F3", "Clínica", False, 800, True, [
        "########################",
        "#SS.....*.....~......SS#",
        "#SS.####D#...#D#####.SS#",
        "#SS.#t.td#...#m.m.m#.SS#",
        "#SS..t*t.D.1.D..*..#.SS#",
        "###.#....#...#m.m.m#.###",
        "#dL.#####D...#D#####.L.#",
        "#V..D..*...2.....~.D...#",
        "#########D...D######...#",
        "#.i.#m.m.#...#t.t..#...#",
        "#.iiI..*.D.3.D..*..D...W",
        "#.i.#m.Ld#...#t.L..#...#",
        "##########.@.########..#",
        "#.....x...........g..E.#",
        "########################",
    ]),
    ("F4", "Enfermarias", False, 1200, True, [
        "########################",
        "#SS.....~.....*..@...SS#",
        "#SS####D####.####D###SS#",
        "#SS#m.m.m#..1#L.Y.m#.SS#",
        "#SS#..*..D...D..*..#.SS#",
        "####m.m.L#...#m.m.m#.###",
        "w.L#######...#######.L.#",
        "#V.D.....~.2.......D.J.w",
        "#.L#######...#######...#",
        "####m.P.m#...#tAt.t#####",
        "#.aD..*..D.3.D..*..D.b.#",
        "#.L#m.m.L#...#t.U.t#.L.#",
        "#.*#######.*.#######.s.#",
        "#.....g...........g..E.#",
        "########################",
    ]),
    ("R", "Telhado", True, 1600, True, [
        "########################",
        "#SS..................SS#",
        "#SS..hh......hh......SS#",
        "#SS..hh..1...hh......SS#",
        "#SS..................SS#",
        "##...........H........##",
        "#..hh....######...hh...#",
        "#..hh....#....#...hh...#",
        "#........#.2..#........#",
        "#........##..##........#",
        "#...3..................#",
        "#.......hh......hh.....#",
        "#.......hh..T...hh.....#",
        "#......................#",
        "########################",
    ]),
    ("T", "Túneis", False, -1200, False, [
        "########################",
        "#...~...#.........#..!.#",
        "#.#####.#.#######.#.####",
        "#.#...#.#.#.....#.#....#",
        "#.#.#.#.#.#.###.#.####.#",
        "#...#.#...#.#.#.#......#",
        "#####.#####.#.#.########",
        "#.....#.....#.#........#",
        "#.###.#.#####.########.#",
        "#.#...#.#...*.......#..#",
        "#.#.###.#.#########.#.##",
        "#.#.....#.......#%..#..#",
        "#.%##########.#.#####..#",
        "#..~.j........#........#",
        "########################",
    ]),
]

# Narrative anchors: which hospital_graph.json node each cell belongs to (first match wins).
# Stairwells are added automatically (<floor>_ST_W / _ST_E). The last rect of a floor is its default.
ZONES = {
    "F4": [("F4_START", 4, 9, 8, 11), ("F4_NURSE", 14, 9, 18, 11), ("F4_WARD_W", 4, 3, 8, 5),
           ("F4_WARD_E", 14, 3, 18, 5), ("F4_OBSERVATION", 20, 6, 22, 9), ("F4_LIFT", 20, 12, 22, 13),
           ("F4_SERVICE", 1, 7, 1, 7), ("F4_STORAGE", 1, 6, 2, 12), ("F4_BATH", 1, 13, 9, 13),
           ("F4_HALL_E", 10, 1, 22, 13), ("F4_HALL_W", 0, 0, 23, 14)],
    "F3": [("F3_EXAMS", 5, 3, 8, 5), ("F3_ICU", 14, 3, 18, 5), ("F3_UTILITY", 5, 9, 8, 11),
           ("F3_ISOLATION", 1, 9, 4, 11), ("F3_SURGERY", 14, 9, 18, 11), ("F3_WINDOW", 20, 8, 22, 11),
           ("F3_LIFT", 20, 12, 22, 13), ("F3_PIPE", 1, 7, 1, 7), ("F3_RECOVERY", 13, 7, 22, 7),
           ("F3_HALL_W", 0, 0, 4, 14), ("F3_CENTRAL", 0, 0, 23, 14)],
    "F2": [("F2_ARCHIVE", 4, 3, 8, 5), ("F2_CAMERAS", 14, 3, 18, 4), ("F2_SECURITY", 14, 5, 18, 5),
           ("F2_SUPPLY", 4, 9, 8, 9), ("F2_SERVER", 4, 10, 8, 11), ("F2_SAFE", 14, 9, 18, 11),
           ("F2_SERVICE", 1, 6, 2, 8), ("F2_LIFT", 20, 12, 22, 13), ("F2_MEETING", 20, 6, 22, 11),
           ("F2_ADMIN", 3, 7, 19, 7), ("F2_CENTRAL", 0, 0, 23, 14)],
    "F1": [("F1_PHARMACY", 5, 3, 8, 5), ("F1_SECURITY", 14, 3, 18, 5), ("F1_TREATMENT", 5, 9, 8, 11),
           ("F1_RADIO", 14, 9, 18, 11), ("F1_SERVICE", 1, 7, 1, 7), ("F1_TRIAGE", 1, 6, 3, 12),
           ("F1_LIFT", 20, 12, 22, 13), ("F1_LOADING", 20, 6, 22, 11), ("F1_EMERGENCY", 3, 7, 19, 7),
           ("F1_LOBBY", 0, 0, 23, 14)],
    "B1": [("B1_GENERATOR", 14, 3, 18, 5), ("B1_STORAGE", 4, 3, 8, 5), ("B1_MORGUE", 4, 9, 8, 11),
           ("B1_TUNNEL", 17, 11, 17, 11), ("B1_PUMP", 14, 9, 18, 11), ("B1_PIPE", 1, 7, 1, 7),
           ("B1_LAUNDRY", 1, 6, 2, 13), ("B1_LIFT", 20, 12, 22, 13), ("B1_VALVE", 20, 6, 22, 11),
           ("B1_CENTRAL", 0, 0, 23, 14)],
    "G": [("G_TUNNEL_LINK", 2, 12, 2, 12), ("G_MAINT", 1, 11, 4, 13), ("G_CONTROL", 21, 11, 22, 13),
          ("G_GATE", 6, 11, 19, 13), ("G_VEHICLES", 3, 6, 19, 8), ("G_FLOOR", 0, 0, 23, 14)],
    "R": [("R_PAD", 9, 10, 15, 13), ("R_LEDGE", 1, 13, 22, 13), ("R_HVAC", 0, 0, 23, 14)],
    "T": [("T_EXIT", 19, 1, 22, 1), ("T_PUMP", 1, 11, 7, 13), ("T_VALVE", 15, 9, 22, 13),
          ("T_DEAD_A", 1, 1, 7, 5), ("T_DEAD_B", 9, 3, 16, 5), ("T_LOOP", 1, 6, 22, 8),
          ("T_FORK", 0, 0, 23, 14)],
}

# Documents (campaign DOC06-DOC16). DOC01-DOC05 are the escape-route stations A/B/C/N/O.
DOCS = {
    ("B1", 17, 9): "DOC06", ("F3", 8, 11): "DOC07", ("F1", 18, 11): "DOC08", ("F2", 15, 3): "DOC09",
    ("F3", 8, 3): "DOC10", ("F2", 5, 11): "DOC11", ("F1", 10, 3): "DOC12", ("F2", 4, 4): "DOC13",
    ("F3", 1, 6): "DOC14", ("B1", 4, 4): "DOC15", ("F2", 17, 5): "DOC16",
}
STATIONS = set("ABCNOZGqRFQTdekjo")


def zone_rects(fid, stairs):
    rects = []
    if stairs:
        rects += [(f"{fid}_ST_W", 1, 1, 2, 4), (f"{fid}_ST_E", 21, 1, 22, 4)]
    return rects + ZONES[fid]


def validate_zones():
    import json
    from pathlib import Path
    graph = json.loads((Path(__file__).resolve().parent / "hospital_graph.json").read_text(encoding="utf-8"))
    nodes = {n["id"] for n in graph["nodes"]}
    ok = True
    used = set()
    for idx, (fid, name, outdoor, z, stairs, rows) in enumerate(FLOORS):
        if fid not in ZONES:
            print(f"{fid}: no zones"); ok = False; continue
        for node, x0, y0, x1, y1 in zone_rects(fid, stairs):
            if node not in nodes:
                print(f"{fid}: zone {node} is not a hospital_graph node"); ok = False
            if not (0 <= x0 <= x1 < W and 0 <= y0 <= y1 < H):
                print(f"{fid}: zone {node} out of bounds"); ok = False
        if ZONES[fid][-1][1:] != (0, 0, W - 1, H - 1):
            print(f"{fid}: last zone must be the floor default"); ok = False
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch == "d":
                    if (fid, x, y) not in DOCS:
                        print(f"{fid}: document at ({x},{y}) has no id"); ok = False
                    used.add((fid, x, y))
                if ch in STATIONS and not any(
                        0 <= x + dx < W and 0 <= y + dy < H and rows[y + dy][x + dx] in WALKABLE
                        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    print(f"{fid}: station {ch} at ({x},{y}) cannot be reached"); ok = False
    for key in DOCS:
        if key not in used:
            print(f"document {DOCS[key]} at {key} is not a 'd' cell"); ok = False
    if len(set(DOCS.values())) != len(DOCS):
        print("duplicate document ids"); ok = False
    return ok


FLOOR_HEIGHT = 400
STAIRS = [(1, 1), (21, 1)]  # (lane A column, top row) for west and east stairwells

WALKABLE = set(".*~DLPHYUJbansxcgiIV v@123456789%!T")
WALKABLE.discard(" ")


def has_stairs(i):
    return 0 <= i < len(FLOORS) and FLOORS[i][4]


def stair_cells(floor_index, lane_a, r0):
    """Returns (walkable cells, void cells, ramp cells, up_link) for one stairwell on one floor."""
    a, b = lane_a, lane_a + 1
    r1, r2, r3 = r0 + 1, r0 + 2, r0 + 3
    even = floor_index % 2 == 0
    top = not has_stairs(floor_index + 1)
    bottom = floor_index == 0
    walk, void, ramp = set(), set(), set()
    up = None
    if even:
        walk |= {(a, r3), (b, r3)}
        if not top:
            ramp |= {(a, r2), (a, r1)}
            up = ((a, r1), (a, r0))
        else:
            walk |= {(a, r2), (a, r1)}
        if not bottom:
            void |= {(b, r1), (b, r2)}
    else:
        walk |= {(a, r0), (b, r0)}
        if not top:
            ramp |= {(b, r1), (b, r2)}
            up = ((b, r2), (b, r3))
        void |= {(a, r1), (a, r2)}
    return walk | ramp, void, ramp, up


def validate():
    ok = True
    for idx, (fid, name, outdoor, z, stairs, rows) in enumerate(FLOORS):
        if len(rows) != H:
            print(f"{fid}: {len(rows)} rows"); ok = False
        for y, row in enumerate(rows):
            if len(row) != W:
                print(f"{fid} row {y}: width {len(row)} -> {row!r}"); ok = False
    if not ok:
        return False

    # Build walkable graph across floors.
    walk = {}
    links = {}
    for idx, (fid, name, outdoor, z, stairs, rows) in enumerate(FLOORS):
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch in WALKABLE:
                    walk[(idx, x, y)] = ch
        if not stairs:
            continue
        for (la, r0) in STAIRS:
            w, v, ramp, up = stair_cells(idx, la, r0)
            for (x, y) in w:
                walk[(idx, x, y)] = "S"
            if up:
                lo, hi = up
                links.setdefault((idx, *lo), []).append((idx + 1, *hi))
                links.setdefault((idx + 1, *hi), []).append((idx, *lo))
    # Hatches (V shaft and v shaft) link vertically to the same XY.
    for (f, x, y), ch in list(walk.items()):
        if ch in "Vv":
            for g in (f - 1, f + 1):
                if walk.get((g, x, y)) == ch:
                    links.setdefault((f, x, y), []).append((g, x, y))
        if ch == "%":
            for g in range(len(FLOORS)):
                if g != f and walk.get((g, x, y)) == "%":
                    links.setdefault((f, x, y), []).append((g, x, y))

    for idx, (fid, name, outdoor, z, stairs, rows) in enumerate(FLOORS):
        if not stairs:
            continue
        for (la, r0) in STAIRS:
            for y in range(r0, r0 + 4):
                for x in (la, la + 1):
                    if rows[y][x] != "S":
                        print(f"{fid}: stairwell cell ({x},{y}) is {rows[y][x]!r}, expected 'S'"); ok = False
    for (f, x, y), ch in walk.items():
        if ch == "%" and not any(walk.get((g, x, y)) == "%" for g in range(len(FLOORS)) if g != f):
            print(f"{FLOORS[f][0]}: tunnel ladder at ({x},{y}) has no partner"); ok = False
        if ch in "Vv" and not any(walk.get((g, x, y)) == ch for g in (f - 1, f + 1)):
            print(f"{FLOORS[f][0]}: hatch {ch} at ({x},{y}) has no partner above/below"); ok = False

    start = next(k for k, c in walk.items() if c == "P")
    seen = {start}
    queue = [start]
    while queue:
        f, x, y = queue.pop()
        nbrs = [(f, x + 1, y), (f, x - 1, y), (f, x, y + 1), (f, x, y - 1)] + links.get((f, x, y), [])
        for n in nbrs:
            if n in walk and n not in seen:
                seen.add(n)
                queue.append(n)
    unreached = [k for k in walk if k not in seen]
    for k in unreached[:40]:
        print("unreachable", FLOORS[k[0]][0], k[1:], walk[k])
    if unreached:
        ok = False
    # Every floor must be reached by stairs without hatches too.
    for idx, (fid, *_rest) in enumerate(FLOORS):
        cnt = sum(1 for k in seen if k[0] == idx)
        print(f"{fid}: {cnt} reachable cells")
    return ok


def emit_zones():
    out = ["// Generated by Tools/MapCore/blockout_floors.py — edit the Python file, not this one.", "",
           "const FZoneSource ZoneSources[] = {"]
    for fid, name, outdoor, z, stairs, rows in FLOORS:
        for node, x0, y0, x1, y1 in zone_rects(fid, stairs):
            out.append(f"\t{{ TEXT(\"{fid}\"), TEXT(\"{node}\"), {x0}, {y0}, {x1}, {y1} }},")
    out += ["};", "", "const FDocSource DocSources[] = {"]
    for (fid, x, y), doc in sorted(DOCS.items(), key=lambda kv: kv[1]):
        out.append(f"\t{{ TEXT(\"{fid}\"), {x}, {y}, TEXT(\"{doc}\") }},")
    out += ["};", ""]
    return "\n".join(out)


def emit():
    out = ["// Generated by Tools/MapCore/blockout_floors.py — edit the Python file, not this one.", ""]
    for idx, (fid, name, outdoor, z, stairs, rows) in enumerate(FLOORS):
        out.append(f"\t{{ TEXT(\"{fid}\"), TEXT(\"{name}\"), {'true' if outdoor else 'false'}, {z}.f, {'true' if stairs else 'false'}, {{")
        for row in rows:
            out.append(f"\t\tTEXT(\"{row}\"),")
        out.append("\t} },")
    return "\n".join(out)


if __name__ == "__main__":
    good = validate() and validate_zones()
    if "--emit" in sys.argv:
        print(emit())
    if "--write" in sys.argv and good:
        from pathlib import Path
        src = Path(__file__).resolve().parents[2] / "UnrealProject" / "Source" / "TheThug"
        (src / "TTHospitalLayout.inl").write_text("\ufeff" + emit() + "\n", encoding="utf-8", newline="\n")
        (src / "TTHospitalZones.inl").write_text("\ufeff" + emit_zones(), encoding="utf-8", newline="\n")
        print("wrote TTHospitalLayout.inl and TTHospitalZones.inl")
    sys.exit(0 if good else 1)
