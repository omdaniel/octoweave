import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from octoweave_py import OctoWeave, ChunkParams


def ensure_sample_parquet(path: str, rows: int = 64):
    try:
        import pandas as pd  # type: ignore
    except Exception as e:
        print("pandas is required to write/read parquet (install pandas + pyarrow)")
        raise
    import random
    random.seed(42)
    xs = [0.2 + random.random() + (2.0 if i % 2 else 0.0) for i in range(rows)]
    ys = [0.2 + random.random() for _ in range(rows)]
    zs = [0.2 + random.random() for _ in range(rows)]
    df = pd.DataFrame({"x": xs, "y": ys, "z": zs})
    os.makedirs(os.path.dirname(path), exist_ok=True)
    try:
        df.to_parquet(path)  # default engine
    except Exception:
        # Fallback to fastparquet if available
        df.to_parquet(path, engine="fastparquet")


def main():
    outdir = os.path.join("examples_out", "python")
    os.makedirs(outdir, exist_ok=True)
    pq = os.path.join(outdir, "points.parquet")
    if not os.path.exists(pq):
        ensure_sample_parquet(pq)

    p = ChunkParams(res=0.25, emit_res=0.5, max_depth_cap=12)
    n = 2

    # Build from parquet, write CSV, then render a slice
    ow = OctoWeave()
    ow.build_hierarchy_from_parquet(pq, p, columns=("x","y","z"))
    csv = os.path.join(outdir, "parquet_leaves.csv")
    ow.write_csv(csv)
    pgm = os.path.join(outdir, "parquet_slice.pgm")
    svg = os.path.join(outdir, "parquet_hist.svg")
    rc = ow.render_csv(csv, slice_z=0, depth=-1, out_pgm=pgm, out_svg=svg)
    print(f"render rc={rc}; wrote {pgm} and {svg}")
    ow.close()


if __name__ == "__main__":
    main()

