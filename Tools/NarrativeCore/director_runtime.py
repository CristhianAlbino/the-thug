"""Engine-independent director facade over authored scene and banter gates.

Only the Unreal adapter can claim a witnessed event, physical actor, sound,
line of sight or successful RunRules route. Eligibility never applies effects.
"""

from dataclasses import dataclass, field, replace
from typing import FrozenSet, Mapping

from act12_rules import Actor, Facts, SCENES as EARLY, eligible as eligible_early
from act35_rules import SCENES as LATE, eligible_late, can_close_route
from banter_rules import BanterFacts, PresentActor, EXCHANGES, eligible_banter
from variant_rules import scene_available


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
    if scene_id not in EARLY and scene_id not in LATE:
        raise ValueError("unknown authored scene")
    if progress.active_scene or progress.active_banter or scene_id in progress.scenes:
        return False
    scene = (EARLY if scene_id in EARLY else LATE)[scene_id]
    if scene["act"] > observation.act or not scene_available(
        scene_id, observation.difficulty, progress.witnessed_clues,
        act=observation.act
    ):
        return False
    facts = _facts(observation, progress)
    allowed = (eligible_early if scene_id in EARLY else eligible_late)(scene_id, facts)
    if scene_id in ("N28", "N29", "N30", "N31"):
        allowed = allowed and can_close_route(scene_id, facts)
    return allowed


def banter_eligible(exchange_id: str, observation: Observation, progress: Progress) -> bool:
    if exchange_id not in EXCHANGES:
        raise ValueError("unknown authored conversation")
    if progress.active_scene or progress.active_banter:
        return False
    actors = {name: PresentActor(actor.node, actor.active)
              for name, actor in observation.companions.items()}
    state = BanterFacts(
        difficulty=observation.difficulty, act=observation.act,
        joe_node=observation.joe_node, seen_scenes=progress.scenes,
        facts=progress.durable_facts | observation.facts,
        delivered=progress.banter, actors=actors,
        in_danger=observation.in_danger,
    )
    return eligible_banter(exchange_id, state)


def begin_scene(scene_id: str, observation: Observation, progress: Progress) -> Progress:
    if not scene_eligible(scene_id, observation, progress):
        raise ValueError("scene lacks a witnessed, spatially valid trigger")
    return replace(progress, active_scene=scene_id, active_scene_line=0)


def finish_scene(scene_id: str, progress: Progress) -> Progress:
    if progress.active_scene != scene_id or scene_id in progress.scenes:
        raise ValueError("scene was not begun or already completed")
    return replace(progress, scenes=progress.scenes | {scene_id},
                   active_scene="", active_scene_line=0)


def begin_banter(exchange_id: str, observation: Observation, progress: Progress) -> Progress:
    if not banter_eligible(exchange_id, observation, progress):
        raise ValueError("conversation is not currently possible")
    return replace(progress, active_banter=exchange_id, active_banter_line=0)


def finish_banter(exchange_id: str, progress: Progress) -> Progress:
    if progress.active_banter != exchange_id or exchange_id in progress.banter:
        raise ValueError("conversation was not begun or already delivered")
    return replace(progress, banter=progress.banter | {exchange_id},
                   active_banter="", active_banter_line=0)


def inspect_document(document_id: str, progress: Progress) -> Progress:
    if document_id not in {f"DOC{i:02d}" for i in range(1, 17)}:
        raise ValueError("unknown document")
    return replace(progress, documents=progress.documents | {document_id})


def inspect_audio(audio_id: str, progress: Progress) -> Progress:
    if audio_id not in {f"AUD{i:02d}" for i in range(1, 9)}:
        raise ValueError("unknown recording")
    return replace(progress, audio=progress.audio | {audio_id})
