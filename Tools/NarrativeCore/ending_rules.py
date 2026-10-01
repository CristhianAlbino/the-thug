"""Resolve epilogues from verified observations, not from absence or abandonment."""

from dataclasses import dataclass
from typing import Mapping

from variant_rules import DATA, resolve_epilogue


@dataclass(frozen=True)
class CompanionEvidence:
    together_at_joe_exit: bool = False
    separate_escape_verified: bool = False
    capture_verified: bool = False
    death_verified: bool = False
    last_contact_node: str = ""


def derive_outcomes(evidence: Mapping[str, CompanionEvidence], joe_escaped: bool):
    if set(evidence) != set(DATA["epilogues"]):
        raise ValueError("one evidence record is required for each companion")
    outcomes = {}
    for person, record in evidence.items():
        verified = (
            record.together_at_joe_exit,
            record.separate_escape_verified,
            record.capture_verified,
            record.death_verified,
        )
        if sum(verified) > 1:
            raise ValueError(f"contradictory verified outcomes for {person}")
        if record.together_at_joe_exit and not joe_escaped:
            raise ValueError(f"{person} cannot escape together if Joe did not escape")
        outcomes[person] = next(
            (status for is_true, status in zip(verified, (
                "together", "escaped_alone", "captured", "dead"
            )) if is_true),
            "unknown",
        )
    return outcomes


def resolve_from_evidence(difficulty, evidence, joe_escaped,
                          joe_witnessed_thomas_proof=False,
                          joe_witnessed_jack_revelation=False):
    return resolve_epilogue(
        difficulty,
        derive_outcomes(evidence, joe_escaped),
        thomas_scene_witnessed=joe_witnessed_thomas_proof,
        jack_revelation_witnessed=joe_witnessed_jack_revelation,
    )
