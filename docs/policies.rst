Refinement Policies
===================

Policies (C++)
--------------

``octoweave::P8estBuilder::Policy`` helpers:

- ``uniform(level)``
- ``from_levels(levels[n^3])``
- ``by_leafcount_linear(H,n,Lmin,Lmax)``
- ``by_mean_prob_threshold(H,n,threshold,Llow,Lhigh)``
- ``by_leafcount_quantiles(H,n,q_lo,q_hi,Llow,Lmid,Lhigh)``
- ``bands_by_count(H,n,thresholds,levels)``
- ``bands_by_mean_prob(H,n,thresholds,levels)``

Per–quadrant means
------------------

Real p8est integration initializes one double per quadrant as the mean of
Hierarchy leaf probabilities at target level ``Ltarget(tree)`` after mapping
global keys to tree–local coordinates.

