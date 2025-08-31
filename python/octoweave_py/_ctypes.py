import os
import sys
import ctypes as C
from dataclasses import dataclass
from typing import Iterable, Tuple
try:
    import numpy as _np  # optional
except Exception:  # pragma: no cover
    _np = None


def _lib_name() -> str:
    if sys.platform == "win32":
        return "octoweave_c.dll"
    if sys.platform == "darwin":
        return "liboctoweave_c.dylib"
    return "liboctoweave_c.so"


def _load_lib() -> C.CDLL:
    here = os.path.dirname(__file__)
    cand = os.path.join(here, _lib_name())
    if not os.path.exists(cand):
        raise FileNotFoundError(f"Cannot find shared library: {cand}. Build with -DOCTOWEAVE_BUILD_PYTHON=ON and rebuild.")
    return C.CDLL(cand)


_L = _load_lib()


class _ow_h(C.Structure):
    pass


class _ow_f(C.Structure):
    pass


ow_hierarchy_t = C.POINTER(_ow_h)
ow_forest_t = C.POINTER(_ow_f)


class _ChunkParams(C.Structure):
    _fields_ = [
        ("res", C.c_double),
        ("prob_hit", C.c_double),
        ("prob_miss", C.c_double),
        ("clamp_min", C.c_double),
        ("clamp_max", C.c_double),
        ("origin_xyz", C.c_double * 3),
        ("max_range", C.c_double),
        ("lazy_eval", C.c_int),
        ("discretize", C.c_int),
        ("emit_res", C.c_double),
        ("max_depth_cap", C.c_int),
    ]


_L.ow_build_hierarchy_from_points.argtypes = [C.POINTER(C.c_double), C.c_size_t, C.POINTER(_ChunkParams), C.c_double, C.c_double, C.c_int]
_L.ow_build_hierarchy_from_points.restype = ow_hierarchy_t
_L.ow_hierarchy_write_csv.argtypes = [ow_hierarchy_t, C.c_char_p]
_L.ow_hierarchy_write_csv.restype = C.c_int
_L.ow_hierarchy_free.argtypes = [ow_hierarchy_t]
_L.ow_build_forest_uniform.argtypes = [ow_hierarchy_t, C.c_int, C.c_int]
_L.ow_build_forest_uniform.restype = ow_forest_t
_L.ow_forest_free.argtypes = [ow_forest_t]
_L.ow_levels_by_leafcount_quantiles.argtypes = [ow_hierarchy_t, C.c_int, C.c_double, C.c_double, C.c_int, C.c_int, C.c_int, C.POINTER(C.c_int), C.c_size_t]
_L.ow_levels_by_leafcount_quantiles.restype = C.c_int
_L.ow_levels_bands_by_mean_prob.argtypes = [ow_hierarchy_t, C.c_int, C.POINTER(C.c_double), C.c_size_t, C.POINTER(C.c_int), C.c_size_t, C.POINTER(C.c_int), C.c_size_t]
_L.ow_levels_bands_by_mean_prob.restype = C.c_int
_L.ow_viz_slice.argtypes = [C.c_char_p, C.c_int, C.c_int, C.c_char_p, C.c_char_p]
_L.ow_viz_slice.restype = C.c_int


@dataclass
class ChunkParams:
    res: float = 0.05
    prob_hit: float = 0.7
    prob_miss: float = 0.4
    clamp_min: float = 0.12
    clamp_max: float = 0.97
    origin: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    max_range: float = -1.0
    lazy_eval: bool = False
    discretize: bool = False
    emit_res: float = -1.0
    max_depth_cap: int = 12

    def to_c(self) -> _ChunkParams:
        c = _ChunkParams()
        c.res = self.res
        c.prob_hit = self.prob_hit
        c.prob_miss = self.prob_miss
        c.clamp_min = self.clamp_min
        c.clamp_max = self.clamp_max
        c.origin_xyz[:] = (self.origin[0], self.origin[1], self.origin[2])
        c.max_range = self.max_range
        c.lazy_eval = int(self.lazy_eval)
        c.discretize = int(self.discretize)
        c.emit_res = self.emit_res
        c.max_depth_cap = self.max_depth_cap
        return c


class OctoWeave:
    def __init__(self):
        self._h = None
        self._f = None

    def build_hierarchy_from_points(self, xyz: Iterable[Tuple[float, float, float]], params: ChunkParams = ChunkParams(), tau: float = 0.5, p_unknown: float = 0.5, base_depth: int = 1):
        # Accept list/iterable of tuples or a numpy (k,3) array
        if _np is not None and isinstance(xyz, _np.ndarray):
            a = _np.asarray(xyz, dtype=_np.float64)
            if a.ndim != 2 or a.shape[1] != 3:
                raise ValueError("numpy array must be shape (k,3)")
            arr = a.ravel()
            ptr = arr.ctypes.data_as(C.POINTER(C.c_double))
            count = a.shape[0]
        else:
            buf = []
            for (x, y, z) in xyz:
                buf.extend([float(x), float(y), float(z)])
            arr = (C.c_double * len(buf))(*buf)
            ptr = arr
            count = len(buf) // 3
        cp = params.to_c()
        h = _L.ow_build_hierarchy_from_points(ptr, count, C.byref(cp), C.c_double(tau), C.c_double(p_unknown), C.c_int(base_depth))
        if not h:
            raise RuntimeError("ow_build_hierarchy_from_points failed")
        self._h = h
        return self

    def write_csv(self, path: str) -> int:
        if not self._h:
            raise RuntimeError("Hierarchy not built")
        rc = _L.ow_hierarchy_write_csv(self._h, path.encode("utf-8"))
        return int(rc)

    def build_forest_uniform(self, n: int, level: int):
        if not self._h:
            raise RuntimeError("Hierarchy not built")
        f = _L.ow_build_forest_uniform(self._h, int(n), int(level))
        if not f:
            raise RuntimeError("ow_build_forest_uniform failed")
        self._f = f
        return self

    def close(self):
        if self._f:
            _L.ow_forest_free(self._f)
            self._f = None
        if self._h:
            _L.ow_hierarchy_free(self._h)
            self._h = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    # Compute levels on C++ side using quantiles; returns list of length n^3
    def compute_levels_by_leafcount_quantiles(self, n: int, q_lo: float, q_hi: float, Llow: int, Lmid: int, Lhigh: int):
        if not self._h:
            raise RuntimeError("Hierarchy not built")
        T = n*n*n
        out = (C.c_int * T)()
        rc = _L.ow_levels_by_leafcount_quantiles(self._h, int(n), float(q_lo), float(q_hi), int(Llow), int(Lmid), int(Lhigh), out, C.c_size_t(T))
        if rc != 0:
            raise RuntimeError(f"ow_levels_by_leafcount_quantiles failed rc={rc}")
        return [out[i] for i in range(T)]

    # Compute levels on C++ side using bands by mean probability
    def compute_levels_bands_by_mean_prob(self, n: int, thresholds, levels):
        if not self._h:
            raise RuntimeError("Hierarchy not built")
        T = n*n*n
        th = (C.c_double * len(thresholds))(*[float(x) for x in thresholds])
        lv = (C.c_int * len(levels))(*[int(x) for x in levels])
        out = (C.c_int * T)()
        rc = _L.ow_levels_bands_by_mean_prob(self._h, int(n), th, C.c_size_t(len(thresholds)), lv, C.c_size_t(len(levels)), out, C.c_size_t(T))
        if rc != 0:
            raise RuntimeError(f"ow_levels_bands_by_mean_prob failed rc={rc}")
        return [out[i] for i in range(T)]

    # Full pipeline helper: points -> hierarchy -> forest (uniform from policy) -> CSV -> viz
    def run_pipeline(self,
                     xyz: Iterable[Tuple[float,float,float]],
                     params: ChunkParams,
                     n: int,
                     csv_path: str,
                     slice_z: int = 0,
                     depth: int = -1,
                     out_pgm: str = None,
                     out_svg: str = None,
                     policy: str = "uniform",
                     policy_args = None):
        self.build_hierarchy_from_points(xyz, params)
        # Compute a derived uniform level from policy helpers for deterministic tests
        lvl = None
        if policy == "uniform":
            lvl = int(policy_args or 0)
        elif policy == "quantiles":
            q_lo, q_hi, Llow, Lmid, Lhigh = policy_args or (0.2, 0.8, 4, 7, 10)
            levels = self.compute_levels_by_leafcount_quantiles(n, q_lo, q_hi, Llow, Lmid, Lhigh)
            levels_sorted = sorted(levels)
            lvl = levels_sorted[len(levels_sorted)//2]
        elif policy == "bands_mean":
            thresholds, levels_in = policy_args or ([0.3, 0.6, 0.85], [4, 7, 9, 11])
            levels = self.compute_levels_bands_by_mean_prob(n, thresholds, levels_in)
            levels_sorted = sorted(levels)
            lvl = levels_sorted[len(levels_sorted)//2]
        else:
            raise ValueError("Unknown policy")
        self.build_forest_uniform(n, lvl)
        # CSV
        os.makedirs(os.path.dirname(csv_path) or ".", exist_ok=True)
        rc = self.write_csv(csv_path)
        if rc != 0:
            raise RuntimeError(f"write_csv failed rc={rc}")
        # Viz
        if out_pgm:
            if depth < 0:
                depth = -1  # omit depth to let viz compute max
            rc2 = _L.ow_viz_slice(csv_path.encode("utf-8"), int(slice_z), int(depth), out_pgm.encode("utf-8"), (out_svg or "").encode("utf-8"))
            if rc2 != 0:
                raise RuntimeError(f"viz_slice failed rc={rc2}")
        return {
            "csv": csv_path,
            "pgm": out_pgm,
            "svg": out_svg,
            "uniform_level": lvl,
        }

    # Load a parquet file and build a hierarchy; requires pandas with pyarrow/fastparquet
    def build_hierarchy_from_parquet(self, parquet_path: str, params: ChunkParams = ChunkParams(), columns=("x","y","z"), **kwargs):
        try:
            import pandas as _pd  # type: ignore
        except Exception as e:
            raise ImportError("pandas is required to load parquet") from e
        df = _pd.read_parquet(parquet_path, **kwargs)
        a = df[list(columns)].to_numpy(dtype=float, copy=False)
        return self.build_hierarchy_from_points(a, params)

    # Simple CSV -> PGM/SVG renderer
    def render_csv(self, csv_path: str, slice_z: int, depth: int = -1, out_pgm: str = "slice.pgm", out_svg: str = "") -> int:
        if depth < 0:
            depth = -1
        return int(_L.ow_viz_slice(csv_path.encode("utf-8"), int(slice_z), int(depth), out_pgm.encode("utf-8"), (out_svg or "").encode("utf-8")))
