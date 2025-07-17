#ifndef __INPUT_H__
#define __INPUT_H__

#include "mth/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

extern v4f gMouseDelta;

/**
 * Handle keyboard and mouse inputs for freecam mode.
 */
void gui_inputFreecam();
void gui_inputFPV();

#ifdef __cplusplus
}
#endif

#endif
