#pragma once
#include <vector>
#include <functional>
#include "hierarchy.hpp"

namespace octoweave {

// Run a per-chunk builder in parallel and return results in chunk-index order.
std::vector<WorkerOut> parallel_build_workers(int num_chunks,
                                              const std::function<WorkerOut(int)>& build,
                                              int max_threads = 0);

} // namespace octoweave

