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
from act35_rules import (CONTRACT as LATE_CONTRACT, SCENES as LATE_SCENES,
                         ROUTE_ENDS, can_close_route, can_witness_late_clue,
                         eligible_late)  # noqa: E402
from ending_rules import CompanionEvidence, derive_outcomes, resolve_from_evidence  # noqa: E402
from banter_rules import (DATA as BANTER_DATA, EXCHANGES, BanterFacts,
                          PresentActor, eligible_banter)  # noqa: E402
from render_banter import render as render_banter  # noqa: E402
from puzzle_rules import PuzzleState, journal_fragments, submit_code  # noqa: E402
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


def verify_later_scenes(nodes, devices):
    manuscript = (ROOT / "../../02_GAME_DESIGN/Narrative/ROTEIRO_ATOS_3_5_RASCUNHO.md").read_text(encoding="utf-8")
    headings = re.findall(r"^### (N\d+) ·", manuscript, flags=re.M)
    assert headings == [f"N{i:02d}" for i in range(12, 35)]
    assert LATE_CONTRACT["schema"] == 1
    assert len(LATE_CONTRACT["scenes"]) == 23
    assert set(LATE_SCENES) == set(headings)
    for scene in LATE_SCENES.values():
        assert scene["act"] in (3, 4, 5)
        assert set(scene["anchors"]) <= nodes
    refs = set(re.findall(r"`((?:F[1-4]|B1|G|T|R|A|X|CAM|D|H|C)_[A-Z0-9_]+)`", manuscript))
    assert refs <= nodes | devices, f"unknown later manuscript references: {refs - (nodes | devices)}"
    assert "J4 é opcional e exclusiva do Punitivo" in manuscript
    assert "A coda não muda fuga" in manuscript

    after_split = Facts("Hard", "F1_PHARMACY", seen=frozenset({"N01", "N02", "N03"}))
    assert eligible_late("N12", after_split)
    assert eligible_late("N14", replace(after_split, joe_node="F2_ARCHIVE"))  # Fragment before key.
    assert not eligible_late("N15", replace(after_split, joe_node="B1_PUMP"))
    assert eligible_late("N15", replace(after_split, joe_node="B1_PUMP",
                                        companions={"bartholomeu": Actor("B1_PUMP")}))
    assert not eligible_late("N21", replace(after_split, facts=frozenset({"hunter_observed_habit"})))
    habit = replace(after_split, joe_node="F3_CENTRAL", facts=frozenset({"hunter_observed_habit"}),
                    hunter_can_reach=True, alternative_reachable=True)
    assert eligible_late("N21", habit)
    assert not eligible_late("N21", replace(habit, joe_node="F1_PHARMACY"))
    assert not eligible_late("N22", replace(after_split, joe_node="B1_GENERATOR"))
    assert eligible_late("N22", replace(after_split, joe_node="B1_GENERATOR",
                                        facts=frozenset({"circuit_changed"})))
    harley = replace(after_split, joe_node="F3_RECOVERY",
                     companions={"harley": Actor("F3_RECOVERY")})
    assert not eligible_late("N23", harley)
    assert eligible_late("N23", replace(harley, facts=frozenset({"harley_under_pressure"})))
    assert not eligible_late("N23", replace(harley, facts=frozenset({"harley_injured"}),
                                            companions={"harley": Actor("F3_RECOVERY", False)}))
    failure = replace(after_split, route_failure="roof", facts=frozenset({"route_failure_observed", "route_failed:roof"}))
    assert not eligible_late("N26", failure)
    assert eligible_late("N26", replace(failure, alternative_reachable=True))
    assert not eligible_late("N26", replace(failure, joe_node="X_ROOF", alternative_reachable=True))
    assert not eligible_late("N26", replace(failure, alternative_reachable=True,
                                            seen=failure.seen | {"N26:roof"}))
    assert eligible_late("N26", replace(failure, route_failure="garage", alternative_reachable=True,
                                        facts=failure.facts | {"route_failed:garage"},
                                        seen=failure.seen | {"N26:roof"}))
    assert not eligible_late("N27", replace(after_split, joe_node="F3_ISOLATION"))
    assert eligible_late("N27", replace(after_split, joe_node="F3_UTILITY"))
    assert eligible_late("N27", replace(after_split, joe_node="F3_ISOLATION",
                                        facts=frozenset({"isolation_known"})))

    for scene_id, (exit_node, flag) in ROUTE_ENDS.items():
        at_exit = replace(after_split, joe_node=exit_node)
        assert eligible_late(scene_id, at_exit)  # Visit alone does not grant a win.
        assert not can_close_route(scene_id, at_exit)
        assert can_close_route(scene_id, replace(at_exit, facts=frozenset({flag})))
        assert not can_close_route(scene_id, replace(after_split, joe_node="F4_START",
                                                     facts=frozenset({flag})))
    assert eligible_late("N32", replace(after_split, facts=frozenset({"dawn_victory"})))
    assert not eligible_late("N32", replace(after_split, difficulty="Punitive",
                                            facts=frozenset({"dawn_victory"})))
    assert eligible_late("N32", replace(after_split, difficulty="Punitive",
                                        facts=frozenset({"hide_victory"})))
    assert not eligible_late("N33", replace(after_split, facts=frozenset({"rare_confrontation"})))
    assert eligible_late("N33", replace(after_split, difficulty="Easy",
                                        facts=frozenset({"rare_confrontation"})))
    assert eligible_late("N33", replace(after_split, difficulty="Punitive",
                                        facts=frozenset({"rare_destruction"})))

    morgue = replace(after_split, joe_node="B1_MORGUE")
    body = frozenset({"joe_identified_thomas_body"})
    proof = body | {"joe_verified_batman_record"}
    assert not eligible_late("N34", replace(morgue, facts=body))
    assert eligible_late("N34", replace(morgue, facts=proof))
    assert eligible_late("N34", replace(morgue, joe_node="F3_SURGERY",
                                        facts=frozenset({"joe_directly_witnessed_thomas_death"})))
    assert not eligible_late("N34", replace(morgue, difficulty="Normal", facts=proof))
    assert not eligible_late("N34", replace(morgue, difficulty="Easy",
                                            facts=frozenset({"joe_directly_witnessed_thomas_death"})))

    jack = replace(after_split, difficulty="Punitive", joe_node="F2_CAMERAS",
                   companions={"jack": Actor("F2_CAMERAS")},
                   seen=after_split.seen | {"N15", "N25"},
                   facts=frozenset({"jack_act3_encounter", "jack_heard_thomas_or_injury",
                                    "jack_perceived_companion_risk"}))
    assert can_witness_late_clue("J2", jack, 3, True)
    assert not can_witness_late_clue("J2", replace(jack, difficulty="Hard"), 3, True)
    assert can_witness_late_clue("J3", jack, 4, True)
    assert not can_witness_late_clue("J3", replace(jack, companions={}), 4, True)
    assert not can_witness_late_clue("J4", replace(jack, facts=jack.facts | {"J3", "jack_final_encounter"}), 5, True)
    assert can_witness_late_clue("J4", replace(jack, facts=jack.facts | {"J2", "J3", "jack_final_encounter"}), 5, True)
    assert not can_witness_late_clue("J4", replace(jack, facts=jack.facts | {"J2", "J3", "jack_final_encounter"}), 4, True)
    return len(refs)


def verify_endings():
    unknown = {person: CompanionEvidence(last_contact_node="F3_RECOVERY")
               for person in DATA["epilogues"]}
    assert set(derive_outcomes(unknown, True).values()) == {"unknown"}
    assert resolve_from_evidence("Normal", unknown, True)["harley"] == "E_HARLEY_UNKNOWN"
    assert resolve_from_evidence("Hard", unknown, True)["thomas"] == "E_THOMAS_CODA_PLAYER_ONLY"
    assert resolve_from_evidence("Easy", unknown, True)["thomas"] == "E_THOMAS_UNCONFIRMED"
    saved = {**unknown, "harley": CompanionEvidence(together_at_joe_exit=True),
             "bartholomeu": CompanionEvidence(separate_escape_verified=True),
             "jack": CompanionEvidence(capture_verified=True)}
    assert derive_outcomes(saved, True) == {
        "harley": "together", "bartholomeu": "escaped_alone", "jack": "captured"}
    assert resolve_from_evidence("Hard", saved, True)["harley"] != \
           resolve_from_evidence("Punitive", saved, True)["harley"]
    dead_jack = {**saved, "jack": CompanionEvidence(death_verified=True)}
    assert resolve_from_evidence("Hard", dead_jack, True)["jack"] == "E_JACK_DEAD"
    for bad, escaped in (
        ({**saved, "harley": CompanionEvidence(together_at_joe_exit=True)}, False),
        ({**saved, "jack": CompanionEvidence(capture_verified=True, death_verified=True)}, True),
    ):
        try:
            derive_outcomes(bad, escaped)
        except ValueError:
            pass
        else:
            raise AssertionError("contradictory epilogue evidence accepted")
    try:
        resolve_from_evidence("Normal", unknown, True, joe_witnessed_thomas_proof=True)
    except ValueError:
        pass
    else:
        raise AssertionError("Thomas knowledge leaked through evidence resolver")


def verify_text_and_balance(graph):
    package = (ROOT / "../../02_GAME_DESIGN/Narrative/PACOTE_TEXTO_CAMPANHA_V1.md").read_text(encoding="utf-8")
    scene_ids = re.findall(r"^\| (N\d{2}) \|", package, flags=re.M)
    doc_ids = re.findall(r"^\| (DOC\d{2}) ·", package, flags=re.M)
    audio_ids = re.findall(r"^\| (AUD\d{2}) ·", package, flags=re.M)
    assert scene_ids == [f"N{i:02d}" for i in range(1, 35)]
    assert doc_ids == [f"DOC{i:02d}" for i in range(1, 17)]
    assert audio_ids == [f"AUD{i:02d}" for i in range(1, 9)]
    nodes = {node["id"] for node in graph["nodes"]}
    refs = set(re.findall(r"`((?:F[1-4]|B1|G|T|R|A|X|CAM|D|H|C)_[A-Z0-9_]+)`", package))
    devices = {d["id"] for group in graph["devices"].values() for d in group}
    assert refs <= nodes | devices, f"unknown text package references: {refs - (nodes | devices)}"

    puzzle = json.loads((ROOT / "puzzle_contract.json").read_text(encoding="utf-8"))
    assert puzzle["schema"] == 1
    garage = puzzle["garage"]
    assert len(garage["code"]) == 6 and garage["code"].isdigit()
    fragments = garage["fragments"]
    assert "".join(part["pair"] for part in fragments) == garage["code"]
    assert [part["document"] for part in fragments] == ["DOC01", "DOC02", "DOC03"]
    action_by_flag = {action["grants"]: action["at"] for action in graph["actions"]}
    for part in fragments:
        assert part["node"] == action_by_flag[part["flag"]]
        row = next(x for x in package.splitlines() if x.startswith(f"| {part['document']} ·"))
        assert part["node"] in row and f"**{part['pair']}**" in row
    for field in ("key_discovery", "safe_discovery"):
        item = garage[field]
        assert item["node"] == action_by_flag[item["flag"]]
        assert any(x.startswith(f"| {item['document']} ·") for x in package.splitlines())
    assert garage["order_document"] == garage["safe_discovery"]["document"]
    assert garage["code"] in package
    assert garage["correct_prior_knowledge_allowed"] and garage["wrong_attempts_never_lock"]
    unfamiliar = PuzzleState()
    assert not journal_fragments(unfamiliar)
    assert submit_code(unfamiliar, garage["code"], at_safe=False) == (unfamiliar, False)
    wrong, accepted = submit_code(unfamiliar, "000000", at_safe=True)
    assert not accepted and wrong.attempts == 1 and not wrong.safe_opened
    recovered, accepted = submit_code(wrong, garage["code"], at_safe=True)
    assert accepted and recovered.safe_opened and recovered.attempts == 2
    assert submit_code(recovered, garage["code"], at_safe=True) == (recovered, False)
    assert submit_code(unfamiliar, "１２３４５６", at_safe=True) == (unfamiliar, False)
    seen = PuzzleState(frozenset({"DOC02"}))
    assert journal_fragments(seen) == (("DOC02", fragments[1]["pair"]),)

    balance = json.loads((ROOT / "balance_initial.json").read_text(encoding="utf-8"))
    assert balance["schema"] == 1 and balance["status"] == "playtest_hypothesis"
    assert set(balance["modes"]) == set(DATA["difficulty"])
    assert balance["trait_order"] == ["trust", "fear", "stress", "loyalty",
                                      "selfPreservation", "relationship"]
    assert [balance["modes"][d]["dawn_minutes"] for d in
            ("Easy", "Normal", "Hard", "Punitive")] == [30, 90, 180, None]
    for profile in balance["modes"].values():
        assert profile["inventory_slots"] > 0
        assert set(profile["companions"]) == set(DATA["epilogues"])
        assert all(len(traits) == 6 and all(isinstance(v, int) and 0 <= v <= 100
                                            for v in traits)
                   for traits in profile["companions"].values())
    assert sum(balance["hard_starting_loadout_slots"].values()) == 8
    assert balance["modes"]["Hard"]["inventory_slots"] >= 8
    return len(refs)


def verify_banter(nodes):
    bank = BANTER_DATA["exchanges"]
    assert BANTER_DATA["schema"] == 1 and BANTER_DATA["locale"] == "pt-BR"
    assert [e["id"] for e in bank] == [f"BAN{i:02d}" for i in range(1, 43)]
    assert len(EXCHANGES) == len(bank)
    speakers = {"joe", "harley", "bartholomeu", "jack", "thomas"}
    story_scenes = {f"N{i:02d}" for i in range(1, 35)}
    evidence_facts = {"garage_camera_loop_observed", "camera_loop_observed_by_group",
                      "doc11_read_by_jack", "camera_live_observed_by_group",
                      "joe_delayed_harley_help", "harley_injured",
                      "bartholomeu_cooperative", "bartholomeu_cooperated_n24",
                      "helicopter_confirmed"}
    for e in bank:
        assert e["act"] in range(1, 6) and set(e["nodes"]) <= nodes
        assert set(e["actors"]) <= speakers
        assert set(e.get("after", ())) | set(e.get("before", ())) <= story_scenes
        assert set(e.get("requires_facts", ())) <= evidence_facts
        assert set(e.get("modes", DATA["difficulty"])) <= set(DATA["difficulty"])
        assert len(e["lines"]) >= 2
        assert {line["speaker"] for line in e["lines"]} <= set(e["actors"])
        assert all(line["text"].strip() for line in e["lines"])
        if "thomas" in e["actors"]:
            assert e["act"] == 1 and "N03" in e.get("before", ())
        assert not any(key.startswith("J") for key in e.get("grants", ()))
    assert all(sum(e["act"] == act for e in bank) >= 6 for act in range(1, 6))

    result = (ROOT / "../../02_GAME_DESIGN/Narrative/DIALOGOS_AMBIENTAIS_V1.md").read_text(encoding="utf-8")
    assert result == render_banter(), "render ambient dialogue after editing JSON"
    assert result.count("### BAN") == 42
    forbidden = re.compile(r"\b(?:porra|caralho|foda(?:-se)?|foder|puta que pariu|filho da puta|cacete)\b", re.I)
    for path in ("PACOTE_TEXTO_CAMPANHA_V1.md", "ROTEIRO_ATOS_1_2_RASCUNHO.md",
                 "ROTEIRO_ATOS_3_5_RASCUNHO.md", "DIALOGOS_AMBIENTAIS_V1.md"):
        manuscript = (ROOT / "../../02_GAME_DESIGN/Narrative" / path).read_text(encoding="utf-8")
        assert not forbidden.search(manuscript), f"strong profanity in {path}"
    gdd = (ROOT / "../../02_GAME_DESIGN/GDD/GDD_MASTER.md").read_text(encoding="utf-8")
    assert not forbidden.search(gdd), "strong profanity in GDD examples"
    assert "Joe matou os pais Wayne" in gdd and "No Difícil/Punitivo" in gdd

    party = {name: PresentActor("F4_START") for name in speakers - {"joe"}}
    opening = BanterFacts("Hard", 1, "F4_START", actors=party)
    assert eligible_banter("BAN01", opening)
    assert not eligible_banter("BAN01", replace(opening, seen_scenes=frozenset({"N03"})))
    assert not eligible_banter("BAN01", replace(opening, actors={**party, "thomas": PresentActor("F4_START", False)}))
    barth = BanterFacts("Normal", 2, "F3_RECOVERY", frozenset({"N03"}),
                        actors={"bartholomeu": PresentActor("F3_RECOVERY")})
    assert eligible_banter("BAN12", barth)
    assert not eligible_banter("BAN12", replace(barth, actors={}))
    assert not eligible_banter("BAN12", replace(barth, delivered=frozenset({"BAN12"})))
    assert not eligible_banter("BAN12", replace(barth, in_danger=True))
    camera = BanterFacts("Hard", 2, "F2_CAMERAS", frozenset({"N03"}),
                         actors={"jack": PresentActor("F2_CAMERAS")})
    assert not eligible_banter("BAN17", camera)
    assert eligible_banter("BAN17", replace(camera, facts=frozenset({"garage_camera_loop_observed"})))
    assert not eligible_banter("BAN17", replace(camera, main_scene_active=True,
                                                  facts=frozenset({"garage_camera_loop_observed"})))
    harley = BanterFacts("Hard", 4, "F3_RECOVERY", frozenset({"N23"}),
                         facts=frozenset({"joe_delayed_harley_help", "harley_injured"}),
                         actors={"harley": PresentActor("F3_RECOVERY")})
    assert eligible_banter("BAN29", harley)
    assert not eligible_banter("BAN30", harley)
    assert eligible_banter("BAN30", replace(harley, difficulty="Punitive"))
    assert not eligible_banter("BAN29", replace(harley, difficulty="Punitive"))
    assert DATA["world_truth"]["wayne_parents"] == "killed_by_joe_chill_years_earlier"
    return len(bank)


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
    later_refs = verify_later_scenes(nodes, devices)
    verify_endings()
    text_refs = verify_text_and_balance(graph)
    banter_count = verify_banter(nodes)

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
    print(f"NarrativeCore: 4 difficulties, 34 scenes, {banter_count} ambient exchanges, "
          "16 documents, 8 audio scripts, "
          f"{len(refs)} campaign/{early_refs} early/{later_refs} later/{text_refs} text "
          "map/device IDs; gates, puzzle, balance hypotheses and epilogues valid")


if __name__ == "__main__":
    verify()
