/**
 * @file timebase.h
 * @author JR
 * @date 10.05.2026
 * @version 1.0
 * @brief Hardware-independent timer callback helpers
 */

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************/
// INCLUDES
/****************************************************/

#include <stdint.h>

/****************************************************/
// GLOBAL TYPE DEFINITIONS
/****************************************************/

typedef uint32_t (*TimebaseGetMillis)(void);

typedef uint32_t (*TimebaseGetMicros)(void);

#if defined(__cplusplus)
} // extern "C"
#endif
