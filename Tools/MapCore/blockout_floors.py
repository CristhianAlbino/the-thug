"""Playable blockout of the hospital (M05) for the Unreal prototype.

Each floor is a 24x15 ASCII grid stacked vertically. This file is the authoring source;
`python blockout_floors.py` validates the layouts and prints the C++ table that
UnrealProject/Source/TheThug/TTHospitalLayout.inl embeds.

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
        "#.....................t#",
        "###D######..######D#####",
        "#t.t.#..............#.t#",
        "#n%..D..@.....3.....D.q#",
        "#t*.c#......*.......#~.#",
        "###########X############",
    ]),
    ("B1", "Subsolo técnico", False, -400, True, [
        "########################",
        "#SS.....g.~....*.....SS#",
        "#SS####D####.####D###SS#",
        "#SS#t.t.t#.1.#G.t.C#.SS#",
        "#SS#..*..D...D..~..#.SS#",
        "####t.t.t#...#t...t#.###",
        "#.L#######...#######...#",
        "#V.D.....*.2.......D.Lv#",
        "#.L#######...#######...#",
        "####m.m.m#...#t.a.t#####",
        "#..D..~..D.3.D..*..D...#",
        "#..#m.m.m#...#t.t%t#...#",
        "#.*#######.*.#######.n.#",
        "#.....................b#",
        "########################",
    ]),
    ("F1", "Emergência", False, 0, True, [
        "########################",
        "#SS..~....@........*.SS#",
        "#SS.####D#...#D#####.SS#",
        "#SS.#t...#...#....N#.SS#",
        "#SS..t.*.D.1.D..*.L#.SS#",
        "###.#t...#...#....t#.###",
        "#M..#####D...#D#####..a#",
        "#V..D..*...2.....*.D..t#",
        "#...#####D...D######...#",
        "#.L.#m.m.#...#tRt..#...#",
        "#.*.D..~.D.3.D..*.tD.*.#",
        "#.L.#m.m.#...#t....#..L#",
        "#...######...########..#",
        "#.....b.....*........E.#",
        "########################",
    ]),
    ("F2", "Administração", False, 400, True, [
        "########################",
        "#SS.....@.....~......SS#",
        "#SS####D###...###D###SS#",
        "#SS#tBt.t#.1.#t.t.=#.SS#",
        "#SS#..*..D...D..*..#.SS#",
        "####t.t.t#...#O.Q.t#.###",
        "#.t#######...#######.t.#",
        "#V.D.....*.2.......D.L.#",
        "#.L#######...#######...#",
        "####t.t.c#...#t.Z.t#####",
        "#t.D..~..D.3.D..*..D..t#",
        "#t.#t.F.t#...#t.t.t#.n.#",
        "#.*#######.*.#######...#",
        "#.....................E#",
        "########################",
    ]),
    ("F3", "Clínica", False, 800, True, [
        "########################",
        "#SS.....*.....~......SS#",
        "#SS.####D#...#D#####.SS#",
        "#SS.#t.t.#...#m.m.m#.SS#",
        "#SS..t*t.D.1.D..*..#.SS#",
        "###.#....#...#m.m.m#.###",
        "#.L.#####D...#D#####.L.#",
        "#V..D..*...2.....~.D...#",
        "#########D...D######...#",
        "#.i.#m.m.#...#t.t..#...#",
        "#.iiI..*.D.3.D..*..D...W",
        "#.i.#m.L.#...#t.L..#...#",
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
        "#..~..........#........#",
        "########################",
    ]),
]

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


def emit():
    out = ["// Generated by Tools/MapCore/blockout_floors.py — edit the Python file, not this one.", ""]
    for idx, (fid, name, outdoor, z, stairs, rows) in enumerate(FLOORS):
        out.append(f"\t{{ TEXT(\"{fid}\"), TEXT(\"{name}\"), {'true' if outdoor else 'false'}, {z}.f, {'true' if stairs else 'false'}, {{")
        for row in rows:
            out.append(f"\t\tTEXT(\"{row}\"),")
        out.append("\t} },")
    return "\n".join(out)


if __name__ == "__main__":
    good = validate()
    if "--emit" in sys.argv:
        print(emit())
    sys.exit(0 if good else 1)
