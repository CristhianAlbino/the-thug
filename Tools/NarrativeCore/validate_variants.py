#!/usr/bin/env python3
"""Validate the authored difficulty matrix against map/story and story gates."""

import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from variant_rules import DATA, resolve_epilogue, scene_available  # noqa: E402
from act12_rules import Actor, CONTRACT, Facts, SCENES, can_witness_j1, eligible  # noqa: E402
from dataclasses import replace


def verify_early_scenes(nodes, devices):
    assert CONTRACT["schema"] == 1
    assert set(SCENES) == {f"N{i:02d}" for i in range(1, 12)}
    assert len(CONTRACT["scenes"]) == len(SCENES)
    manuscript = (ROOT / "../../02_GAME_DESIGN/Narrative/ROTEIRO_ATOS_1_2_RASCUNHO.md").read_text(encoding="utf-8")
    headings = re.findall(r"^### (N\d+) ·", manuscript, flags=re.M)
    assert len(headings) == len(set(headings)) == 11 and set(headings) == set(SCENES)
    refs = set(re.findall(r"`((?:F[1-4]|B1|G|T|R|A|X|CAM|D|H|C)_[A-Z0-9_]+)`", manuscript))
    assert refs <= nodes | devices, f"unknown early manuscript references: {refs - (nodes | devices)}"
    for scene in SCENES.values():
        assert scene["act"] in (1, 2)
        assert set(scene["anchors"]) <= nodes
        assert set(scene["must_seen"]) <= set(SCENES)
        assert set(scene.get("any_seen", ())) <= set(SCENES)

    opening = Facts("Hard", "F4_START")
    assert eligible("N01", opening)
    assert not eligible("N03", opening)
    assert eligible("N02", replace(opening, joe_node="F4_OBSERVATION", seen=frozenset({"N01"})))
    after_split = replace(opening, seen=frozenset({"N01", "N02", "N03"}))
    assert eligible("N04", replace(after_split, joe_node="F4_HALL_E"))
    assert eligible("N06", replace(after_split, joe_node="F3_CENTRAL"))
    assert eligible("N09", replace(after_split, joe_node="F2_SECURITY"))
    assert not eligible("N10", replace(after_split, joe_node="F2_SERVER"))
    assert eligible("N10", replace(after_split, joe_node="F2_SERVER", facts=frozenset({"doc11_read"})))

    harley_area = replace(after_split, joe_node="F3_ICU")
    assert not eligible("N07", harley_area)
    assert not eligible("N07", replace(harley_area, companions={"harley": Actor("F1_TREATMENT")}))
    assert not eligible("N07", replace(harley_area, companions={"harley": Actor("F3_ICU", False)}))
    assert eligible("N07", replace(harley_area, companions={"harley": Actor("F3_ICU")}))
    jack_area = replace(after_split, joe_node="F3_EXAMS", companions={"jack": Actor("F3_EXAMS")})
    assert eligible("N08", jack_area)
    assert not can_witness_j1(jack_area, True)
    jack_met = replace(jack_area, seen=jack_area.seen | {"N08"})
    assert can_witness_j1(replace(jack_met, difficulty="Punitive"), True)
    assert not can_witness_j1(replace(jack_met, difficulty="Punitive"), False)
    assert not can_witness_j1(replace(jack_met, difficulty="Normal"), True)
    assert not can_witness_j1(replace(jack_met, difficulty="Punitive", companions={"jack": Actor("F2_CAMERAS")}), True)

    hunt = replace(after_split, joe_node="F3_CENTRAL", hunter_has_evidence=True,
                   hunter_can_reach=True, alternative_reachable=True,
                   area_previously_visited=True)
    assert eligible("N11", hunt)
    for override in ({"hunter_has_evidence": False}, {"hunter_can_reach": False},
                     {"alternative_reachable": False}, {"area_previously_visited": False}):
        assert not eligible("N11", replace(hunt, **override))
    assert not eligible("N11", replace(hunt, seen=hunt.seen | {"N11"}))
    return len(refs)


def verify():
    assert DATA["schema"] == 1
    assert set(DATA["difficulty"]) == {"Easy", "Normal", "Hard", "Punitive"}
    assert DATA["world_truth"]["thomas"] == "killed_by_batman"
    story = (ROOT / "../../02_GAME_DESIGN/Narrative/CAMPANHA_ATOS_RASCUNHO.md").read_text(encoding="utf-8")
    graph = json.loads((ROOT / "../MapCore/hospital_graph.json").read_text(encoding="utf-8"))
    nodes = {node["id"] for node in graph["nodes"]}
    devices = {d["id"] for group in graph["devices"].values() for d in group}
    refs = set(re.findall(r"`((?:F[1-4]|B1|G|T|R|A|X|CAM|D|H|C)_[A-Z0-9_]+)`", story))
    assert refs <= nodes | devices, f"unknown story references: {refs - (nodes | devices)}"
    scenes = re.findall(r"\| (N\d+) ·", story)
    assert len(scenes) == len(set(scenes)) == 33
    assert DATA["thomas_reveal"]["scene"] == "N34" and story.count("**N34 ·") == 1
    assert set(DATA["thomas_reveal"]["scene_nodes"]) <= nodes
    clues = DATA["jack_secret"]["clues"]
    assert len(clues) == len(set(clues)) == 3 and DATA["jack_secret"]["late_clue"] in clues
    assert DATA["jack_secret"]["minimum_act"] == {"J1": 2, "J2": 3, "J3": 4, "J4": 5}
    assert set(DATA["epilogues"]) == {"harley", "bartholomeu", "jack"}
    assert len(set(DATA["companion_statuses"])) == 5
    early_refs = verify_early_scenes(nodes, devices)

    for difficulty in DATA["difficulty"]:
        revealed = difficulty in ("Hard", "Punitive")
        assert scene_available("N34", difficulty) == revealed
        assert scene_available("J1", difficulty, act=2) == (difficulty == "Punitive")
        assert scene_available("J4", difficulty, ("J1", "J3"), act=5) == (difficulty == "Punitive")
        outcomes = {name: "unknown" for name in DATA["epilogues"]}
        epilogue = resolve_epilogue(difficulty, outcomes)
        assert epilogue["thomas"] == (
            "E_THOMAS_CODA_PLAYER_ONLY" if revealed else "E_THOMAS_UNCONFIRMED")
        if revealed:
            assert resolve_epilogue(difficulty, outcomes, True)["thomas"] == "E_THOMAS_JOE_KNOWS"
        else:
            try:
                resolve_epilogue(difficulty, outcomes, True)
            except ValueError:
                pass
            else:
                raise AssertionError("Thomas evidence leaked into Easy/Normal")

    assert not scene_available("J1", "Punitive", act=1)
    assert not scene_available("J2", "Punitive", act=2)
    assert scene_available("J2", "Punitive", act=3)
    assert not scene_available("J3", "Punitive", ("J1",), act=3)
    assert scene_available("J3", "Punitive", act=4)
    assert not scene_available("J4", "Punitive", ("J1", "J2"), act=5)
    assert not scene_available("J4", "Punitive", ("J3",), act=5)
    assert not scene_available("J4", "Punitive", ("J1", "J3"), act=4)
    assert scene_available("J4", "Punitive", ("J2", "J3"), act=5)
    outcomes = {"harley": "together", "bartholomeu": "escaped_alone", "jack": "captured"}
    assert resolve_epilogue("Hard", outcomes)["harley"] != \
           resolve_epilogue("Punitive", outcomes)["harley"]
    assert len({resolve_epilogue("Hard", {**outcomes, "jack": status})["jack"]
                for status in DATA["companion_statuses"]}) == 5
    assert resolve_epilogue("Hard", {**outcomes, "harley": "unknown"})["harley"] != \
           resolve_epilogue("Hard", {**outcomes, "harley": "dead"})["harley"]
    try:
        resolve_epilogue("Normal", outcomes, jack_revelation_witnessed=True)
    except ValueError:
        pass
    else:
        raise AssertionError("Jack secret leaked into Normal")
    print(f"NarrativeCore: 4 difficulties, 34 scenes, {len(refs)} campaign and {early_refs} "
          "early-scene map/device IDs, Thomas/Jack gates and individual epilogues valid")


if __name__ == "__main__":
    verify()
