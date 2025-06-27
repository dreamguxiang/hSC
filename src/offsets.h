#ifndef __OFFSETS_H__
#define __OFFSETS_H__

#include "aliases.h"

#define VERSION_SKY 2

/**
 * Firstly we find the SkyCameraProp::update function through signature from IDA,
 * Then we can find the other functions.
 * 
 * Sig: SkyCameraProp::update():
 * 55 41 57 41 56 41 54 56 57 53 48 81 EC ?  ?  ? 
 * ?  48 8D AC 24 ?  ?  ?  ?  C5 78 29 85 ?  ?  ?
 * ?  C5 F8 29 BD ?  ?  ?  ?  C5 F8 29 B5 ?  ?  ?
 * ?  48 83 E4 E0 48 89 D7 48 89 CE 48 8B 82 ?  ?
 * ?  ?  C5 FA 10 70 ?  C5 FA 10 81 ?  ?  ?  ?  C4
 * E2 79 18 3D ?  ?  ?  ?  C5 C8 57 CF C5 F8 2E C8
 * 76 06 C5 CA 58 C8 EB 0E C5 F0 57 C9 C5 F8 2E C6
 * 76 04 C5 FA 5C CE
 * 
 * Sig: Level::interactionCheck():
 * 55 41 56 56 57 53 48 81 EC ?  ?  ?  ?  48 8D AC
 * 24 ?  ?  ?  ?  C5 F8 28 02 C5 F8 29 45 ?  C4 C1
 * 78 28 00 C5 F8 59 C8 C5 FA 16 D1 C5 F0 58 D2 C4
 * E3 79 05 C9 ?  C5 F0 58 CA C5 F2 52 C9 C4 E2 79
 * 18 C9 C5 F8 59 C1 C4 E3 79 05 C8 ?  C5 F8 29 45
 * ?  C4 E2 79 18 15 ?  ?  ?  ?  C5 F8 54 D2 C4 E2
 * 79 18 25 ?  ?  ?  ?  C5 E8 5F D4 C5 D8 57 E4 C5
 * DA
 * 
 * Sig: Player::doMovement():
 * 55 41 57 41 56 41 55 41 54 56 57 53 B8 ?  ?  ?
 * ?  E8 ?  ?  ?  ?  48 29 C4 48 8D AC 24 ?  ?  ?
 * ?  C5 78 29 BD ?  ?  ?  ?  C5 78 29 B5 ?  ?  ?
 * ?  C5 78 29 AD ?  ?  ?  ?  C5 78 29 A5 ?  ?  ?
 * ?  C5 78 29 9D ?  ?  ?  ?  C5 78 29 95 ?  ?  ?
 * ?  C5 78 29 8D ?  ?  ?  ?  C5 78 29 85 ?  ?  ?
 * ?  C5 F8 29 BD ?  ?  ?  ?  C5 F8 29 B5 ?  ?  ?
 * ?  49 89 D4 49 89 CF 80 79 10 00 75 0B 4C 89 F9
 * 4C 89 E2
 * 
 * Find the constructor of WhiskerCamera, and find WhiskerCamera::update()
 * through lpVtbl.
 * 
 * Sig: WhiskerCamera::WhiskerCamera():
 * 55 41 57 41 56 56 57 53 48 83 EC 28 48 8D 6C 24
 * ?  41 89 D6 49 89 CF 48 8D 05 ?  ?  ?  ?  48 89
 * 01 48 8B 89 ?  ?  ?  ?  48 85 C9 74 22 41 8B 97
 * 
 * Sig: WhiskerCamera::update():
 * 55 41 57 41 56 41 55 41 54 56 57 53 48 81 EC ?
 * ?  ?  ?  48 8D AC 24 ?  ?  ?  ?  C5 78 29 BD ?
 * ?  ?  ?  C5 78 29 B5 ?  ?  ?  ?  C5 78 29 AD ?
 * ?  ?  ?  C5 78 29 A5 ?  ?  ?  ?  C5 78 29 9D ?
 * ?  ?  ?  C5 78 29 95 ?  ?  ?  ?  C5 78 29 8D ?
 * ?  ?  ?  C5 78 29 85 ?  ?  ?  ?  C5 F8 29 BD ?
 * ?  ?  ?  C5 F8 29 B5 ?  ?  ?  ?  49 89 CD 48 83
 * BA ?  ?  ?  ?  ?  0F 84 
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
  , offset_Player_getPos = 0x01C45070ull
  , offset_World_interactionTest = 0x001B5670ull
  , offset_WhiskerCamera_update = 0x0049D4C0ull;
#endif

#endif
