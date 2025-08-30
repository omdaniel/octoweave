#pragma once
#include <string>

namespace octoweave {

// CLI stub that will render 2D slices and level histograms from CSV
// Expected CSV: x,y,z,depth,prob
int viz_main(int argc, char** argv);

} // namespace octoweave
