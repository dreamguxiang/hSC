#include <windows.h>

#include "sigscan.h"
#include "log.h"

static const char *sig_World_interactionTest =
  "48 8D 95 ?  ?  ?  ?  4C 8D 45 70 C5 F8 28 DE E8";
  //"?  ?  ?  ?  84 C0 0F 84";
static const char *sig_SkyCamera_updateUI =
  "4C 8D 84 24 ?  ?  ?  ?  4C 8D 8C 24 ?  ?  ?  ? "
  "48 89 F1 48 89 FA E8 ?  ?  ?  ?  C5 FA 10 86 ? ";
static const char *sig_SkyCamera__updateParams =
  "C6 44 24 ?  ?  48 89 F1 48 89 FA E8 ?  ?  ?  ? "
  "4C 8D 44 24 ?  48 89 F1 48 89 FA E8 ?  ?  ?  ? ";
static const char *sig_WhiskerCamera_update = 
  "49 89 ? 48 83 ? ? ? ? ? ? 0F 84 ? ? ? ? ? 89 ? "
  "48 8B ? ? ? ? ? 48 8D ? ? 48 89 ? 45 31 ? 45 31";

void tryInitWithSig() {
  LOGI("Scan functions...");

  void *fn_SkyCamera__updateParams = sigScanE8(
    "Sky.exe",
    sig_SkyCamera__updateParams,
    0x0B);
  LOGI("SkyCamera::_updateParams(): 0x%p\n", fn_SkyCamera__updateParams);

  void *fn_SkyCamera_updateUI = sigScanE8(
    "Sky.exe",
    sig_SkyCamera_updateUI,
    0x16);
  LOGI("SkyCamera::_updateUI(): 0x%p\n", fn_SkyCamera_updateUI);

  void *fn_World_interactionTest = sigScanE8(
    "Sky.exe",
    sig_World_interactionTest,
    0x0F);
  LOGI("World::interactionTest(): 0x%p\n", fn_World_interactionTest);

  void *fn_WhiskerCamera_update = sigScan(
    "Sky.exe",
    sig_WhiskerCamera_update,
    -0x6B);
  LOGI("WhiskerCamera::update(): 0x%p\n", fn_WhiskerCamera_update);
}
