Installation
============

Requirements
------------

- CMake ≥ 3.16
- C++17 compiler (Clang ≥ 12, GCC ≥ 9)
- pkg-config (for p4est via MacPorts)
- Optional: sphinx-build for documentation

Optional Libraries
------------------

- OctoMap (Homebrew): ``brew install octomap``
- p4est + libsc (MacPorts): ``sudo port install p4est``

Environment Hints (macOS)
-------------------------

- OctoMap: ``-DCMAKE_PREFIX_PATH=/opt/homebrew/opt/octomap`` and
  ``-DCMAKE_BUILD_RPATH=/opt/homebrew/opt/octomap/lib``
- p4est: ``export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:$PKG_CONFIG_PATH``
  and ``-DCMAKE_PREFIX_PATH=/opt/local`` with
  ``-DCMAKE_BUILD_RPATH=/opt/local/lib``

Python
------

- Optional numpy (for array input)
- Optional pandas + pyarrow or fastparquet (for Parquet)

Sphinx Docs
-----------

Install Sphinx (e.g. ``pip install sphinx``) and build:

.. code-block:: bash

   sphinx-build -b html docs docs/_build/html
