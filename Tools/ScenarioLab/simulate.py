#!/usr/bin/env python3
"""Reproducible logical traversal + the real C++ RunRules state machine.

Travel and five-second interactions are hypotheses. No AI, animation, combat,
line of sight, companion decision or Unreal geometry is simulated here.
"""

import argparse
import json
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / "MapCore"))
from map_data import build  # noqa: E402
from validate_map import solve  # noqa: E402

GRAPH = build()
EDGES = {(e["a"], e["b"]): e for e in GRAPH["edges"]}
ACTIONS = {a["id"]: a for a in GRAPH["actions"]}
ENDING = {"garage": "GarageEscape", "roof": "RoofEscape",
          "tunnel": "TunnelEscape", "window": "WindowEscape"}
FAILURE = {"garage": "GarageBlocked", "tunnel": "TunnelIntercepted",
           "window": "WindowDiscovered"}


def edge_between(a, b):
    edge = EDGES.get((a, b)) or EDGES.get((b, a))
    if edge is None or (edge["one_way"] and edge["a"] != a):
        raise AssertionError(f"invalid traversal: {a} -> {b}")
    return edge


def make_scenario(name, seed):
    rng = random.Random(seed)
    if name == "random":
        target = rng.choice(tuple(ENDING))
        failed = rng.choice(("garage", "tunnel", "window"))
        if failed == target:
            failed = None
    elif name == "roof_timeout_window":
        target, failed = "window", "roof"
    elif "_failed_" in name:
        failed, target = name.split("_failed_", 1)
    else:
        target, failed = name, None
    if target not in ENDING or failed not in (None, *ENDING):
        raise ValueError(f"unknown scenario: {name}")
    if failed == target:
        raise ValueError("failed route cannot be the chosen ending")

    commands = []
    trace = []
    expected = []
    now = 0
    position = GRAPH["start"]
    known = set()

    def apply(event, at, node, action):
        commands.append(f"APPLY {at} {event}")
        expected.append(action != "probe_rejected")
        trace.append(dict(at=at, node=node, action=action, event=event))

    # A useful negative check: C++ must reject an exit without a gate.
    apply("GarageExitReached", 0, position, "probe_rejected")

    if failed == "roof":
        # First physically reach F1_RADIO, gather the batteries/interference,
        # then actually confirm. The starting point of fallback is that room.
        route = solve("roof")
        prefix = []
        for stop in route:
            prefix.append(stop)
            if "confirm_helicopter" in stop[1]:
                break
        now, position, known = walk(prefix, now, position, known, apply)
        assert "helicopter_confirmed" in known and position == "F1_RADIO"
        now += 301
        commands.append(f"WAIT {now}")
        trace.append(dict(at=now, node=position,
                          action="miss_helicopter_deadline", event=None))
    elif failed:
        apply(FAILURE[failed], 0, position, f"fail_{failed}")

    failures = {GRAPH["exits"][failed]["failure"]} if failed else set()
    witness = solve(target, failures=failures, start_node=position,
                    initial_flags=known)
    if witness is None:
        raise AssertionError(f"no fallback path: {name}")
    now, position, known = walk(witness, now, position, known, apply)
    if position != GRAPH["exits"][target]["node"]:
        raise AssertionError(f"wrong ending position: {name}")
    return dict(name=name, seed=seed, target=target, failed=failed,
                commands=commands, trace=trace, accepted=expected,
                time=now, ending=ENDING[target])


def walk(witness, now, position, known, apply):
    for node, gains in witness:
        if node != position:
            now += edge_between(position, node)["seconds"]
            position = node
        for action_id in gains:
            action = ACTIONS[action_id]
            if not set(action["requires"]) <= known:
                raise AssertionError(f"action not ready: {action_id}")
            now += 5  # Pure hypothesis; actual interactions must be timed in UE.
            known.add(action["grants"])
            if action["run_event"]:
                apply(action["run_event"], now, position, action_id)
    return now, position, known


def run(name, seed, driver):
    scenario = make_scenario(name, seed)
    data = "Hard\n" + "\n".join(scenario["commands"]) + "\n"
    response = subprocess.run([driver], input=data, text=True,
                              capture_output=True, check=True)
    state = json.loads(response.stdout)
    if state["accepted"] != scenario["accepted"]:
        raise AssertionError(f"C++ rejected a graph action: {name}: {state}")
    if state["ending"] != scenario["ending"]:
        raise AssertionError(f"wrong C++ ending: {name}: {state}")
    if scenario["failed"] and not state["failed"][scenario["failed"]]:
        raise AssertionError(f"failure not recorded by C++: {name}")
    if name == "roof_timeout_window" and state["deadline"] is None:
        raise AssertionError("helicopter deadline never started")
    return {"scenario": name, "seed": seed, "ending": state["ending"],
            "failed_route": scenario["failed"], "seconds_hypothesis": scenario["time"],
            "events": scenario["trace"], "deadline": state["deadline"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", required=True, help="compiled rules_driver")
    parser.add_argument("--scenario", default="all",
                        choices=("all", "garage", "roof", "tunnel", "window",
                                 "garage_failed_tunnel", "window_failed_garage",
                                 "roof_timeout_window", "random"))
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    names = ("garage", "roof", "tunnel", "window", "garage_failed_tunnel",
             "window_failed_garage", "roof_timeout_window", "random")
    for name in names if args.scenario == "all" else (args.scenario,):
        print(json.dumps(run(name, args.seed, args.driver), ensure_ascii=False,
                         sort_keys=True))


if __name__ == "__main__":
    main()
