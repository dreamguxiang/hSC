#include <windows.h>

#include "sigscan.h"
#include "log.h"
#include "setup.h"
#include "sigcodes.h"

i08 setupFuncWithSig(SetupFunctions_t *functions) {
  i08 r = 1;
  i32 length;
  void *p;
  const Signature_t *sig;

  LOGI("Scaning functions...\n");

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
    
    functions->functions[i] = p;

    if (p)
      LOGI("Found %s: 0x%p\n", sig->name, p);
    else {
      LOGE("Scan %s failed!\n", sig->name);
      r = 0;
    }
  }
  
  return r;
}

i08 setupPaths(HMODULE hModule, char *prefPath, char *guiIniPath) {
  char dllPath[MAX_PATH]
    , *p;
  u32 len;

  len = GetModuleFileNameA(hModule, dllPath, MAX_PATH);
  if (!len || len >= MAX_PATH)
    return 0;
  p = strrchr(dllPath, '\\');
  if (!p)
    return 0;
  *p = 0;
  strcpy_s(prefPath, MAX_PATH, dllPath);
  strcat_s(prefPath, MAX_PATH, "\\.hsc-data");

  strcpy_s(guiIniPath, MAX_PATH, dllPath);
  strcat_s(guiIniPath, MAX_PATH, "\\.hsc-gui.ini");

  return 1;
}
