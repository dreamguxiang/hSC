#ifndef __SIGCODES_H__
#define __SIGCODES_H__

#include <windows.h>
#include "aliases.h"

#define VERSION_SKY 1

typedef struct {
  const char *sig;
  const char *name;
  i08 indirect;
  i32 offset;
} Signature_t;

static const Signature_t sigE8_SkyCameraProp_updateUI = {
  .sig =
    "4C 8D 84 24 ?  ?  ?  ?  4C 8D 8C 24 ?  ?  ?  ?  "
    "48 89 F1 48 89 FA E8 ?  ?  ?  ?  C5 FA 10 86 ?  ",
  .name = "SkyCameraProp::_updateUI()",
  .indirect = 1,
  .offset = 0x16
};
static const Signature_t sigE8_SkyCameraProp__updateParams = {
  .sig =
    "C6 44 24 ?  ?  48 89 F1 48 89 FA E8 ?  ?  ?  ?  "
    "4C 8D 44 24 ?  48 89 F1 48 89 FA E8 ?  ?  ?  ?  ",
  .name = "SkyCameraProp::_updateParams()",
  .indirect = 1,
  .offset = 0x0B
};
static const Signature_t sigE8_SkyCameraProp_update = {
  .sig =
    "80 ?  ?  ?  ?  ?  ?  74 ?  48 89 ?  48 89 ?  E8 "
    "?  ?  ?  ?  E9 ?  ?  ?  ?  48 8B ?  ?  ?  ?  ?  ",
  .name = "SkyCameraProp::update()",
  .indirect = 1,
  .offset = 0x0F
};
static const Signature_t sigE8_Player_getCameraPos = {
  .sig =
    "48 8B ?  ?  48 8D ?  ?  ?  ?  ?  ?  E8 ?  ?  ?  "
    "?  C5 F8 28 ?  ?  ?  ?  ?  ?  C5 F8 29 ?  ?  ?  ",
  .name = "Player::getCameraPos()",
  .indirect = 1,
  .offset = 0x0C
};
static const Signature_t sigE8_World_interactionTest = {
  .sig =
    "48 8D 95 ?  ?  ?  ?  4C 8D 45 70 C5 F8 28 DE E8 ",
    //"?  ?  ?  ?  84 C0 0F 84",
  .name = "World::interactionTest()",
  .indirect = 1,
  .offset = 0x0F
};
static const Signature_t sig_WhiskerCamera_update = {
  .sig =
    "49 89 ?  48 83 ?  ?  ?  ?  ?  ?  0F 84 ?  ?  ?  "
    "?  ?  89 ?  48 8B ?  ?  ?  ?  ?  48 8D ?  ?  48 "
    "89 ?  45 31 ?  45 31",
  .name = "WhiskerCamera::update()",
  .indirect = 0,
  .offset = -0x6B
};
static const Signature_t sig_SkyCamera_update = {
  .sig =
    "55 56 57 48 83 EC ?  48 8D 6C 24 ?  ?  89 ?  C5 "
    "FA 10 05 ?  ?  ?  ?  C5 F8 29 41 ?  C5 F8 28 05 "
    "?  ?  ?  ?  C5 F8 29 41 ?  48 8D ?  ?  C5 F8 28 ",
  .name = "SkyCamera::update()",
  .indirect = 0,
  .offset = 0x00
};
static const Signature_t sigE8_MainCamera__getDelta = {
  .sig =
    "48 8D ?  ?  ?  ?  ?  48 89 ?  4D 89 ?  4C 8B ?  "
    "?  ?  ?  ?  E8 ?  ?  ?  ?  80 BD",
  .name = "MainCamera::_getDelta()",
  .indirect = 1,
  .offset = 0x14
};

static const Signature_t *funcSig[9] = {
  &sigE8_SkyCameraProp__updateParams,
  &sigE8_SkyCameraProp_updateUI,
  NULL,
  &sigE8_SkyCameraProp_update,
  &sigE8_Player_getCameraPos,
  &sigE8_World_interactionTest,
  &sig_WhiskerCamera_update,
  &sig_SkyCamera_update,
  &sigE8_MainCamera__getDelta
};

#endif
