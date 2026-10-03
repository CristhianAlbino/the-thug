#!/usr/bin/env python3
"""Index authored production cues; no recording or creative approval is implied."""
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
SOURCES = ("Tools/DirectorCore/runtime_ptbr.json",
           "Tools/NarrativeCore/banter_ptbr.json", "Tools/ProductionCore/requirements.json")


def build():
    inputs = [json.loads((ROOT / p).read_text(encoding="utf-8")) for p in SOURCES]
    runtime, banter, requirements = inputs
    cues = []
    for bi, block in enumerate(runtime["blocks"]):
        for ci, cue in enumerate(block["cues"]):
            cues.append({"id": cue["id"], "block": block["id"], "kind": cue["kind"],
                         "source": SOURCES[0], "pointer": f"/blocks/{bi}/cues/{ci}",
                         "content": cue, "status": "creative_review",
                         "production_file": None})
    for ei, exchange in enumerate(banter["exchanges"]):
        for li, line in enumerate(exchange["lines"]):
            cues.append({"id": f"{exchange['id']}:{li}", "block": exchange["id"],
                         "kind": "speech", "source": SOURCES[1],
                         "pointer": f"/exchanges/{ei}/lines/{li}", "content": line,
                         "status": "creative_review", "production_file": None})
    ids = [c["id"] for c in cues]
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate production cue ID")
    requirement_ids = [r["id"] for r in requirements["assets"]]
    if len(requirement_ids) != len(set(requirement_ids)):
        raise ValueError("Duplicate asset requirement ID")
    for requirement in requirements["assets"]:
        for source in requirement["sources"]:
            if not (ROOT / source).is_file():
                raise ValueError(f"Missing source: {source}")
        if requirement["status"] != "planned" or requirement["production_file"] is not None:
            raise ValueError("Requirements index must not imply produced assets")
    return {"schema": 1, "locale": "pt-BR", "status": "production_preparation",
            "sources_sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
                               for p in SOURCES},
            "summary": {"runtime_cues": len(cues), "kinds": dict(sorted(Counter(
                c["kind"] for c in cues).items())), "requirements": len(requirement_ids),
                "produced_files": 0},
            "cues": cues, "requirements": requirements["assets"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = json.dumps(build(), ensure_ascii=False, indent=2) + "\n"
    output = HERE / "production_manifest.json"
    if args.check:
        if not output.is_file() or output.read_text(encoding="utf-8") != expected:
            raise SystemExit("ProductionCore: manifest is stale; run build_manifest.py")
    else:
        output.write_text(expected, encoding="utf-8")
    data = json.loads(expected)
    print(f"ProductionCore: {data['summary']['runtime_cues']} indexed cues, "
          f"{data['summary']['requirements']} planned requirements; no produced assets claimed")


if __name__ == "__main__":
    main()
