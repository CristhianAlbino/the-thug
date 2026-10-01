"""Eligibility for optional, authored in-world conversations.

The engine supplies proximity, presence and threat state. This module does not
spawn actors, infer events, decide routes, or grant Jack's Punitive clues.
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import FrozenSet, Mapping
import json

BANTER_PATH = Path(__file__).with_name("banter_ptbr.json")
DATA = json.loads(BANTER_PATH.read_text(encoding="utf-8"))
EXCHANGES = {entry["id"]: entry for entry in DATA["exchanges"]}


@dataclass(frozen=True)
class PresentActor:
    node: str
    active: bool = True


@dataclass(frozen=True)
class BanterFacts:
    difficulty: str
    act: int
    joe_node: str
    seen_scenes: FrozenSet[str] = frozenset()
    facts: FrozenSet[str] = frozenset()
    delivered: FrozenSet[str] = frozenset()
    actors: Mapping[str, PresentActor] = field(default_factory=dict)
    in_danger: bool = False
    main_scene_active: bool = False


def eligible_banter(exchange_id: str, state: BanterFacts) -> bool:
    if state.difficulty not in {"Easy", "Normal", "Hard", "Punitive"}:
        raise ValueError("unknown difficulty")
    entry = EXCHANGES[exchange_id]
    if entry["act"] != state.act or state.joe_node not in entry["nodes"]:
        return False
    if exchange_id in state.delivered or state.in_danger or state.main_scene_active:
        return False
    if state.difficulty not in entry.get("modes", ["Easy", "Normal", "Hard", "Punitive"]):
        return False
    if not set(entry.get("after", ())).issubset(state.seen_scenes):
        return False
    if not set(entry.get("requires_facts", ())).issubset(state.facts):
        return False
    if set(entry.get("before", ())) & state.seen_scenes:
        return False
    for name in entry["actors"]:
        if name == "joe":
            continue
        actor = state.actors.get(name)
        if actor is None or not actor.active or actor.node != state.joe_node:
            return False
    return True
