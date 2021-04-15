#ifndef UTILS_H
#define UTILS_H

#include <Windows.h>

BOOL IsElevated();
HANDLE CreateDummyFile(LPTSTR filePath, LONGLONG size);

#endif