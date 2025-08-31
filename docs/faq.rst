FAQ
===

Does OctoWeave require OctoMap/p8est?
-------------------------------------

No. Stub implementations let you build and run tests without external deps. Enable
real integrations via CMake options when available.

Can I pass numpy arrays from Python?
------------------------------------

Yes. Use a (k,3) array for points; the wrapper converts it to the required C layout.

How are probabilities combined?
--------------------------------

Per–node combiners use numerically stable union: ``1 - Π (1 - p)``.

