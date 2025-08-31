import os
import sys
import math
import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from octoweave_py import OctoWeave, ChunkParams
from octoweave_py.viz import (
    render_slice_png,
    render_max_projection_png,
    render_montage,
    render_overlay_slices,
)


def sample_mvn_mixture(n: int, rng: np.random.Generator, domain=(0, 8, 0, 8, 0, 4)):
    weights = np.array([0.55, 0.3, 0.15])
    weights = weights / weights.sum()
    means = np.array([
        [2.0, 2.0, 1.0],
        [5.5, 2.5, 1.5],
        [3.5, 6.0, 2.5],
    ])
    covs = np.array([
        [[0.20, 0.05, 0.00], [0.05, 0.20, 0.00], [0.00, 0.00, 0.10]],
        [[0.15, -0.04, 0.00], [-0.04, 0.25, 0.00], [0.00, 0.00, 0.10]],
        [[0.30, 0.10, 0.00], [0.10, 0.15, 0.00], [0.00, 0.00, 0.12]],
    ])
    # Mixture assignment
    comps = rng.choice(len(weights), size=n, p=weights)
    pts = np.zeros((n, 3), dtype=float)
    for k in range(len(weights)):
        mask = comps == k
        cnt = int(mask.sum())
        if cnt:
            pts[mask] = rng.multivariate_normal(means[k], covs[k], size=cnt)
    # Rejection toward periphery to create sparse/empty borders
    xmin, xmax, ymin, ymax, zmin, zmax = domain
    cx, cy, cz = (0.5*(xmin+xmax), 0.5*(ymin+ymax), 0.5*(zmin+zmax))
    sx, sy, sz = (0.5*(xmax-xmin), 0.5*(ymax-ymin), 0.5*(zmax-zmin))
    kept = []
    for p in pts:
        r2 = ((p[0]-cx)/sx)**2 + ((p[1]-cy)/sy)**2 + ((p[2]-cz)/sz)**2
        # acceptance decays with radius; near periphery mostly rejected
        acc = math.exp(-3.0 * max(0.0, r2-0.4))
        if rng.random() < acc and xmin <= p[0] <= xmax and ymin <= p[1] <= ymax and zmin <= p[2] <= zmax:
            kept.append(p)
    if not kept:
        return np.empty((0,3), dtype=float)
    return np.asarray(kept, dtype=float)


def main():
    outdir = os.path.join("examples_out", "mvn_viz")
    os.makedirs(outdir, exist_ok=True)

    rng = np.random.default_rng(12345)
    # Dense population
    pts_dense = sample_mvn_mixture(6000, rng)
    # Sparse population for overlay
    pts_sparse = sample_mvn_mixture(1500, rng)

    # Build pipelines
    p = ChunkParams(res=0.25, emit_res=0.5, max_depth_cap=12)
    ow = OctoWeave(); res_dense = ow.run_pipeline(pts_dense, p, n=2, csv_path=os.path.join(outdir, 'leaves_dense.csv'))
    ow2 = OctoWeave(); res_sparse = ow2.run_pipeline(pts_sparse, p, n=2, csv_path=os.path.join(outdir, 'leaves_sparse.csv'))

    csv_d = res_dense['csv']
    csv_s = res_sparse['csv']

    # Slice with occupancy preset
    render_slice_png(csv_d, os.path.join(outdir, 'slice_occ5_z1.png'), z=1, preset='occ5', legend=True, grid=True)

    # Max and mean projections across a z-range
    render_max_projection_png(csv_d, os.path.join(outdir, 'maxproj_z0_3.png'), (0,3), preset='occ3', legend=True, grid=True)
    render_max_projection_png(csv_d, os.path.join(outdir, 'meanproj_z0_3.png'), (0,3), preset='occ_heat', legend=False, grid=False, op='mean')

    # Montage of several slices
    render_montage(csv_d, os.path.join(outdir, 'montage_z0_3.png'), zs=[0,1,2,3], colormap='viridis', ncols=2, grid=True)

    # Overlay dense vs sparse at a representative slice
    render_overlay_slices(csv_d, csv_s, os.path.join(outdir, 'overlay_dense_sparse_z1.png'), z=1,
                          colormap_a='Greens', colormap_b='Reds', alpha_a=0.6, alpha_b=0.6, grid=True)

    print('Wrote visualization outputs to', outdir)


if __name__ == '__main__':
    main()

