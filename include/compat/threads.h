#ifndef COMPAT_THREADS_H
#define COMPAT_THREADS_H

#ifndef _WIN32

#include <threads.h>

#else // _WIN32

#include <stdlib.h>
#include <windows.h>

typedef HANDLE thrd_t;
typedef int (*thrd_start_t)(void *);

enum { thrd_success = 0, thrd_nomem = 3, thrd_error = 2 };

typedef struct
{
	thrd_start_t fn;
	void *arg;
} compat_thrd_ctx;

static DWORD WINAPI
compat_thrd_entry(LPVOID p)
{
	compat_thrd_ctx ctx = *(compat_thrd_ctx *) p;
	free(p);
	return (DWORD) ctx.fn(ctx.arg);
}

static inline int
thrd_create(thrd_t *t, thrd_start_t fn, void *arg)
{
	compat_thrd_ctx *ctx = malloc(sizeof(*ctx));
	if (!ctx) return thrd_nomem;
	ctx->fn = fn;
	ctx->arg = arg;
	*t = CreateThread(NULL, 0, compat_thrd_entry, ctx, 0, NULL);
	if (!*t)
	{
		free(ctx);
		return thrd_error;
	}
	return thrd_success;
}

static inline int
thrd_join(thrd_t t, int *res)
{
	if (WaitForSingleObject(t, INFINITE) != WAIT_OBJECT_0) return thrd_error;
	if (res)
	{
		DWORD code = 0;
		GetExitCodeThread(t, &code);
		*res = (int) code;
	}
	CloseHandle(t);
	return thrd_success;
}

#endif // _WIN32

#endif // COMPAT_THREADS_H
