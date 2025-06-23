#include <windows.h>

#include "aliases.h"

void *sigScan(const char *moduleName, char *sig) {
  HINSTANCE handle;
  PIMAGE_DOS_HEADER dosHeader;
  PIMAGE_NT_HEADERS ntHeaders;
  u64 imageSize, matchLen, l, i, s;
  i16 *pattern;
  i08 *image
    , found, matchCtr;
  char *p;
  void *match;

  if (!moduleName || !sig)
    return NULL;

  handle = GetModuleHandleA(moduleName);
  if (!handle)
    return NULL;

  dosHeader = (PIMAGE_DOS_HEADER)handle;
  ntHeaders = (PIMAGE_NT_HEADERS)((u08 *)handle + dosHeader->e_lfanew);
  imageSize = ntHeaders->OptionalHeader.SizeOfImage;

  l = strlen(sig);
  if (l <= 1)
    return NULL;
  pattern = malloc(l >> 1);

  for (p = sig, i = 0; p < (sig + l); p++) {
    if (*p == '?') {
      p++;
      if (*p == '?')
        p++;
      pattern[i] = 0xFFFF;
    } else
      pattern[i] = strtoul(p, &p, 16);
    i++;
  }

  s = i;
  image = (i08 *)handle;
  matchLen = 0;
  matchCtr = 0;
  // x64 functions is aligned to 16 bytes.
  for (u64 i = 0; i < imageSize - s; i += 0x10) {
    found = 1;
    for (u64 j = 0; j < s; j++) {
      if (image[i + j] != pattern[j] && pattern[j] != 0xFFFF) {
        if (matchLen < j) {
          found = 0;
          matchLen = j;
          matchCtr = 1;
          match = (void *)&image[i];
        } else if (matchLen == j)
          matchCtr++;
        break;
      }
    }

    if (found)
      return (void *)&image[i];
  }

  if (matchCtr > 1 || !matchCtr)
    return NULL;

  return match;
}
