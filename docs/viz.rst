Visualization
=============

CSV Format
----------

Plain text rows of: ``x,y,z,depth,prob``

CLI
---

``octoweave_viz --csv leaves.csv --slice_z 0 --depth 3 --out slice.pgm --hist hist.svg``

Python
------

``OctoWeave.render_csv(csv, slice_z=0, depth=-1, out_pgm='slice.pgm', out_svg='')``

Output
------

- PGM (P2): grayscale of probabilities mapped to [0..255]
- SVG: depth histogram with counts

