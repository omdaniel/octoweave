import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from octoweave_py import OctoWeave


def main():
    if len(sys.argv) < 2:
        print("Usage: python render_csv.py leaves.csv [slice_z] [depth] [out.pgm] [out.svg]", file=sys.stderr)
        return 2
    csv = sys.argv[1]
    slice_z = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    depth = int(sys.argv[3]) if len(sys.argv) > 3 else -1
    outpgm = sys.argv[4] if len(sys.argv) > 4 else os.path.join("examples_out", "python", "render_slice.pgm")
    outsvg = sys.argv[5] if len(sys.argv) > 5 else os.path.join("examples_out", "python", "render_hist.svg")
    os.makedirs(os.path.dirname(outpgm), exist_ok=True)
    ow = OctoWeave()
    rc = ow.render_csv(csv, slice_z, depth, outpgm, outsvg)
    print(f"render rc={rc}; wrote {outpgm} and {outsvg}")
    return rc


if __name__ == "__main__":
    raise SystemExit(main())

