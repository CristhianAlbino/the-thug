#!/usr/bin/env python3
"""Spoiler-free structural audit. Human comprehension still needs a blind reader."""

from collections import deque
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / "MapCore"))
from validate_map import solve  # noqa: E402
from puzzle_rules import PuzzleState, journal_fragments, submit_code  # noqa: E402


def check():
    puzzle = json.loads((ROOT / "puzzle_contract.json").read_text(encoding="utf-8"))["garage"]
    graph = json.loads((ROOT.parent / "MapCore/hospital_graph.json").read_text(encoding="utf-8"))
    package = (ROOT / "../../02_GAME_DESIGN/Narrative/PACOTE_TEXTO_CAMPANHA_V1.md").read_text(encoding="utf-8")
    lines = package.splitlines()
    adjacency = {}
    for edge in graph["edges"]:
        # A missing circuit/puzzle cannot hide a required clue behind itself.
        if any(flag in edge["requires"] for flag in ("power_restored", "safe_opened", "garage_key")):
            continue
        adjacency.setdefault(edge["a"], []).append(edge["b"])
        if not edge["one_way"]:
            adjacency.setdefault(edge["b"], []).append(edge["a"])
    visited = {graph["start"]}
    queue = deque(visited)
    while queue:
        for neighbor in adjacency.get(queue.popleft(), ()):
            if neighbor not in visited:
                visited.add(neighbor)
                queue.append(neighbor)

    entries = puzzle["fragments"] + [puzzle["key_discovery"], puzzle["safe_discovery"]]
    assert len({entry["document"] for entry in entries}) == len(entries)
    assert all(entry["node"] in visited for entry in entries), "A clue is physically gated"
    for entry in entries:
        row = next(line for line in lines if line.startswith("| " + entry["document"] + " ·"))
        assert entry["node"] in row, "Document's location disagrees with authored map"
        assert len(row) >= 75, "Clue has no player-facing authored text"
    for fragment in puzzle["fragments"]:
        row = next(line for line in lines if line.startswith("| " + fragment["document"] + " ·"))
        assert re.search(r"\*\*" + re.escape(fragment["pair"]) + r"\*\*", row)
    order = next(line for line in lines if line.startswith("| " + puzzle["order_document"] + " ·"))
    assert all(word in order for word in ("leitos", "arquivo", "manutenção")), "Order is not stated in game text"

    state = PuzzleState()
    assert journal_fragments(state) == ()
    for entry in puzzle["fragments"]:
        state = PuzzleState(state.seen_documents | {entry["document"]}, state.attempts, state.safe_opened)
        assert len(journal_fragments(state)) == len(state.seen_documents)
    assert not submit_code(state, puzzle["code"], at_safe=False)[1]
    wrong = "0" * len(puzzle["code"])
    if wrong == puzzle["code"]:
        wrong = "1" * len(puzzle["code"])
    state, accepted = submit_code(state, wrong, at_safe=True)
    assert not accepted
    state, accepted = submit_code(state, puzzle["code"], at_safe=True)
    assert accepted and state.safe_opened
    assert solve("garage") is not None
    for failed, fallback in (("garage", "tunnel"), ("window", "garage")):
        assert solve(fallback, failures={graph["exits"][failed]["failure"]}) is not None
    print("Auditoria sem spoilers: pistas alcançáveis, textos coerentes, erro reversível e rotas alternativas validados.")
    print("Limite: leitura humana cega, geometria, percepção e duração dependem de playtest.")


if __name__ == "__main__":
    check()
