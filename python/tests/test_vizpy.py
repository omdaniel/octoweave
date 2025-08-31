import os
import sys
import shutil
import matplotlib
matplotlib.use('Agg')

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from octoweave_py import OctoWeave, ChunkParams
from octoweave_py.viz import (
    render_slice_png,
    render_max_projection_png,
    render_montage,
    render_overlay_slices,
    export_slice_grid,
)


def setup_module():
    os.makedirs('pyviz_tmp', exist_ok=True)


def teardown_module():
    shutil.rmtree('pyviz_tmp', ignore_errors=True)


def _pipeline_csv(path):
    ow = OctoWeave()
    pts = [(0.2,0.2,0.2),(1.1,0.2,0.2),(0.5,1.3,0.2),(2.2,0.1,0.3),(2.8,1.7,0.2)]
    p = ChunkParams(res=0.25, emit_res=0.5, max_depth_cap=12)
    res = ow.run_pipeline(pts, p, n=2, csv_path=path)
    ow.close()
    return res['csv']


def test_render_slice_png():
    csv = _pipeline_csv('pyviz_tmp/leaves.csv')
    out = 'pyviz_tmp/slice.png'
    render_slice_png(csv, out, z=0, depth=None, colormap='viridis', grid=True, discrete_levels=[0.0,0.25,0.5,0.75,1.0], legend=True)
    assert os.path.exists(out) and os.path.getsize(out) > 0


def test_render_max_projection_png():
    csv = _pipeline_csv('pyviz_tmp/leaves2.csv')
    out = 'pyviz_tmp/maxproj.png'
    render_max_projection_png(csv, out, (0,3), depth=None, colormap='plasma', grid=False, op='mean')
    assert os.path.exists(out) and os.path.getsize(out) > 0


def test_render_montage():
    csv = _pipeline_csv('pyviz_tmp/leaves3.csv')
    out = 'pyviz_tmp/montage.png'
    render_montage(csv, out, zs=[0,1,2,3], colormap='cividis', ncols=2, grid=True)
    assert os.path.exists(out) and os.path.getsize(out) > 0


def test_render_overlay_and_export_grid():
    csv = _pipeline_csv('pyviz_tmp/leaves4.csv')
    out = 'pyviz_tmp/overlay.png'
    render_overlay_slices(csv, csv, out, z=0, colormap_a='Greens', colormap_b='Reds', alpha_a=0.5, alpha_b=0.5)
    assert os.path.exists(out) and os.path.getsize(out) > 0
    grid_out = 'pyviz_tmp/grid.csv'
    export_slice_grid(csv, grid_out, z=0)
    assert os.path.exists(grid_out) and os.path.getsize(grid_out) > 0

