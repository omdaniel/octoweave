Python API
==========

Package: ``octoweave_py``

Overview
--------

Lightweight ctypes wrapper exposing core functionality without extra C++ dependencies.

Classes
-------

ChunkParams
~~~~~~~~~~~

Fields:

- ``res`` (float): OcTree resolution
- ``prob_hit`` / ``prob_miss`` (float)
- ``clamp_min`` / ``clamp_max`` (float)
- ``origin`` (x,y,z)
- ``max_range`` (<=0 unlimited)
- ``lazy_eval`` / ``discretize`` (bool)
- ``emit_res`` (float)
- ``max_depth_cap`` (int)

OctoWeave
~~~~~~~~~

- ``build_hierarchy_from_points(xyz, params, tau=0.5, p_unknown=0.5, base_depth=1)``
  Accepts list of (x,y,z) or numpy array (k,3)
- ``build_hierarchy_from_parquet(path, params, columns=('x','y','z'))``
- ``write_csv(path)``
- ``build_forest_uniform(n, level)``
- ``compute_levels_by_leafcount_quantiles(n, q_lo, q_hi, Llow, Lmid, Lhigh)``
- ``compute_levels_bands_by_mean_prob(n, thresholds, levels)``
- ``run_pipeline(xyz, params, n, csv_path, slice_z=0, depth=-1, out_pgm=None, out_svg=None, policy='uniform'|'quantiles'|'bands_mean', policy_args=None)``
- ``render_csv(csv_path, slice_z, depth=-1, out_pgm='slice.pgm', out_svg='')``

CLI
---

Module: ``octoweave_py.cli`` (also installed as ``octoweave-pipeline``)

Usage:

.. code-block:: bash

   python -m octoweave_py.cli --points-csv examples_out/leaves.csv --columns x,y,z \
     --n 2 --outdir examples_out/python --policy quantiles --q-lo 0.2 --q-hi 0.8 --Llow 4 --Lmid 7 --Lhigh 10

