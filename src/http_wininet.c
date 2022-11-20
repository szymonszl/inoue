#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinInet.h>

#include "inoue.h"

static HINTERNET internet;

LPCSTR rgpszAcceptTypes[] = {"*/*", NULL};

int
http_init(void)
{
	// cracking open a can of internet
	internet = InternetOpenA("Mozilla/5.0 (only pretending; Inoue/v1; wininet)",
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!internet) {
		logE("failed to open an internet (%ld)", GetLastError());
		return 0;
	}
	return 1;
}

void
http_deinit(void)
{
	InternetCloseHandle(internet);
}

int
http_get(const char *url, buffer *b, long *status)
{
	HINTERNET req = InternetOpenUrlA(
		internet, url, NULL, 0,
		INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_UI
		|INTERNET_FLAG_SECURE|INTERNET_FLAG_NO_CACHE_WRITE,
		0
	);
	if (!req) {
		DWORD err;
		char ebuf[64];
		DWORD buflen = 63;
		InternetGetLastResponseInfoA(&err, ebuf, &buflen);
		logE("failed to open URL: %s (%ld)", ebuf, err);
		return 0;
	}
	DWORD stat;
	DWORD slen = sizeof(DWORD);
	HttpQueryInfoA(
		req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
		&stat, &slen, NULL
	);
	if (status)
		*status = stat;
	char buf[1024];
	DWORD rl;
	while (InternetReadFile(req, buf, 1023, &rl) && rl) {
		buffer_appendbytes(b, buf, rl);
	}
	InternetCloseHandle(req);
	return 1;
}