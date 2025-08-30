#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace octoweave {

inline double logit(double p) { return std::log(p/(1.0-p)); }
inline double inv_logit(double l){ return 1.0/(1.0 + std::exp(-l)); }

// Numerically-stable 8-way union: P = 1 - Π_i (1 - p_i)
double union_prob8_stable(const std::array<double,8>& p8, double p_unknown=0.5);

} // namespace octoweave
