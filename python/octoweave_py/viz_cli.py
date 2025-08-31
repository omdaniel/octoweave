import argparse
import os
from .viz import render_slice_png, render_max_projection_png, render_montage


def _parse_range(s):
    if ':' in s:
        a,b = s.split(':', 1)
        return int(a), int(b)
    raise ValueError("range must be a:b")


def _parse_list(s):
    return [int(x) for x in s.split(',') if x]


def main(argv=None):
    ap = argparse.ArgumentParser(description="OctoWeave Python visualization (matplotlib)")
    ap.add_argument('--csv', required=True, help='CSV path (x,y,z,depth,prob)')
    ap.add_argument('--outdir', default='examples_out/pyviz', help='Output directory')
    ap.add_argument('--depth', type=int, default=None, help='Depth to visualize (default=max depth in CSV)')
    ap.add_argument('--colormap', default='viridis')
    ap.add_argument('--grid', action='store_true', help='Overlay grid')
    ap.add_argument('--mode', choices=['slice','maxproj','montage','overlay'], default='slice')
    ap.add_argument('--slice-z', type=int, default=0, help='Z index for single slice')
    ap.add_argument('--z-range', type=str, default=None, help='zmin:zmax for projection')
    ap.add_argument('--proj-op', choices=['max','mean'], default='max')
    ap.add_argument('--zs', type=str, default=None, help='Comma-separated z indices for montage')
    ap.add_argument('--ncols', type=int, default=4)
    ap.add_argument('--csv2', type=str, default=None, help='Second CSV for overlay mode')
    ap.add_argument('--alpha-a', type=float, default=0.6)
    ap.add_argument('--alpha-b', type=float, default=0.6)
    ap.add_argument('--colormap2', type=str, default='Reds')
    ap.add_argument('--discrete-levels', type=str, default=None, help='Comma-separated bin edges, e.g. 0,0.25,0.5,0.75,1')
    ap.add_argument('--preset', type=str, default=None, help='Occupancy colormap preset: occ3|occ5|occ_heat')
    ap.add_argument('--legend', action='store_true', help='Show legend labels for discrete bins')
    ap.add_argument('--export-grid', type=str, default=None, help='Export slice grid (x,y,prob) CSV')

    args = ap.parse_args(argv)
    os.makedirs(args.outdir, exist_ok=True)

    # Parse discrete levels if provided
    levels = None
    if args.discrete-levels or args.discrete_levels:
        s = (args.discrete_levels or args.discrete-levels)  # type: ignore[attr-defined]
        levels = [float(x) for x in s.split(',') if x]

    if args.mode == 'slice':
        out = os.path.join(args.outdir, f'slice_z{args.slice_z}.png')
        render_slice_png(args.csv, out, z=args.slice_z, depth=args.depth, colormap=args.colormap, grid=args.grid,
                         discrete_levels=levels, legend=args.legend, preset=args.preset)
        if args.export_grid:
            from .viz import export_slice_grid
            export_slice_grid(args.csv, args.export_grid, args.slice_z, args.depth)
        print('wrote', out)
    elif args.mode == 'maxproj':
        if not args.z_range:
            raise SystemExit('--z-range a:b is required for maxproj')
        zr = _parse_range(args.z_range)
        out = os.path.join(args.outdir, f'maxproj_{zr[0]}_{zr[1]}.png')
        render_max_projection_png(args.csv, out, zr, depth=args.depth, colormap=args.colormap, grid=args.grid, op=args.proj_op,
                                  discrete_levels=levels, legend=args.legend, preset=args.preset)
        print('wrote', out)
    elif args.mode == 'montage':
        if not args.zs:
            raise SystemExit('--zs z1,z2,... is required for montage')
        zs = _parse_list(args.zs)
        out = os.path.join(args.outdir, f'montage.png')
        render_montage(args.csv, out, zs, depth=args.depth, colormap=args.colormap, ncols=args.ncols, grid=args.grid)
        print('wrote', out)
    else:
        if not args.csv2:
            raise SystemExit('--csv2 is required for overlay mode')
        out = os.path.join(args.outdir, f'overlay_z{args.slice_z}.png')
        from .viz import render_overlay_slices
        render_overlay_slices(args.csv, args.csv2, out, z=args.slice_z, depth=args.depth,
                              colormap_a=args.colormap, colormap_b=args.colormap2,
                              alpha_a=args.alpha_a, alpha_b=args.alpha_b, grid=args.grid)
        print('wrote', out)

if __name__ == '__main__':
    raise SystemExit(main())
