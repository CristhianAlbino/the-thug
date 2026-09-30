#!/usr/bin/env python3
"""Validate authored hospital data and solve the four escape routes.

The solver tracks actual player position and collected flags. It deliberately
ignores optional actions unrelated to the selected ending to keep states small.
All test times are traversal hypotheses, without interaction, stealth or combat.
"""

from collections import defaultdict, deque
import heapq
import json
from pathlib import Path
import re

from map_data import build


ROOT = Path(__file__).resolve().parent
DATA = build()


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def check_contract():
    exported = json.loads((ROOT / "hospital_graph.json").read_text(encoding="utf-8"))
    require(exported == DATA, "JSON desatualizado: rode build_map.py")
    nodes = {n["id"]: n for n in DATA["nodes"]}
    floors = {f["id"] for f in DATA["floors"]}
    grants = {a["grants"] for a in DATA["actions"]}
    require(len(nodes) == len(DATA["nodes"]), "IDs de nós duplicados")
    require(len(grants) == len(DATA["actions"]), "flags concedidas duplicadas")
    require(DATA["start"] in nodes, "início inexistente")
    for n in nodes.values():
        require(n["floor"] in floors, f"andar inválido: {n['id']}")
    edge_keys = set()
    degrees = defaultdict(int)
    for e in DATA["edges"]:
        a, b = e["a"], e["b"]
        require(a in nodes and b in nodes and a != b, f"aresta inválida: {a}/{b}")
        require(e["seconds"] > 0, f"tempo inválido: {a}/{b}")
        require(set(e["requires"]) <= grants, f"trava sem ação: {a}/{b}")
        require(e["exposure"] in ("low", "medium", "high"), f"exposição: {a}/{b}")
        key = (a, b) if e["one_way"] else tuple(sorted((a, b)))
        require(key not in edge_keys, f"aresta duplicada: {a}/{b}")
        edge_keys.add(key)
        degrees[a] += 1
        degrees[b] += 1
    require(set(nodes) == set(degrees), "nós desconectados estruturalmente")
    for a in DATA["actions"]:
        require(a["at"] in nodes, f"ação em sala inexistente: {a['id']}")
        require(set(a["requires"]) <= grants, f"pré-requisito sem fonte: {a['id']}")
        require(len(a["requires"]) == len(set(a["requires"])), f"requisito duplicado: {a['id']}")
    require(set(DATA["exits"]) == {"garage", "roof", "tunnel", "window"},
            "conjunto de saídas incorreto")
    for name, route in DATA["exits"].items():
        require(route["node"] in nodes and route["flag"] in grants,
                f"saída inválida: {name}")
        require(any(a["grants"] == route["flag"] and a["at"] == route["node"]
                    for a in DATA["actions"]), f"evento final ausente: {name}")
        require(any(e["forbids"] == [route["failure"]]
                    and route["node"] in (e["a"], e["b"])
                    for e in DATA["edges"]), f"falha não bloqueia saída: {name}")
    for sight in DATA["sightlines"]:
        require(sight["a"] in nodes and sight["b"] in nodes,
                f"visada inválida: {sight}")
    devices = DATA["devices"]
    require(set(devices) == {"circuits", "doors", "cameras", "hides"},
            "famílias de dispositivos inválidas")
    ids = [d["id"] for group in devices.values() for d in group]
    require(len(ids) == len(set(ids)), "IDs duplicados de dispositivos")
    circuits = {c["id"] for c in devices["circuits"]}
    require(circuits and all(c["at"] in nodes and c["initial"] in ("on", "off")
                             for c in devices["circuits"]), "circuito inválido")
    door_edges = set()
    for door in devices["doors"]:
        pair = frozenset((door["a"], door["b"]))
        require(pair in {frozenset((e["a"], e["b"])) for e in DATA["edges"]},
                f"porta sem ligação: {door['id']}")
        require(pair not in door_edges, f"duas portas numa mesma ligação: {door['id']}")
        door_edges.add(pair)
        require(door["initial"] in ("open", "closed", "locked", "jammed"),
                f"estado de porta inválido: {door['id']}")
        require(door["power"] in ("manual", "needs_power", "releases_when_off")
                and ((door["power"] == "manual" and not door["circuit"])
                     or (door["power"] != "manual" and door["circuit"] in circuits)),
                f"alimentação de porta inválida: {door['id']}")
    for camera in devices["cameras"]:
        require(camera["at"] in nodes and camera["view"] in nodes
                and camera["circuit"] in circuits
                and camera["initial"] in ("online", "interference", "broken", "looped"),
                f"câmera inválida: {camera['id']}")
    for hide in devices["hides"]:
        require(hide["at"] in nodes and hide["initial"] in ("empty", "occupied"),
                f"esconderijo inválido: {hide['id']}")
    all_adjacent = defaultdict(list)
    for edge in DATA["edges"]:
        all_adjacent[edge["a"]].append(edge["b"])
        if not edge["one_way"]:
            all_adjacent[edge["b"]].append(edge["a"])
    seen = {DATA["start"]}
    frontier = [DATA["start"]]
    while frontier:
        for other in all_adjacent[frontier.pop()]:
            if other not in seen:
                seen.add(other)
                frontier.append(other)
    require(seen == set(nodes), f"setores isolados: {set(nodes) - seen}")
    require(all(any(e["kind"] == "stairs" and {e["a"], e["b"]} ==
                    {f"{a}_ST_{side}", f"{b}_ST_{side}"} for e in DATA["edges"])
                for side in ("E", "W")
                for a, b in (("F4", "F3"), ("F3", "F2"), ("F2", "F1"),
                             ("F1", "B1"), ("B1", "G"))),
            "uma das duas escadas verticais está interrompida")
    require(sum(e["kind"] == "pipe_crawl" for e in DATA["edges"]) >= 5,
            "prumada de tubulações incompleta")
    # Every RunRules event emitted by this data must be a real C++ enum value.
    header = (ROOT / "../HunterCore/RunRules.h").read_text(encoding="utf-8")
    event_enum = re.search(r"enum class Event\s*\{([^}]*)\}", header, re.S)
    require(event_enum is not None, "enum RunRules::Event ausente")
    cpp_events = set(re.findall(r"\b[A-Za-z][A-Za-z0-9_]*\b",
                                re.sub(r"//[^\n]*", "", event_enum.group(1))))
    emitted = {a["run_event"] for a in DATA["actions"] if a["run_event"]}
    require(emitted <= cpp_events, f"eventos sem contrato C++: {emitted - cpp_events}")
    return nodes


def route_flags(target):
    """Backward prerequisite closure from a particular ending."""
    by_grant = {a["grants"]: a for a in DATA["actions"]}
    needed = {target}
    frontier = [target]
    while frontier:
        flag = frontier.pop()
        for req in by_grant[flag]["requires"]:
            if req not in needed:
                needed.add(req)
                frontier.append(req)
    return needed


def solve(route_name, failures=(), blocked_edges=(), start_node=None,
          initial_flags=()):
    failures = set(failures)
    route = DATA["exits"][route_name]
    needed = route_flags(route["flag"])
    actions = defaultdict(list)
    adjacent = defaultdict(list)
    for action in DATA["actions"]:
        if action["grants"] in needed:
            actions[action["at"]].append(action)
    blocked = {frozenset(pair) for pair in blocked_edges}
    for edge in DATA["edges"]:
        if frozenset((edge["a"], edge["b"])) in blocked:
            continue
        adjacent[edge["a"]].append((edge["b"], edge))
        if not edge["one_way"]:
            adjacent[edge["b"]].append((edge["a"], edge))

    def collect(node, flags):
        owned = set(flags)
        gained = []
        while True:
            available = [a for a in actions[node]
                         if a["grants"] not in owned
                         and set(a["requires"]) <= owned]
            if not available:
                break
            for action in available:
                owned.add(action["grants"])
                gained.append(action["id"])
        return frozenset(owned), gained

    origin = start_node or DATA["start"]
    require(origin in {node["id"] for node in DATA["nodes"]},
            f"início inexistente: {origin}")
    initial_flags, gained = collect(origin, frozenset(initial_flags))
    start = (origin, initial_flags)
    queue = deque([start])
    previous = {start: None}
    effects = {start: gained}
    end = None
    while queue:
        state = queue.popleft()
        node, flags = state
        if node == route["node"] and route["flag"] in flags:
            end = state
            break
        for neighbor, edge in adjacent[node]:
            if not set(edge["requires"]) <= flags or set(edge["forbids"]) & failures:
                continue
            new_flags, gained = collect(neighbor, flags)
            next_state = (neighbor, new_flags)
            if next_state not in previous:
                previous[next_state] = state
                effects[next_state] = gained
                queue.append(next_state)
    if end is None:
        return None
    witness = []
    while end is not None:
        witness.append((end[0], effects[end]))
        end = previous[end]
    witness.reverse()
    return witness


def travel_seconds(start, end, flags, blocked_edges=()):
    blocked = {frozenset(pair) for pair in blocked_edges}
    adjacent = defaultdict(list)
    for edge in DATA["edges"]:
        if (frozenset((edge["a"], edge["b"])) in blocked
                or not set(edge["requires"]) <= flags):
            continue
        adjacent[edge["a"]].append((edge["b"], edge["seconds"]))
        if not edge["one_way"]:
            adjacent[edge["b"]].append((edge["a"], edge["seconds"]))
    queue = [(0, start)]
    best = {start: 0}
    while queue:
        seconds, node = heapq.heappop(queue)
        if node == end:
            return seconds
        if seconds != best[node]:
            continue
        for other, cost in adjacent[node]:
            if seconds + cost < best.get(other, float("inf")):
                best[other] = seconds + cost
                heapq.heappush(queue, (seconds + cost, other))
    return None


def main():
    nodes = check_contract()
    routes = DATA["exits"]
    for name in routes:
        path = solve(name)
        require(path is not None, f"rota inacessível: {name}")
        require(all(solve(name, {other["failure"]}) is not None
                    for other in routes.values() if other != routes[name]),
                f"falha de outra rota bloqueia {name}")
        # Even three failed exits leave the fourth reachable.
        failures = {other["failure"] for other in routes.values()
                    if other != routes[name]}
        require(solve(name, failures) is not None,
                f"três falhas bloqueiam a última rota: {name}")
        require(solve(name, {routes[name]["failure"]}) is None,
                f"falha não bloqueia a própria rota: {name}")
        actions = [effect for _, gains in path for effect in gains]
        print(f"{name}: {len(path)} travessias, {len(actions)} ações necessárias")
    require(all(solve(name, {r["failure"] for r in routes.values()}) is None
                for name in routes), "quatro falhas deveriam bloquear saídas")
    # Unpowered lift and either one of the two vertical stair spines.
    for name in routes:
        lifts = [(e["a"], e["b"]) for e in DATA["edges"] if e["kind"] == "lift"]
        require(solve(name, blocked_edges=lifts) is not None,
                f"rota depende do elevador: {name}")
    disabled = [(d["a"], d["b"]) for d in DATA["devices"]["doors"]
                if d["initial"] in ("locked", "jammed")
                or d["power"] == "needs_power"]
    for name in routes:
        require(solve(name, blocked_edges=disabled) is not None,
                f"rota depende de porta inicialmente trancada ou energizada: {name}")
    for side in ("E", "W"):
        blocked = [(f"{a}_ST_{side}", f"{b}_ST_{side}")
                   for a, b in (("F4", "F3"), ("F3", "F2"), ("F2", "F1"),
                                ("F1", "B1"), ("B1", "G"))]
        for name in routes:
            require(solve(name, blocked_edges=blocked) is not None,
                    f"rota depende da escada {side}: {name}")
    for blocked in ((), (("F2_ST_E", "F1_ST_E"),),
                    (("F2_ST_W", "F1_ST_W"),)):
        estimate = travel_seconds("F1_RADIO", "X_ROOF",
                                  {"helicopter_confirmed"}, blocked)
        require(estimate is not None and estimate < 300,
                f"telhado excede janela de cinco minutos no grafo: {estimate}")
    print(f"OK: {len(nodes)} nós, {len(DATA['edges'])} ligações, "
          f"{len(DATA['actions'])} ações, 4 fugas; hipóteses de tempo pendem do blockout")


if __name__ == "__main__":
    main()
