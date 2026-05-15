from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

ROOT = Path(__file__).resolve().parents[1]
PYTHON = sys.executable


def find_compiler() -> Path:
    candidates = [
        ROOT / "build" / ("thermolangc.exe" if os.name == "nt" else "thermolangc"),
        ROOT / "build" / "Release" / "thermolangc.exe",
        ROOT / "build" / "Debug" / "thermolangc.exe",
        ROOT / "build" / "RelWithDebInfo" / "thermolangc.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


COMPILER = find_compiler()


def ensure_compiler() -> None:
    if not COMPILER.exists():
        raise FileNotFoundError(
            f"Compiler not found at {COMPILER}. Build first: cmake -S . -B build && cmake --build build"
        )


def run_cmd(
    cmd: Sequence[str],
    *,
    cwd: Path = ROOT,
    env: Optional[dict] = None,
    timeout: int = 300,
) -> subprocess.CompletedProcess:
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)

    return subprocess.run(
        list(cmd),
        cwd=str(cwd),
        env=merged_env,
        text=True,
        capture_output=True,
        timeout=timeout,
    )


def compile_thermo(
    source: Path,
    *,
    target: str = "sim",
    no_opts: bool = False,
    env: Optional[dict] = None,
    timeout: int = 300,
) -> Tuple[Path, str]:
    ensure_compiler()

    cmd = [str(COMPILER), str(source), f"--target={target}"]
    if no_opts:
        cmd.append("--no-opts")

    res = run_cmd(cmd, env=env, timeout=timeout)
    log = res.stdout + res.stderr

    if res.returncode != 0:
        raise RuntimeError(f"Compilation failed for {source}\n{log}")

    suffix = "_sim.py" if target == "sim" else f"_{target}.py"
    out_file = ROOT / f"{source.stem}{suffix}"

    if not out_file.exists():
        raise FileNotFoundError(
            f"Expected generated file {out_file} was not created. Compiler log:\n{log}"
        )

    return out_file, log


def parse_final_state(text: str) -> List[int]:
    match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", text, flags=re.DOTALL)
    if not match:
        raise ValueError("Could not parse [FINAL_STATE] from simulation output")

    return [int(float(x.strip())) for x in match.group(1).split(",") if x.strip()]


def parse_reported_energy(text: str) -> Optional[float]:
    patterns = [
        r"\[FINAL_ENERGY\]:\s*([-+0-9.eE]+)",
        r"Best Energy Found:\s*([-+0-9.eE]+)",
    ]

    for pattern in patterns:
        match = re.search(pattern, text)
        if match:
            return float(match.group(1))

    return None


def patch_generated_sim(
    py_file: Path,
    *,
    seed: int,
    initial_state: Optional[Sequence[int]] = None,
) -> None:
    """
    Patch generated Python for deterministic experiment-level validation.

    Current SimulationCodeGenerator does not expose seed/initial-state arguments.
    This keeps the compiler untouched while making experiments reproducible.
    """
    text = py_file.read_text(encoding="utf-8")

    if "np.random.seed(" not in text:
        text = text.replace(
            "import sys\n\n",
            f"import sys\nnp.random.seed({seed})\n\n",
            1,
        )
    else:
        text = re.sub(
            r"np\.random\.seed\([^)]*\)",
            f"np.random.seed({seed})",
            text,
            count=1,
        )

    if initial_state is not None:
        init = ", ".join(str(int(v)) for v in initial_state)

        text = re.sub(
            r"initial_state=\[1\.0\]\s*\*\s*\d+",
            f"initial_state=[{init}]",
            text,
        )
        text = re.sub(
            r"initial_state=\[1\]\s*\*\s*\d+",
            f"initial_state=[{init}]",
            text,
        )

    py_file.write_text(text, encoding="utf-8")