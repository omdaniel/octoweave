CLI
===

octoweave_viz (C++)
-------------------

Render a 2D slice and depth histogram from a CSV of ``x,y,z,depth,prob``.

.. code-block:: bash

   octoweave_viz --csv leaves.csv --slice_z 0 --depth 3 --out slice.pgm --hist hist.svg

Python CLI
----------

``python -m octoweave_py.cli`` or entry point ``octoweave-pipeline``.

Inputs: ``--points-csv`` or ``--parquet`` with ``--columns`` (x,y,z),
policy selection (uniform/quantiles/bands_mean), and output controls.

