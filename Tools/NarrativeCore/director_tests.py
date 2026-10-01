"""Meaningful integration scenarios for the authored director facade."""

from dataclasses import replace

from act12_rules import Actor
from director_runtime import (Observation, Progress, begin_banter, begin_scene,
                              banter_eligible, finish_banter, finish_scene,
                              inspect_document, inspect_audio, scene_eligible,
                              banter_can_resume, cancel_banter)


def rejected(action):
    try:
        action()
    except ValueError:
        return
    raise AssertionError("invalid narrative transition was accepted")


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
    progress = finish_banter("BAN01", opening, progress)
    assert not banter_eligible("BAN01", opening, progress)
    assert not banter_eligible("BAN01", replace(opening, in_danger=True), Progress())
    progress = inspect_document("DOC01", opening, progress)
    assert "DOC01" in progress.documents

    gate = Observation("Hard", 5, "X_GARAGE")
    progressed = Progress(scenes=frozenset({"N03"}))
    assert not scene_eligible("N28:exit", gate, progressed)
    assert scene_eligible("N28:exit", replace(gate, facts=frozenset({"garage_escaped"})), progressed)
    # Prepare a reachable route before the nominal final act; do not claim victory.
    approach = replace(gate, joe_node="G_CONTROL", act=2)
    prepared = finish_scene("N28", begin_scene("N28", approach, progressed))
    assert "N28:exit" not in prepared.scenes
    assert not scene_eligible("N28:exit", approach, prepared)
    escaped = replace(gate, facts=frozenset({"garage_escaped"}))
    completed = finish_scene("N28:exit", begin_scene("N28:exit", escaped, prepared))
    assert not scene_eligible("N28:exit", escaped, completed)
    assert not scene_eligible("N28", escaped, progressed)

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

    # Main scenes suspend optional speech, and resumption rechecks the speakers.
    chatter = begin_banter("BAN01", opening, Progress(scenes=frozenset({"N01"})))
    interrupted = replace(opening, in_danger=True, now_seconds=10)
    assert not banter_can_resume(interrupted, chatter)
    rejected(lambda: finish_banter("BAN01", interrupted, chatter))
    absent = replace(opening, companions={})
    assert not banter_can_resume(absent, chatter)
    main = replace(opening, joe_node="F4_OBSERVATION")
    assert scene_eligible("N02", main, chatter)
    paused = begin_scene("N02", main, replace(chatter, active_banter_line=1))
    assert paused.active_banter_line == 1 and not banter_can_resume(opening, paused)
    resumed = finish_scene("N02", paused)
    assert banter_can_resume(opening, resumed)
    delivered = finish_banter("BAN01", replace(opening, now_seconds=20), resumed)
    # A different eligible conversation is cooled down too; replaying the old ID
    # is independently forbidden by its delivered ledger.
    candidate = Progress(scenes=frozenset({"N01"}), last_banter_completed_at=20)
    assert not banter_eligible("BAN01", replace(opening, now_seconds=109), candidate)
    assert banter_eligible("BAN01", replace(opening, now_seconds=110), candidate)
    assert delivered.last_banter_completed_at == 20
    assert not cancel_banter(chatter).active_banter
    assert "BAN01" not in cancel_banter(chatter).banter

    for difficulty in ("Easy", "Normal"):
        common = replace(opening, difficulty=difficulty)
        rejected(lambda: inspect_document("DOC16", common, Progress()))
        rejected(lambda: inspect_audio("AUD08", common, Progress()))
    rejected(lambda: inspect_audio("AUD07", opening, Progress()))
    assert "AUD07" in inspect_audio("AUD07", replace(opening, difficulty="Punitive"), Progress()).audio
    rejected(lambda: Progress(durable_facts=frozenset({"jack_final_encounter"})))
    rejected(lambda: finish_scene("", Progress()))
    print("DirectorRuntime: integration scenarios passed")


if __name__ == "__main__":
    verify()
