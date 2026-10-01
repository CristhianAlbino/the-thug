"""Late campaign scene eligibility from observed, spatially supplied facts.

The returned permission neither moves an actor nor grants a route or outcome.
RunRules owns escape victories; WorldCore and the Unreal adapter own spatial facts.
"""

import json
from pathlib import Path

from act12_rules import Facts
from variant_rules import mode, scene_available


ROOT = Path(__file__).resolve().parent
CONTRACT = json.loads((ROOT / "act35_scenes.json").read_text(encoding="utf-8"))
SCENES = {scene["id"]: scene for scene in CONTRACT["scenes"]}
GRAPH = json.loads((ROOT / "../MapCore/hospital_graph.json").read_text(encoding="utf-8"))
NODES = {node["id"] for node in GRAPH["nodes"]}
NODE_ROLES = {node["id"]: node["role"] for node in GRAPH["nodes"]}

ROUTE_ENDS = {
    "N28": ("X_GARAGE", "garage_escaped"),
    "N29": ("X_ROOF", "roof_escaped"),
    "N30": ("X_TUNNEL", "tunnel_escaped"),
    "N31": ("X_WINDOW", "window_escaped"),
}
FAILURE_ROUTES = {"garage", "roof", "tunnel", "window"}


def eligible_late(scene_id: str, state: Facts) -> bool:
    mode(state.difficulty)
    scene = SCENES[scene_id]
    if "N03" not in state.seen or (scene_id != "N26" and scene_id in state.seen):
        return False
    if state.joe_node not in NODES:
        return False
    if scene_id == "N21" and NODE_ROLES[state.joe_node] != "circulacao":
        return False
    if scene_id == "N26" and NODE_ROLES[state.joe_node] == "saida":
        return False
    if scene["anchors"] and state.joe_node not in scene["anchors"]:
        return False
    if "difficulty" in scene and state.difficulty not in scene["difficulty"]:
        return False
    actor_id = scene.get("companion")
    if actor_id:
        actor = state.companions.get(actor_id)
        if actor is None or not actor.active or actor.node != state.joe_node:
            return False
    if scene.get("facts_any") and not set(scene["facts_any"]) & state.facts:
        return False
    if scene.get("requires_reachable_hunter") and not state.hunter_can_reach:
        return False
    if scene.get("requires_reachable_exit") and not state.alternative_reachable:
        return False
    if scene_id == "N27" and state.joe_node == "F3_ISOLATION" and "isolation_known" not in state.facts:
        return False
    if scene_id == "N26" and (
        state.route_failure not in FAILURE_ROUTES
        or f"N26:{state.route_failure}" in state.seen
        or "route_failure_observed" not in state.facts
        or f"route_failed:{state.route_failure}" not in state.facts
    ):
        return False
    if scene_id == "N32" and state.difficulty == "Punitive" and "hide_victory" not in state.facts:
        return False
    if scene_id == "N33":
        winning_fact = "rare_confrontation" if state.difficulty == "Easy" else (
            "rare_destruction" if state.difficulty == "Punitive" else None)
        if winning_fact is None or winning_fact not in state.facts:
            return False
    if scene_id == "N34" and not (
        {"joe_identified_thomas_body", "joe_verified_batman_record"} <= state.facts
        or "joe_directly_witnessed_thomas_death" in state.facts
    ):
        return False
    return True


def can_close_route(scene_id: str, state: Facts) -> bool:
    """Narrative coda may close only after RunRules reported actual exit."""
    exit_node, runrules_fact = ROUTE_ENDS[scene_id]
    return (state.joe_node == exit_node and runrules_fact in state.facts
            and "N03" in state.seen)


def can_witness_late_clue(clue_id: str, state: Facts, act: int,
                          sight_and_sound: bool) -> bool:
    if clue_id not in ("J2", "J3", "J4"):
        raise ValueError("only late Jack clues are handled here")
    jack = state.companions.get("jack")
    related_scene = {"J2": None, "J3": "N25", "J4": None}[clue_id]
    if (not sight_and_sound or jack is None or not jack.active
            or jack.node != state.joe_node):
        return False
    if related_scene and related_scene not in state.seen:
        return False
    if clue_id == "J2" and not {
        "jack_act3_encounter", "jack_heard_thomas_or_injury"
    } <= state.facts:
        return False
    if clue_id == "J3" and "jack_perceived_companion_risk" not in state.facts:
        return False
    if clue_id == "J4" and "jack_final_encounter" not in state.facts:
        return False
    seen_clues = state.facts & {"J1", "J2", "J3"}
    return scene_available(clue_id, state.difficulty, seen_clues, act=act)
