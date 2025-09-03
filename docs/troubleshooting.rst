Troubleshooting
===============

OctoMap dylib not found (macOS)
-------------------------------

Add RPATH: ``-DCMAKE_BUILD_RPATH=/opt/homebrew/opt/octomap/lib`` or set
``DYLD_LIBRARY_PATH`` temporarily.

p4est not found
---------------

Ensure MacPorts pkg-config is visible: ``export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:$PKG_CONFIG_PATH``
and pass ``-DCMAKE_PREFIX_PATH=/opt/local``.

Catch2 fetch/stub conflicts
---------------------------

A local FindCatch2 stub is used offline. To force FetchContent of Catch2, remove the
stub or adjust ``CMAKE_MODULE_PATH``.

Python cannot find shared library
---------------------------------

Build with ``-DOCTOWEAVE_BUILD_PYTHON=ON`` so CMake copies the shared library into
``python/octoweave_py/``. Then ``sys.path.insert(0,'python')`` before import.
