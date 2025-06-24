#include <windows.h>

#include "sigscan.h"
#include "log.h"
#include "setup.h"

typedef struct {
  const char *sig;
  const char *name;
  i08 indirect;
  i32 offset;
} Signature_t;

static const Signature_t sigE8_SkyCamera_updateUI = {
  .sig =
    "4C 8D 84 24 ?  ?  ?  ?  4C 8D 8C 24 ?  ?  ?  ?  "
    "48 89 F1 48 89 FA E8 ?  ?  ?  ?  C5 FA 10 86 ?  ",
  .name = "SkyCamera::_updateUI()",
  .indirect = 1,
  .offset = 0x16
};
static const Signature_t sigE8_SkyCamera__updateParams = {
  .sig =
    "C6 44 24 ?  ?  48 89 F1 48 89 FA E8 ?  ?  ?  ?  "
    "4C 8D 44 24 ?  48 89 F1 48 89 FA E8 ?  ?  ?  ?  ",
  .name = "SkyCamera::_updateParams()",
  .indirect = 1,
  .offset = 0x0B
};
static const Signature_t sigE8_SkyCamera_update = {
  .sig =
    "80 ?  ?  ?  ?  ?  ?  74 ?  48 89 ?  48 89 ?  E8 "
    "?  ?  ?  ?  E9 ?  ?  ?  ?  48 8B ?  ?  ?  ?  ?  ",
  .name = "SkyCamera::update()",
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

static const Signature_t *funcSig[7] = {
  &sigE8_SkyCamera__updateParams,
  &sigE8_SkyCamera_updateUI,
  NULL,
  &sigE8_SkyCamera_update,
  &sigE8_Player_getCameraPos,
  &sigE8_World_interactionTest,
  &sig_WhiskerCamera_update
};

i08 setupFuncWithSig(SetupFunctions_t *functions) {
  i08 r = 1;
  i32 length;
  void *p;
  const Signature_t *sig;

  LOGI("Scan functions...\n");

  if (!functions)
    return 0;

  length = sizeof(funcSig) / sizeof(Signature_t *);
  for (i32 i = 0; i < length; i++) {
    sig = funcSig[i];
    if (!sig)
      continue;
    if (sig->indirect)
      p = sigScanE8("Sky.exe", sig->sig, sig->offset);
    else
      p = sigScan("Sky.exe", sig->sig, sig->offset);

    if (p)
      LOGI("Found %s: 0x%p\n", sig->name, p);
    else {
      LOGI("Scan %s failed!\n", sig->name);
      r = 0;
    }
  }
  
  return r;
}

i08 setupPaths(HMODULE hModule) {
  wchar_t dllPath[MAX_PATH + 11]
    , *p;

  if (!GetModuleFileNameW(hModule, dllPath, MAX_PATH))
    return 0;
  p = wcsrchr(dllPath, L'\\');
  if (!p)
    return 0;
  *p = 0;
  wcscat_s(dllPath, MAX_PATH, L"\\.hsc-data");

  return 1;
}
