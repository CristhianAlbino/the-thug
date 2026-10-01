"""Meaningful integration scenarios for the authored director facade."""

from dataclasses import replace

from act12_rules import Actor
from director_runtime import (Observation, Progress, begin_banter, begin_scene,
                              banter_eligible, finish_banter, finish_scene,
                              inspect_document, scene_eligible)


def verify():
    opening = Observation("Hard", 1, "F4_START", companions={
        "thomas": Actor("F4_START"), "bartholomeu": Actor("F4_START")})
    progress = Progress()
    assert scene_eligible("N01", opening, progress)
    progress = begin_scene("N01", opening, progress)
    assert not scene_eligible("N01", opening, progress)
    progress = finish_scene("N01", progress)
    assert not scene_eligible("N01", opening, progress)
    assert banter_eligible("BAN01", opening, progress)
    progress = begin_banter("BAN01", opening, progress)
    assert not banter_eligible("BAN01", opening, progress)
    progress = finish_banter("BAN01", progress)
    assert not banter_eligible("BAN01", opening, progress)
    assert not banter_eligible("BAN01", replace(opening, in_danger=True), Progress())
    progress = inspect_document("DOC01", progress)
    assert "DOC01" in progress.documents

    gate = Observation("Hard", 5, "X_GARAGE")
    progressed = Progress(scenes=frozenset({"N03"}))
    assert not scene_eligible("N28", gate, progressed)  # Physical exit alone is insufficient.
    assert scene_eligible("N28", replace(gate, facts=frozenset({"garage_escaped"})), progressed)

    proof = Observation("Hard", 5, "B1_MORGUE", facts=frozenset({
        "joe_identified_thomas_body", "joe_verified_batman_record"}))
    assert scene_eligible("N34", proof, progressed)
    assert not scene_eligible("N34", replace(proof, difficulty="Normal"), progressed)
    assert not scene_eligible("N34", replace(proof, joe_node="F4_START"), progressed)

    # Merely placing a companion ID in the map does not grant a dialogue.
    scene = Observation("Hard", 2, "F3_ICU", companions={"harley": Actor("F3_RECOVERY")})
    assert not scene_eligible("N07", scene, progressed)
    assert scene_eligible("N07", replace(scene, companions={"harley": Actor("F3_ICU")}), progressed)
    failed = Observation("Hard", 4, "F3_CENTRAL", route_failure="roof",
                         alternative_reachable=True,
                         facts=frozenset({"route_failure_observed", "route_failed:roof"}))
    progress = begin_scene("N26", failed, progressed)
    assert progress.active_scene == "N26:roof"
    progress = finish_scene("N26", progress)
    assert "N26:roof" in progress.scenes
    assert not scene_eligible("N26", failed, progress)
    another = replace(failed, route_failure="garage",
                      facts=frozenset({"route_failure_observed", "route_failed:garage"}))
    assert scene_eligible("N26", another, progress)
    print("DirectorRuntime: integration scenarios passed")


if __name__ == "__main__":
    verify()
