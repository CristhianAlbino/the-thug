"""Engine-independent narrative gates; no story event is triggered by time alone.

These rules produce stable IDs for an Unreal director. They never decide the
physical fate of a companion, witness an event or move an actor themselves.
"""

import json
from pathlib import Path


DATA = json.loads(Path(__file__).with_name("variants.json").read_text(encoding="utf-8"))


def mode(difficulty):
    try:
        return DATA["difficulty"][difficulty]
    except KeyError as exc:
        raise ValueError(f"unknown difficulty: {difficulty}") from exc


def scene_available(scene_id, difficulty, witnessed_jack_clues=(),
                    jack_available=True, act=1):
    profile = mode(difficulty)
    if scene_id == DATA["thomas_reveal"]["scene"]:
        return profile["thomas"] == "confirmed"
    secret = DATA["jack_secret"]
    if scene_id in (*secret["clues"], secret["reveal"]):
        if profile["jack"] != "progressive_secret" or not jack_available:
            return False
        if act < secret["minimum_act"][scene_id]:
            return False
        seen = set(witnessed_jack_clues) & set(secret["clues"])
        if scene_id == secret["reveal"]:
            return (secret["late_clue"] in seen
                    and len(seen) >= secret["minimum_clues"])
    return True


def resolve_epilogue(difficulty, companion_outcomes,
                     thomas_scene_witnessed=False,
                     jack_revelation_witnessed=False):
    """Return outcome IDs; supplied facts must have been observed/persisted by play."""
    profile = mode(difficulty)
    if set(companion_outcomes) != set(DATA["epilogues"]):
        raise ValueError("one outcome is required for each companion")
    if thomas_scene_witnessed and profile["thomas"] != "confirmed":
        raise ValueError("Thomas reveal cannot be witnessed in this difficulty")
    if jack_revelation_witnessed and profile["jack"] != "progressive_secret":
        raise ValueError("Jack's secret cannot be witnessed in this difficulty")

    result = {}
    for person, status in companion_outcomes.items():
        if status not in DATA["companion_statuses"]:
            raise ValueError(f"unknown status for {person}: {status}")
        entry = DATA["epilogues"][person][status]
        result[person] = (entry[profile["joe_harley"]]
                          if person == "harley" and status == "together"
                          else entry)
    thomas = DATA["thomas_reveal"]
    result["thomas"] = (
        thomas["easy_normal"] if profile["thomas"] == "unconfirmed"
        else thomas["joe_witnessed"] if thomas_scene_witnessed
        else thomas["fallback"]
    )
    result["jack_secret"] = (
        "E_JACK_SECRET_REVEALED" if jack_revelation_witnessed
        else "E_JACK_SECRET_NOT_WITNESSED" if profile["jack"] == "progressive_secret"
        else "E_JACK_BASELINE"
    )
    return result
