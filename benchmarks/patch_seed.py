# benchmarks/patch_seed.py
import re
from pathlib import Path

def patch_thrml_seed(py_path: str, seed: int):
    p = Path(py_path)
    txt = p.read_text()

    # Replace first occurrence of jax.random.key(<number>)
    new_txt, n = re.subn(r"jax\.random\.key\(\s*\d+\s*\)", f"jax.random.key({seed})", txt, count=1)
    if n == 0:
        raise RuntimeError(f"Could not find 'jax.random.key(...)' in {py_path}")

    p.write_text(new_txt)

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python3 benchmarks/patch_seed.py <file_thrml.py> <seed>")
        raise SystemExit(1)
    patch_thrml_seed(sys.argv[1], int(sys.argv[2]))
