"""Eligibility for authored early scenes; the Unreal adapter supplies observed facts.

The director cannot move an actor, generate perception, apply inventory effects or
advance a companion relationship. An eligible scene still requires a real trigger.
"""

from dataclasses import dataclass, field
import json
from pathlib import Path
from typing import Mapping, FrozenSet

from variant_rules import mode, scene_available


CONTRACT = json.loads(Path(__file__).with_name("act12_scenes.json").read_text(encoding="utf-8"))
SCENES = {scene["id"]: scene for scene in CONTRACT["scenes"]}


@dataclass(frozen=True)
class Actor:
    node: str
    active: bool = True


@dataclass(frozen=True)
class Facts:
    difficulty: str
    joe_node: str
    seen: FrozenSet[str] = frozenset()
    facts: FrozenSet[str] = frozenset()
    companions: Mapping[str, Actor] = field(default_factory=dict)
    hunter_has_evidence: bool = False
    hunter_can_reach: bool = False
    alternative_reachable: bool = False
    area_previously_visited: bool = False


def eligible(scene_id: str, state: Facts) -> bool:
    mode(state.difficulty)  # Reject misspelled modes instead of silently leaking content.
    scene = SCENES[scene_id]
    if scene_id in state.seen or not set(scene["must_seen"]).issubset(state.seen):
        return False
    if scene["anchors"] and state.joe_node not in scene["anchors"]:
        return False
    actor_id = scene.get("companion")
    if actor_id:
        actor = state.companions.get(actor_id)
        if actor is None or not actor.active or actor.node != state.joe_node:
            return False
    if "any_seen" in scene and not (
        set(scene["any_seen"]) & state.seen
        or scene["alternative_fact"] in state.facts
    ):
        return False
    if scene.get("requires_hunter_evidence") and not state.hunter_has_evidence:
        return False
    if scene.get("requires_reachable_hunter") and not state.hunter_can_reach:
        return False
    if scene.get("requires_reachable_exit") and not state.alternative_reachable:
        return False
    if scene.get("requires_visited_area") and not state.area_previously_visited:
        return False
    return True


def can_witness_j1(state: Facts, sight_and_sound: bool) -> bool:
    """J1 is recorded only when Joe actually witnesses Jack's Act 2 beat."""
    jack = state.companions.get("jack")
    return ("N08" in state.seen and sight_and_sound and jack is not None
            and jack.active and jack.node == state.joe_node
            and scene_available("J1", state.difficulty, act=2))
