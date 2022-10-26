#ifndef INOUE_WINDOWS_COMPAT_H
#define INOUE_WINDOWS_COMPAT_H

#include <direct.h>
#include <io.h>
#define F_OK 0
#define strdup _strdup
#define strtok_r strtok_s
#define access _access
#define chdir _chdir
#define unlink _unlink

#endif