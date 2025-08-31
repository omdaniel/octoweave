#ifdef OCTOWEAVE_WITH_OCTOMAP
#include <octomap/OcTree.h>

// A tiny TU to ensure we actually reference OctoMap symbols when the flag is on.
// This gets compiled into the octoweave static lib.
extern "C" void ow_octomap_probe_symbol() {
  octomap::OcTree tree(0.1);
  tree.setProbHit(0.7);
  tree.setProbMiss(0.4);
  tree.setClampingThresMin(0.12);
  tree.setClampingThresMax(0.97);
}
#endif

