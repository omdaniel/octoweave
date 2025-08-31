import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from octoweave_py import OctoWeave, ChunkParams
from octoweave_py.viz import render_slice_png, render_max_projection_png, render_montage


def main():
    outdir = os.path.join("examples_out", "pyviz_demo")
    os.makedirs(outdir, exist_ok=True)

    # Build a small CSV via the pipeline
    ow = OctoWeave()
    pts = [(0.2,0.2,0.2),(1.1,0.2,0.2),(0.5,1.3,0.2),(2.2,0.1,0.3),(2.8,1.7,0.2)]
    p = ChunkParams(res=0.25, emit_res=0.5, max_depth_cap=12)
    res = ow.run_pipeline(pts, p, n=2, csv_path=os.path.join(outdir, 'leaves.csv'))
    csv = res['csv']
    print('pipeline CSV:', csv)

    # Single slice with grid
    render_slice_png(csv, os.path.join(outdir, 'slice_z0_viridis_grid.png'), z=0, colormap='viridis', grid=True)

    # Single slice magma without grid
    render_slice_png(csv, os.path.join(outdir, 'slice_z0_magma.png'), z=0, colormap='magma', grid=False)

    # Max projection across a z range
    render_max_projection_png(csv, os.path.join(outdir, 'maxproj_z0_2.png'), (0,2), colormap='plasma', grid=True)

    # Montage of multiple z slices
    render_montage(csv, os.path.join(outdir, 'montage.png'), zs=[0,1,2,3], colormap='cividis', ncols=2, grid=True)

    print('Wrote outputs to', outdir)


if __name__ == '__main__':
    main()

