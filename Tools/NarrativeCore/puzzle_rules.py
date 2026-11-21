"""Pure pre-Unreal interaction contract for the garage keypad.

MapCore treats an authored action as collected on arrival; the actual keypad
requires this explicit submission before the world emits RunRules.SafeOpened.
"""

from dataclasses import dataclass, replace
from pathlib import Path
from typing import FrozenSet

import json

CONTRACT = json.loads(Path(__file__).with_name("puzzle_contract.json").read_text(encoding="utf-8"))


@dataclass(frozen=True)
class PuzzleState:
    seen_documents: FrozenSet[str] = frozenset()
    attempts: int = 0
    safe_opened: bool = False


def journal_fragments(state: PuzzleState):
    """Only return fragment pairs Joe actually inspected; never autocomplete."""
    return tuple((p["document"], p["pair"])
                 for p in CONTRACT["garage"]["fragments"]
                 if p["document"] in state.seen_documents)


def submit_code(state: PuzzleState, entered: str, *, at_safe: bool):
    """Return (next state, accepted); a wrong guess never permanently locks a route."""
    if state.safe_opened or not at_safe or len(entered) != 6 or not entered.isascii() or not entered.isdigit():
        return state, False
    attempted = replace(state, attempts=state.attempts + 1)
    if entered == CONTRACT["garage"]["code"]:
        return replace(attempted, safe_opened=True), True
    return attempted, False
