from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATED_ROOT = ROOT / "artifacts" / "generated"


def generated_artifact_path(source: str | Path, suffix: str) -> Path:
    source_path = Path(source)

    if source_path.is_absolute():
        relative_path = source_path.relative_to(ROOT)
    else:
        relative_path = source_path

    return GENERATED_ROOT / relative_path.with_suffix(suffix)