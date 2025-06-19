#ifndef __OFFSETS_H__
#define __OFFSETS_H__

#include "aliases.h"

#define VERSION_SKY 2

/**
 * Firstly we find the SkyCamera::update function through signature from IDA,
 * Then we can find the other functions.
 * 
 * Sig: SkyCamera::update():
 * 55 41 57 41 56 41 54 56 57 53 48 81 EC ?  ?  ? 
 * ?  48 8D AC 24 ?  ?  ?  ?  C5 78 29 85 ?  ?  ?
 * ?  C5 F8 29 BD ?  ?  ?  ?  C5 F8 29 B5 ?  ?  ?
 * ?  48 83 E4 E0 48 89 D7 48 89 CE 48 8B 82 ?  ?
 * ?  ?  C5 FA 10 70 ?  C5 FA 10 81 ?  ?  ?  ?  C4
 * E2 79 18 3D ?  ?  ?  ?  C5 C8 57 CF C5 F8 2E C8
 * 76 06 C5 CA 58 C8 EB 0E C5 F0 57 C9 C5 F8 2E C6
 * 76 04 C5 FA 5C CE
 */

#if VERSION_SKY == 1
// 0.14.2.325380, 2025.6.11
static const u64 offset_SkyCamera_update = 0x019DBA80ull
  , offset_SkyCamera_updateUI = 0x00779A50ull
  , offset_SkyCamera__updateParams = 0x019DD660ull
  , offset_SkyCamera_setState = 0x00000000ull
  , offset_Player_getPos = 0x01C02ED0ull;
#elif VERSION_SKY == 2
// 0.14.3.326140, 2025.6.19
static const u64 offset_SkyCamera_update = 0x01A1DCF0ull
  , offset_SkyCamera_updateUI = 0x007A8D60ull
  , offset_SkyCamera__updateParams = 0x01A1F8D0ull
  , offset_SkyCamera_setState = 0x00000000ull
  , offset_Player_getPos = 0x01C45070ull;
#endif

#endif
