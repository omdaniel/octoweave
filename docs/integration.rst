Integrations
============

OctoMap
-------

- Enable: ``-DOCTOWEAVE_WITH_OCTOMAP=ON``
- Hints (Homebrew):

  - ``-DCMAKE_PREFIX_PATH=/opt/homebrew/opt/octomap``
  - ``-DCMAKE_BUILD_RPATH=/opt/homebrew/opt/octomap/lib``

OctoChunker (Real)
~~~~~~~~~~~~~~~~~~

Uses ``octomap::OcTree::insertPointCloud`` with configurable hit/miss probabilities,
clamping thresholds, origin, max range, lazy evaluation, and discretization. Exports
probabilities at a target resolution (``emit_res``) with a safety depth cap
(``max_depth_cap``).

p4est
-----

- Enable: ``-DOCTOWEAVE_WITH_P4EST=ON``
- Hints (MacPorts): ``export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:$PKG_CONFIG_PATH``
  and ``-DCMAKE_PREFIX_PATH=/opt/local`` with RPATH ``/opt/local/lib``

P4estBuilder (Real)
~~~~~~~~~~~~~~~~~~~

Creates a brick connectivity, builds a forest (3D via the p4est project's p8est API),
refines per tree according to a policy target level, balances, and initializes
per–quadrant user data to per–cell means.
