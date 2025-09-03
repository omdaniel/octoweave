#ifdef OCTOWEAVE_WITH_P4EST
extern "C" {
#include <p4est_connectivity.h>
}

// A tiny TU to ensure we actually link p4est (and transitively sc) symbols.
extern "C" void ow_p4est_probe_symbol() {
  // Create and destroy a small 2x2 brick connectivity to force symbol refs
  p4est_connectivity_t *conn = p4est_connectivity_new_brick(2, 2, 1, 0);
  p4est_connectivity_destroy(conn);
}
#endif

