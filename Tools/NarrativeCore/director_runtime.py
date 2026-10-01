"""Engine-independent director facade over authored scene and banter gates.

Only the Unreal adapter can claim a witnessed event, physical actor, sound,
line of sight or successful RunRules route. Eligibility never applies effects.
"""

from dataclasses import dataclass, field, replace
from math import isfinite
from typing import FrozenSet, Mapping
import json
from pathlib import Path

from act12_rules import Actor, Facts, SCENES as EARLY, eligible as eligible_early
from act35_rules import SCENES as LATE, ROUTE_ENDS, eligible_late, can_close_route
from banter_rules import BanterFacts, PresentActor, EXCHANGES, eligible_banter
from variant_rules import mode, scene_available


BANTER_COOLDOWN_SECONDS = 90
TRANSIENT_FACTS = frozenset(json.loads(
    Path(__file__).with_name("fact_lifetimes.json").read_text(encoding="utf-8")
)["transient_facts"])


@dataclass(frozen=True)
class Observation:
    difficulty: str
    act: int
    joe_node: str
    companions: Mapping[str, Actor] = field(default_factory=dict)
    facts: FrozenSet[str] = frozenset()
    hunter_has_evidence: bool = False
    hunter_can_reach: bool = False
    alternative_reachable: bool = False
    area_previously_visited: bool = False
    route_failure: str = ""
    in_danger: bool = False
    now_seconds: float = 0

    def __post_init__(self):
        mode(self.difficulty)
        if not 1 <= self.act <= 5 or not isfinite(self.now_seconds) or self.now_seconds < 0:
            raise ValueError("invalid act or world clock")


@dataclass(frozen=True)
class Progress:
    scenes: FrozenSet[str] = frozenset()
    banter: FrozenSet[str] = frozenset()
    documents: FrozenSet[str] = frozenset()
    audio: FrozenSet[str] = frozenset()
    witnessed_clues: FrozenSet[str] = frozenset()
    durable_facts: FrozenSet[str] = frozenset()
    active_scene: str = ""
    active_scene_line: int = 0
    active_banter: str = ""
    active_banter_line: int = 0
    last_banter_completed_at: float = -1

    def __post_init__(self):
        if self.durable_facts & (TRANSIENT_FACTS | {"J1", "J2", "J3", "J4"}):
            raise ValueError("current encounter observations cannot be persisted as facts")
        if (not isfinite(self.last_banter_completed_at)
                or (self.last_banter_completed_at < 0 and self.last_banter_completed_at != -1)):
            raise ValueError("invalid conversation clock")


def _facts(observation: Observation, progress: Progress) -> Facts:
    return Facts(
        difficulty=observation.difficulty,
        joe_node=observation.joe_node,
        seen=progress.scenes,
        facts=progress.durable_facts | observation.facts | progress.witnessed_clues,
        companions=observation.companions,
        hunter_has_evidence=observation.hunter_has_evidence,
        hunter_can_reach=observation.hunter_can_reach,
        alternative_reachable=observation.alternative_reachable,
        area_previously_visited=observation.area_previously_visited,
        route_failure=observation.route_failure,
    )


def scene_eligible(scene_id: str, observation: Observation, progress: Progress) -> bool:
    if scene_id.endswith(":exit") and scene_id[:-5] in ROUTE_ENDS:
        return (not progress.active_scene and scene_id not in progress.scenes
                and can_close_route(scene_id[:-5], _facts(observation, progress)))
    if scene_id not in EARLY and scene_id not in LATE:
        raise ValueError("unknown authored scene")
    if progress.active_scene or scene_id in progress.scenes:
        return False
    # Acts group spatial scenes; they are not locks on an otherwise valid route.
    # Jack's separate clue gates still require their explicit progression marks.
    if not scene_available(
        scene_id, observation.difficulty, progress.witnessed_clues,
        act=observation.act
    ):
        return False
    facts = _facts(observation, progress)
    allowed = (eligible_early if scene_id in EARLY else eligible_late)(scene_id, facts)
    if scene_id in ROUTE_ENDS and observation.joe_node == ROUTE_ENDS[scene_id][0]:
        allowed = False  # Already outside: use the confirmed coda, not approach VO.
    return allowed


def banter_eligible(exchange_id: str, observation: Observation, progress: Progress) -> bool:
    if exchange_id not in EXCHANGES:
        raise ValueError("unknown authored conversation")
    if progress.active_scene or progress.active_banter:
        return False
    if (progress.last_banter_completed_at >= 0 and
            observation.now_seconds - progress.last_banter_completed_at < BANTER_COOLDOWN_SECONDS):
        return False
    return _banter_context_valid(exchange_id, observation, progress)


def _banter_context_valid(exchange_id: str, observation: Observation, progress: Progress) -> bool:
    actors = {name: PresentActor(actor.node, actor.active)
              for name, actor in observation.companions.items()}
    state = BanterFacts(
        difficulty=observation.difficulty, act=observation.act,
        joe_node=observation.joe_node, seen_scenes=progress.scenes,
        facts=progress.durable_facts | observation.facts,
        delivered=progress.banter, actors=actors,
        in_danger=observation.in_danger,
        main_scene_active=bool(progress.active_scene),
    )
    return eligible_banter(exchange_id, state)


def begin_scene(scene_id: str, observation: Observation, progress: Progress) -> Progress:
    if not scene_eligible(scene_id, observation, progress):
        raise ValueError("scene lacks a witnessed, spatially valid trigger")
    completed_id = (f"N26:{observation.route_failure}"
                    if scene_id == "N26" else scene_id)
    return replace(progress, active_scene=completed_id, active_scene_line=0)


def finish_scene(scene_id: str, progress: Progress) -> Progress:
    completed_id = (progress.active_scene if scene_id == "N26" and
                    progress.active_scene.startswith("N26:") else scene_id)
    if not completed_id or progress.active_scene != completed_id or completed_id in progress.scenes:
        raise ValueError("scene was not begun or already completed")
    return replace(progress, scenes=progress.scenes | {completed_id},
                   active_scene="", active_scene_line=0)


def begin_banter(exchange_id: str, observation: Observation, progress: Progress) -> Progress:
    if not banter_eligible(exchange_id, observation, progress):
        raise ValueError("conversation is not currently possible")
    return replace(progress, active_banter=exchange_id, active_banter_line=0)


def banter_can_resume(observation: Observation, progress: Progress) -> bool:
    return (bool(progress.active_banter)
            and _banter_context_valid(progress.active_banter, observation, progress)
            and observation.now_seconds >= progress.last_banter_completed_at)


def cancel_banter(progress: Progress) -> Progress:
    """Drop an interrupted exchange without claiming that its lines were heard."""
    return replace(progress, active_banter="", active_banter_line=0)


def finish_banter(exchange_id: str, observation: Observation, progress: Progress) -> Progress:
    if progress.active_banter != exchange_id or not banter_can_resume(observation, progress):
        raise ValueError("conversation is interrupted, absent or already delivered")
    return replace(progress, banter=progress.banter | {exchange_id},
                   active_banter="", active_banter_line=0,
                   last_banter_completed_at=observation.now_seconds)


def inspect_document(document_id: str, observation: Observation, progress: Progress) -> Progress:
    if document_id not in {f"DOC{i:02d}" for i in range(1, 17)}:
        raise ValueError("unknown document")
    if document_id == "DOC16" and observation.difficulty not in {"Hard", "Punitive"}:
        raise ValueError("document is unavailable in this difficulty")
    return replace(progress, documents=progress.documents | {document_id})


def inspect_audio(audio_id: str, observation: Observation, progress: Progress) -> Progress:
    if audio_id not in {f"AUD{i:02d}" for i in range(1, 9)}:
        raise ValueError("unknown recording")
    if ((audio_id == "AUD07" and observation.difficulty != "Punitive")
            or (audio_id == "AUD08" and observation.difficulty not in {"Hard", "Punitive"})):
        raise ValueError("recording is unavailable in this difficulty")
    return replace(progress, audio=progress.audio | {audio_id})
