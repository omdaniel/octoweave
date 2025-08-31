#ifdef OCTOWEAVE_WITH_P8EST
extern "C" {
#include <p8est_connectivity.h>
}

// A tiny TU to ensure we actually link p8est (and transitively p4est/sc) symbols.
extern "C" void ow_p8est_probe_symbol() {
  // Create and destroy a small 2x2x2 brick connectivity to force symbol refs
  p8est_connectivity_t *conn = p8est_connectivity_new_brick(2, 2, 2, 1, 0);
  p8est_connectivity_destroy(conn);
}
#endif

