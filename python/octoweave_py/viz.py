import os
from collections.abc import Sequence

try:
    import pandas as pd  # type: ignore
except Exception:
    pd = None

import numpy as np
import matplotlib
matplotlib.use('Agg')  # non-interactive backend for headless environments
import matplotlib.pyplot as plt


def _load_csv(csv_path: str):
    if pd is not None:
        df = pd.read_csv(csv_path, header=None, names=["x","y","z","depth","prob"], dtype={"x":int,"y":int,"z":int,"depth":int,"prob":float})
        return df
    # fallback simple loader
    data = np.loadtxt(csv_path, delimiter=",", dtype=float)
    return {"x": data[:,0].astype(int), "y": data[:,1].astype(int), "z": data[:,2].astype(int), "depth": data[:,3].astype(int), "prob": data[:,4]}


def _filter_depth_and_z(data, depth: int | None, z_values: Sequence[int] | None):
    if pd is not None and isinstance(data, pd.DataFrame):
        df = data
        if depth is None:
            depth = int(df["depth"].max())
        df = df[df["depth"] == depth]
        if z_values is not None:
            df = df[df["z"].isin(z_values)]
        return df, depth
    # dict/ndarray fallback
    d = data
    if depth is None:
        depth = int(np.max(d["depth"]))
    mask = (d["depth"] == depth)
    if z_values is not None:
        mask = mask & np.isin(d["z"], np.array(z_values, dtype=int))
    out = {k: v[mask] for k,v in d.items()}
    return out, depth


def _make_grid(data) -> tuple[np.ndarray, int, int]:
    if pd is not None and isinstance(data, pd.DataFrame):
        xmax = int(data["x"].max()) if len(data) else 0
        ymax = int(data["y"].max()) if len(data) else 0
        W, H = xmax + 1, ymax + 1
        img = np.zeros((H, W), dtype=float)
        for _, r in data.iterrows():
            img[int(r["y"]), int(r["x"])] = float(r["prob"])
        return img, W, H
    x = data["x"]; y = data["y"]; p = data["prob"]
    W = int(np.max(x)) + 1 if len(x) else 1
    H = int(np.max(y)) + 1 if len(y) else 1
    img = np.zeros((H, W), dtype=float)
    img[y, x] = p
    return img, W, H


def _norm_for_discrete(discrete_levels: Sequence[float] | None, cmap_name: str):
    if not discrete_levels:
        return None, plt.get_cmap(cmap_name)
    import matplotlib.colors as mcolors
    bounds = list(discrete_levels)
    cmap = plt.get_cmap(cmap_name, len(bounds)-1)
    norm = mcolors.BoundaryNorm(boundaries=bounds, ncolors=cmap.N, clip=True)
    return norm, cmap


def occupancy_colormap_preset(name: str) -> tuple[list[float], 'matplotlib.colors.Colormap', list[str]]:
    """Return (levels, colormap, labels) for an occupancy band preset.

    Presets:
    - 'occ3': free/unknown/occupied bands
      levels: [0.0, 0.5, 0.7, 1.0]
      colors: free (blue), unknown (gray), occupied (red)
    - 'occ5': fine bands with clamp hints
      levels: [0.0, 0.12, 0.5, 0.7, 0.97, 1.0]
      colors: deep blue, light blue, gray, orange, red
    - 'occ_heat': quintiles using a listed gradient
      levels: [0.0, 0.25, 0.5, 0.75, 1.0]
      colors: using 'inferno' discrete
    """
    import matplotlib.colors as mcolors
    n = name.lower()
    if n == 'occ3':
        levels = [0.0, 0.5, 0.7, 1.0]
        colors = ['#2c7fb8', '#bdbdbd', '#d7301f']
        labels = ['free', 'unknown', 'occupied']
        cmap = mcolors.ListedColormap(colors)
        return levels, cmap, labels
    if n == 'occ5':
        levels = [0.0, 0.12, 0.5, 0.7, 0.97, 1.0]
        colors = ['#084081', '#4eb3d3', '#bdbdbd', '#fdae61', '#d7301f']
        labels = ['< clamp_min', '[clamp_min,0.5)', '[0.5,hit)', '[hit,clamp_max)', '>= clamp_max']
        cmap = mcolors.ListedColormap(colors)
        return levels, cmap, labels
    if n == 'occ_heat':
        levels = [0.0, 0.25, 0.5, 0.75, 1.0]
        base = plt.get_cmap('inferno', len(levels)-1)
        colors = [base(i) for i in range(base.N)]
        labels = [f'[{a:.2f},{b:.2f})' for a,b in zip(levels[:-1], levels[1:])]
        cmap = mcolors.ListedColormap(colors)
        return levels, cmap, labels
    raise ValueError(f"Unknown preset '{name}'")


def render_slice_png(csv_path: str, out_png: str, z: int, depth: int | None = None,
                     colormap: str = "viridis", grid: bool = True,
                     vmin: float | None = 0.0, vmax: float | None = 1.0,
                     figsize=(6,6), dpi=150, colorbar: bool = True,
                     discrete_levels: Sequence[float] | None = None,
                     legend: bool = False,
                     preset: str | None = None):
    data = _load_csv(csv_path)
    df, depth = _filter_depth_and_z(data, depth, [z])
    img, W, H = _make_grid(df)

    preset_labels: list[str] | None = None
    if preset:
        levels, cmap, preset_labels = occupancy_colormap_preset(preset)
        discrete_levels = levels
        norm = None
        import matplotlib.colors as mcolors
        norm = mcolors.BoundaryNorm(discrete_levels, cmap.N)
    else:
        norm, cmap = _norm_for_discrete(discrete_levels, colormap)
    fig, ax = plt.subplots(figsize=figsize, dpi=dpi)
    im = ax.imshow(img, origin='lower', cmap=cmap, vmin=None if norm else vmin, vmax=None if norm else vmax, norm=norm)
    ax.set_title(f"slice z={z}, depth={depth}")
    ax.set_xlabel("x"); ax.set_ylabel("y")
    ax.set_aspect('equal')
    if grid:
        ax.set_xticks(np.arange(-.5, W, 1), minor=True)
        ax.set_yticks(np.arange(-.5, H, 1), minor=True)
        ax.grid(which='minor', color='w', linestyle='-', linewidth=0.5, alpha=0.5)
    if colorbar:
        cb = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04, label='prob')
        if discrete_levels and legend:
            centers = [(a+b)/2 for a,b in zip(discrete_levels[:-1], discrete_levels[1:])]
            cb.set_ticks(centers)
            if preset_labels and len(preset_labels) == len(discrete_levels)-1:
                cb.set_ticklabels(preset_labels)
            else:
                cb.set_ticklabels([f"[{a:.2f},{b:.2f})" for a,b in zip(discrete_levels[:-1], discrete_levels[1:])])
    os.makedirs(os.path.dirname(out_png) or '.', exist_ok=True)
    fig.savefig(out_png, bbox_inches='tight')
    plt.close(fig)


def render_max_projection_png(csv_path: str, out_png: str, z_range: tuple[int,int], depth: int | None = None,
                              colormap: str = "magma", grid: bool = False,
                              vmin: float | None = 0.0, vmax: float | None = 1.0,
                              figsize=(6,6), dpi=150, colorbar: bool = True, op: str = 'max',
                              discrete_levels: Sequence[float] | None = None,
                              legend: bool = False,
                              preset: str | None = None):
    zmin, zmax = z_range
    zs = list(range(zmin, zmax+1))
    data = _load_csv(csv_path)
    df, depth = _filter_depth_and_z(data, depth, zs)

    if pd is not None and isinstance(df, pd.DataFrame):
        xmax = int(df["x"].max()) if len(df) else 0
        ymax = int(df["y"].max()) if len(df) else 0
        W, H = xmax + 1, ymax + 1
        stack = np.zeros((len(zs), H, W), dtype=float)
        for i, z in enumerate(zs):
            s = df[df["z"] == z]
            for _, r in s.iterrows():
                stack[i, int(r["y"]), int(r["x"])] = float(r["prob"])
    else:
        x = df["x"]; y = df["y"]; z = df["z"]; p = df["prob"]
        W = int(np.max(x)) + 1 if len(x) else 1
        H = int(np.max(y)) + 1 if len(y) else 1
        stack = np.zeros((len(zs), H, W), dtype=float)
        for i, zi in enumerate(zs):
            mask = (z == zi)
            stack[i][y[mask], x[mask]] = p[mask]

    if op == 'max':
        img = np.max(stack, axis=0)
    elif op == 'mean':
        img = np.mean(stack, axis=0)
    else:
        raise ValueError("op must be 'max' or 'mean'")

    preset_labels: Optional[List[str]] = None
    if preset:
        levels, cmap, preset_labels = occupancy_colormap_preset(preset)
        discrete_levels = levels
        import matplotlib.colors as mcolors
        norm = mcolors.BoundaryNorm(discrete_levels, cmap.N)
    else:
        norm, cmap = _norm_for_discrete(discrete_levels, colormap)
    fig, ax = plt.subplots(figsize=figsize, dpi=dpi)
    im = ax.imshow(img, origin='lower', cmap=cmap, vmin=None if norm else vmin, vmax=None if norm else vmax, norm=norm)
    ax.set_title(f"z∈[{zmin},{zmax}], depth={depth}, {op} projection")
    ax.set_xlabel("x"); ax.set_ylabel("y")
    ax.set_aspect('equal')
    if grid:
        ax.set_xticks(np.arange(-.5, img.shape[1], 1), minor=True)
        ax.set_yticks(np.arange(-.5, img.shape[0], 1), minor=True)
        ax.grid(which='minor', color='w', linestyle='-', linewidth=0.5, alpha=0.5)
    if colorbar:
        cb = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04, label='prob')
        if discrete_levels and legend:
            centers = [(a+b)/2 for a,b in zip(discrete_levels[:-1], discrete_levels[1:])]
            cb.set_ticks(centers)
            if preset_labels and len(preset_labels) == len(discrete_levels)-1:
                cb.set_ticklabels(preset_labels)
            else:
                cb.set_ticklabels([f"[{a:.2f},{b:.2f})" for a,b in zip(discrete_levels[:-1], discrete_levels[1:])])
    os.makedirs(os.path.dirname(out_png) or '.', exist_ok=True)
    fig.savefig(out_png, bbox_inches='tight')
    plt.close(fig)


def render_montage(csv_path: str, out_png: str, zs: Sequence[int], depth: int | None = None,
                   colormap: str = 'viridis', ncols: int = 4, grid: bool = True,
                   figsize=(10,6), dpi=150):
    data = _load_csv(csv_path)
    df, depth = _filter_depth_and_z(data, depth, zs)

    # Build each slice image
    imgs = []
    for z in zs:
        if pd is not None and isinstance(df, pd.DataFrame):
            s = df[df["z"] == z]
        else:
            mask = (df["z"] == z)
            s = {k: v[mask] for k,v in df.items()}
        img, W, H = _make_grid(s)
        imgs.append((img, W, H, z))

    n = len(imgs)
    ncols = max(1, ncols)
    nrows = (n + ncols - 1) // ncols
    fig, axes = plt.subplots(nrows, ncols, figsize=figsize, dpi=dpi)
    axes = np.atleast_1d(axes).ravel()
    for ax, tup in zip(axes, imgs):
        img, W, H, z = tup
        im = ax.imshow(img, origin='lower', cmap=colormap, vmin=0.0, vmax=1.0)
        ax.set_title(f"z={z}")
        ax.set_aspect('equal')
        if grid:
            ax.set_xticks(np.arange(-.5, W, 1), minor=True)
            ax.set_yticks(np.arange(-.5, H, 1), minor=True)
            ax.grid(which='minor', color='w', linestyle='-', linewidth=0.5, alpha=0.5)
        ax.set_xticks([]); ax.set_yticks([])
    # Hide any unused axes
    for i in range(len(imgs), len(axes)):
        axes[i].axis('off')
    os.makedirs(os.path.dirname(out_png) or '.', exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_png, bbox_inches='tight')
    plt.close(fig)


def render_overlay_slices(csv_a: str, csv_b: str, out_png: str, z: int, depth: int | None = None,
                          colormap_a: str = 'Greens', colormap_b: str = 'Reds',
                          alpha_a: float = 0.6, alpha_b: float = 0.6, grid: bool = True,
                          figsize=(6,6), dpi=150):
    da = _load_csv(csv_a)
    db = _load_csv(csv_b)
    da, depth_a = _filter_depth_and_z(da, depth, [z])
    db, depth_b = _filter_depth_and_z(db, depth, [z])
    img_a, W, H = _make_grid(da)
    img_b, _, _ = _make_grid(db)

    fig, ax = plt.subplots(figsize=figsize, dpi=dpi)
    ax.imshow(img_a, origin='lower', cmap=colormap_a, vmin=0.0, vmax=1.0, alpha=alpha_a)
    ax.imshow(img_b, origin='lower', cmap=colormap_b, vmin=0.0, vmax=1.0, alpha=alpha_b)
    ax.set_aspect('equal'); ax.set_xlabel('x'); ax.set_ylabel('y')
    ax.set_title(f"overlay z={z}, depth={depth_a}")
    if grid:
        ax.set_xticks(np.arange(-.5, W, 1), minor=True)
        ax.set_yticks(np.arange(-.5, H, 1), minor=True)
        ax.grid(which='minor', color='w', linestyle='-', linewidth=0.5, alpha=0.5)
    os.makedirs(os.path.dirname(out_png) or '.', exist_ok=True)
    fig.savefig(out_png, bbox_inches='tight')
    plt.close(fig)


def export_slice_grid(csv_path: str, out_csv: str, z: int, depth: int | None = None):
    data = _load_csv(csv_path)
    df, depth = _filter_depth_and_z(data, depth, [z])
    img, W, H = _make_grid(df)
    os.makedirs(os.path.dirname(out_csv) or '.', exist_ok=True)
    if pd is not None:
        rows = []
        for yy in range(H):
            for xx in range(W):
                rows.append((xx, yy, float(img[yy, xx])))
        pdf = pd.DataFrame(rows, columns=['x','y','prob'])
        pdf.to_csv(out_csv, index=False)
    else:
        with open(out_csv, 'w') as f:
            f.write('x,y,prob\n')
            for yy in range(H):
                for xx in range(W):
                    f.write(f"{xx},{yy},{img[yy,xx]:.9f}\n")
