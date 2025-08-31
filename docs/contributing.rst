Contributing
============

Coding Standards
----------------

- C++17, headers under ``include/octoweave/...``
- Namespaces: ``octoweave::``
- Zero allocations in hot paths; prefer ``std::vector`` with reserve
- Document pre/post conditions in headers

Tests
-----

Use Catch2 (stub offline) and add tests under ``tests/unit``. Gate each phase
with tests and keep scope focused.

Docs
----

Add new pages under ``docs/`` and include them in ``index.rst`` toctree. Keep
instructions reproducible and platform hints explicit.

PRs
---

Open a PR describing changes, rationale, and testing performed. Avoid introducing
new external build dependencies.

