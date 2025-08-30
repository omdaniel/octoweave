#include "octoweave/union.hpp"

namespace octoweave {

double union_prob8_stable(const std::array<double,8>& p8, double p_unknown) {
  double sum_log_q = 0.0;
  for (double p : p8) {
    double pc = std::clamp(p, 0.0, 1.0);
    // treat NaN / sentinel by substituting p_unknown
    if (!(pc >= 0.0 && pc <= 1.0)) pc = p_unknown;
    sum_log_q += std::log1p(-pc);
  }
  double q = std::exp(sum_log_q);
  double P = 1.0 - q;
  if (P < 0.0) P = 0.0;
  if (P > 1.0) P = 1.0;
  return P;
}

} // namespace octoweave
