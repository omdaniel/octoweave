Build & Test
============

Common Options
--------------

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DOCTOWEAVE_BUILD_EXAMPLES=ON \
         -DOCTOWEAVE_BUILD_PYTHON=ON
   cmake --build build -j
   ctest --test-dir build -j

OctoMap Integration
-------------------

.. code-block:: bash

   cmake -S . -B build-om -DOCTOWEAVE_WITH_OCTOMAP=ON \
     -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/octomap \
     -DCMAKE_BUILD_RPATH=/opt/homebrew/opt/octomap/lib
   cmake --build build-om -j && ctest --test-dir build-om -j

p4est Integration
-----------------

.. code-block:: bash

   export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:$PKG_CONFIG_PATH
   cmake -S . -B build-int -DOCTOWEAVE_WITH_OCTOMAP=ON -DOCTOWEAVE_WITH_P4EST=ON \
     -DCMAKE_PREFIX_PATH="/opt/local;/opt/homebrew/opt/octomap" \
     -DCMAKE_BUILD_RPATH="/opt/local/lib;/opt/homebrew/opt/octomap/lib"
   cmake --build build-int -j && ctest --test-dir build-int -j

Docs
----

If ``sphinx-build`` is available, CMake adds a ``docs`` target:

.. code-block:: bash

   cmake --build build --target docs
