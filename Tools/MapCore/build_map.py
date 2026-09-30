#!/usr/bin/env python3
"""Export the authored map in deterministic JSON for Unreal data ingestion."""

import json
from pathlib import Path

from map_data import build


if __name__ == "__main__":
    target = Path(__file__).with_name("hospital_graph.json")
    target.write_text(json.dumps(build(), ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    print(target)
