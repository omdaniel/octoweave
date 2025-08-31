octoweave-py
============

A ctypes-based Python wrapper around the OctoWeave C++ library.

This package expects a shared library `liboctoweave_c.*` (or `octoweave_c.*` on Windows)
shipped inside `octoweave_py/`. Build it with CMake and the flag:

```
cmake -S .. -B ../build -DOCTOWEAVE_BUILD_PYTHON=ON
cmake --build ../build -j
```

When building from a source checkout, install in editable mode:

```
pip install -e .
```

Optional extras:

```
pip install ".[dev,test]"
```

This enables numpy input (`(k,3)` arrays) and Parquet IO via pandas.

