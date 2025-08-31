C API
=====

Header: ``include/octoweave/c_api.h``

Opaque Handles
--------------

- ``ow_hierarchy_t``
- ``ow_forest_t``

Functions
---------

- ``ow_build_hierarchy_from_points(xyz,count,params,tau,p_unknown,base_depth)`` → ``ow_hierarchy_t``
- ``ow_hierarchy_write_csv(h,path)`` → ``int``
- ``ow_build_forest_uniform(h,n,level)`` → ``ow_forest_t``
- ``ow_hierarchy_free(h)`` / ``ow_forest_free(f)``
- Levels from Hierarchy:
  - ``ow_levels_by_leafcount_quantiles(...)``
  - ``ow_levels_bands_by_mean_prob(...)``
- Viz wrapper:
  - ``ow_viz_slice(csv_path,slice_z,depth,out_pgm,out_svg)``

Notes
-----

The C API is designed for stable FFI and used by the Python ctypes wrapper.

