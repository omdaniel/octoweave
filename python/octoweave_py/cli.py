import argparse
import os
import sys

try:
    import pandas as pd  # type: ignore
except Exception as e:
    pd = None

try:
    import numpy as np  # type: ignore
except Exception as e:
    np = None

from . import OctoWeave, ChunkParams


def parse_args(argv=None):
    ap = argparse.ArgumentParser(description="OctoWeave pipeline CLI")
    g_in = ap.add_mutually_exclusive_group(required=True)
    g_in.add_argument("--points-csv", dest="points_csv", help="CSV with columns x,y,z")
    g_in.add_argument("--parquet", dest="parquet", help="Parquet with columns x,y,z")
    ap.add_argument("--columns", default="x,y,z", help="Column names for CSV/Parquet inputs")

    ap.add_argument("--n", type=int, default=2, help="Brick root dimension (n x n x n)")
    ap.add_argument("--outdir", default="examples_out/python", help="Output directory")
    ap.add_argument("--slice-z", type=int, default=0, help="Z slice index for rendering")
    ap.add_argument("--depth", type=int, default=-1, help="Depth for rendering (-1 = auto)")

    # Octo params
    ap.add_argument("--res", type=float, default=0.25)
    ap.add_argument("--emit-res", type=float, default=0.5)
    ap.add_argument("--max-depth-cap", type=int, default=12)

    # Policy
    ap.add_argument("--policy", choices=["uniform","quantiles","bands_mean"], default="quantiles")
    ap.add_argument("--level", type=int, default=6, help="Uniform policy level")
    ap.add_argument("--q-lo", type=float, default=0.2)
    ap.add_argument("--q-hi", type=float, default=0.8)
    ap.add_argument("--Llow", type=int, default=4)
    ap.add_argument("--Lmid", type=int, default=7)
    ap.add_argument("--Lhigh", type=int, default=10)
    ap.add_argument("--thresholds", default="0.3,0.6,0.85")
    ap.add_argument("--levels", default="4,7,9,11")
    return ap.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    os.makedirs(args.outdir, exist_ok=True)

    cols = [c.strip() for c in args.columns.split(",")]
    if len(cols) != 3:
        print("--columns must specify three names (x,y,z)", file=sys.stderr)
        return 2

    # Load points into numpy for run_pipeline
    if args.parquet:
        if pd is None:
            print("pandas is required for parquet input", file=sys.stderr)
            return 2
        df = pd.read_parquet(args.parquet)
        pts = df[cols].to_numpy(dtype=float, copy=False)
    else:
        if pd is None:
            # fallback simple CSV parsing without pandas
            import csv
            rows = []
            with open(args.points_csv, "r", newline="") as f:
                rdr = csv.DictReader(f)
                for row in rdr:
                    rows.append([float(row[cols[0]]), float(row[cols[1]]), float(row[cols[2]])])
            if np is None:
                pts = rows
            else:
                pts = np.asarray(rows, dtype=float)
        else:
            df = pd.read_csv(args.points_csv)
            pts = df[cols].to_numpy(dtype=float, copy=False)

    params = ChunkParams(res=args.res, emit_res=args.emit_res, max_depth_cap=args.max_depth_cap)

    # Prepare policy args
    policy_args = None
    if args.policy == "uniform":
        policy_args = args.level
    elif args.policy == "quantiles":
        policy_args = (args.q_lo, args.q_hi, args.Llow, args.Lmid, args.Lhigh)
    elif args.policy == "bands_mean":
        thr = [float(x) for x in args.thresholds.split(",") if x]
        lev = [int(x) for x in args.levels.split(",") if x]
        policy_args = (thr, lev)

    ow = OctoWeave()
    res = ow.run_pipeline(
        pts, params, n=args.n,
        csv_path=os.path.join(args.outdir, "pipeline_leaves.csv"),
        slice_z=args.slice_z, depth=args.depth,
        out_pgm=os.path.join(args.outdir, "pipeline_slice.pgm"),
        out_svg=os.path.join(args.outdir, "pipeline_hist.svg"),
        policy=args.policy, policy_args=policy_args,
    )
    print("pipeline result:", res)
    ow.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

