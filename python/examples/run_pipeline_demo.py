import os
import sys
import random
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from octoweave_py import OctoWeave, ChunkParams


def make_points(num=50):
    random.seed(123)
    pts = []
    # cluster A
    for _ in range(num // 2):
        pts.append((0.2 + random.random(), 0.2 + random.random(), 0.2 + random.random()))
    # cluster B shifted in +x
    for _ in range(num - num // 2):
        pts.append((2.0 + random.random(), 0.2 + random.random(), 0.2 + random.random()))
    return pts


def main():
    outdir = os.path.join("examples_out", "python")
    os.makedirs(outdir, exist_ok=True)

    p = ChunkParams(res=0.25, emit_res=0.5, max_depth_cap=12)
    pts = make_points(60)
    n = 2

    # Quantiles policy: derive uniform level from per-tree quantile levels
    ow = OctoWeave()
    res1 = ow.run_pipeline(
        pts, p, n=n,
        csv_path=os.path.join(outdir, "quantiles_leaves.csv"),
        slice_z=0, depth=0,
        out_pgm=os.path.join(outdir, "quantiles_slice.pgm"),
        out_svg=os.path.join(outdir, "quantiles_hist.svg"),
        policy="quantiles", policy_args=(0.2, 0.8, 4, 7, 10),
    )
    print("quantiles policy result:", res1)
    ow.close()

    # Bands by mean probability policy
    ow2 = OctoWeave()
    res2 = ow2.run_pipeline(
        pts, p, n=n,
        csv_path=os.path.join(outdir, "bands_leaves.csv"),
        slice_z=0, depth=0,
        out_pgm=os.path.join(outdir, "bands_slice.pgm"),
        out_svg=os.path.join(outdir, "bands_hist.svg"),
        policy="bands_mean", policy_args=([0.3, 0.6, 0.85], [4, 7, 9, 11]),
    )
    print("bands_mean policy result:", res2)
    ow2.close()


if __name__ == "__main__":
    main()
