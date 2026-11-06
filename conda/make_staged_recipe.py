"""Generate the staged-recipes copy of the NEP recipe.

Usage: python3 conda/make_staged_recipe.py VERSION SHA256 OUTDIR

Writes OUTDIR/recipes/nep/recipe.yaml and build.sh, with the
checkout source replaced by the release tarball and the development
version line replaced by VERSION, so the two recipes cannot drift.

Author: Edward Hartnett, Intelligent Data Design, Inc.
"""

import re
import shutil
import sys
from pathlib import Path

RECIPE_DIR = Path(__file__).resolve().parent / "recipe"

VERSION_RE = re.compile(r'^  version: "[^"]*"\n$')
BEGIN = "# BEGIN source"
END = "# END source"

SOURCE_URL = "https://github.com/Intelligent-Data-Design-Inc/NEP/archive/refs/tags/v${{ version }}.tar.gz"


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    version, sha256, outdir = sys.argv[1], sys.argv[2], Path(sys.argv[3])

    if not re.fullmatch(r"[0-9a-f]{64}", sha256):
        print(f"error: sha256 must be 64 lowercase hex characters: {sha256!r}", file=sys.stderr)
        return 1

    text = (RECIPE_DIR / "recipe.yaml").read_text()
    lines = text.splitlines(keepends=True)

    if sum(1 for line in lines if VERSION_RE.match(line)) != 1:
        print("error: expected exactly one quoted version line in recipe.yaml", file=sys.stderr)
        return 1
    if lines.count(BEGIN + "\n") != 1 or lines.count(END + "\n") != 1:
        print(f"error: expected exactly one {BEGIN!r} and one {END!r} marker in recipe.yaml",
              file=sys.stderr)
        return 1
    if lines.index(BEGIN + "\n") > lines.index(END + "\n"):
        print(f"error: {BEGIN!r} must come before {END!r} in recipe.yaml", file=sys.stderr)
        return 1

    out = []
    skipping = False
    for line in lines:
        if line == BEGIN + "\n":
            skipping = True
            continue
        if skipping:
            if line == END + "\n":
                skipping = False
                out.append(f"source:\n  url: {SOURCE_URL}\n  sha256: {sha256}\n")
            continue
        if VERSION_RE.match(line):
            line = f'  version: "{version}"\n'
        out.append(line)

    dest = outdir / "recipes" / "nep"
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "recipe.yaml").write_text("".join(out))
    shutil.copy2(RECIPE_DIR / "build.sh", dest / "build.sh")
    print(f"wrote {dest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
