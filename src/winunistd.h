#ifndef _WINUNISTD_H
#define _WINUNISTD_H

#ifdef _WIN32

#include <direct.h>
#include <io.h>
#define F_OK 0
#define strdup _strdup
#define strtok_r strtok_s
#define access _access
#define chdir _chdir
#define unlink _unlink

#else
#include <unistd.h>
#endif

#endif