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
