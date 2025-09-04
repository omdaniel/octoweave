#!/usr/bin/env python3
import argparse
import os
import sys
import random

# Ensure local Python package is discoverable when running from repo root
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "python"))
from octoweave_py import OctoWeave, ChunkParams  # type: ignore


def make_blob_points(n: int, spread: float = 0.5) -> list[tuple[float, float, float]]:
    random.seed(123)
    pts: list[tuple[float, float, float]] = []
    centers = [
        (0.0, 0.0, 0.0),
        (3.0, 0.5, 0.5),
        (1.5, 2.5, 1.0),
    ]
    per = max(1, n // len(centers))
    for (cx, cy, cz) in centers:
        for _ in range(per):
            pts.append(
                (
                    cx + (random.random() - 0.5) * spread,
                    cy + (random.random() - 0.5) * spread,
                    cz + (random.random() - 0.5) * spread,
                )
            )
    return pts


def main():
    ap = argparse.ArgumentParser(description="Union/hierarchy demo using OctoWeave ctypes wrapper")
    ap.add_argument("--mode", choices=["blobs"], default="blobs")
    ap.add_argument("--n", type=int, default=200, help="number of sample points (approx)")
    ap.add_argument("--res", type=float, default=0.5, help="chunker base resolution")
    ap.add_argument("--taus", type=float, nargs="+", default=[0.5], help="one or more tau thresholds")
    ap.add_argument("--outdir", type=str, default=os.path.join("examples_out", "demo_union"))
    args = ap.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    if args.mode != "blobs":
        raise SystemExit("Only --mode blobs is supported in this demo")

    pts = make_blob_points(args.n)

    # Build once per tau; write CSV for inspection
    results = []
    for tau in args.taus:
        p = ChunkParams(res=args.res, emit_res=-1.0, max_depth_cap=12)
        ow = OctoWeave()
        ow.build_hierarchy_from_points(pts, p, tau=tau, p_unknown=0.5, base_depth=1)
        csv_path = os.path.join(args.outdir, f"leaves_tau_{tau:.2f}.csv")
        rc = ow.write_csv(csv_path)
        if rc != 0:
            ow.close()
            raise RuntimeError(f"write_csv failed rc={rc}")
        results.append(csv_path)
        ow.close()

    print("Wrote:")
    for r in results:
        print(" -", r)


if __name__ == "__main__":
    main()
