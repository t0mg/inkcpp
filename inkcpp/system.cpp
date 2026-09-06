/* Copyright (c) 2024 Julian Benda
 *
 * This file is part of inkCPP which is released under MIT license.
 * See file LICENSE.txt or go to
 * https://github.com/JBenda/inkcpp for full license details.
 */
#include "system.h"

#ifdef _WIN32
#include <windows.h>
#include <cstdio>

namespace {
LONG WINAPI GlobalCrashHandler(EXCEPTION_POINTERS* pException)
{
	fprintf(stderr, "\n=== SYSTEM CRASH DETECTED! ExceptionCode = 0x%08X at Address = %p ===\n",
	        (unsigned)pException->ExceptionRecord->ExceptionCode,
	        pException->ExceptionRecord->ExceptionAddress);
	fflush(stderr);
	return EXCEPTION_EXECUTE_HANDLER;
}

struct GlobalInitCrashHandler {
	GlobalInitCrashHandler() {
		SetUnhandledExceptionFilter(GlobalCrashHandler);
	}
} g_globalCrashHandler;
}
#endif

#ifndef INK_ENABLE_UNREAL

namespace ink
{
jmp_buf g_ink_jmp_buf;
bool g_ink_has_jmp_buf = false;
char g_ink_last_error[256] = {0};

#	define A      54059 /* a prime */
#	define B      76963 /* another prime */
#	define C      86969 /* yet another prime */
#	define FIRSTH 37    /* also prime */

hash_t hash_string(const char* string)
{
	hash_t h = FIRSTH;
	while (*string) {
		h = (h * A) ^ (string[0] * B);
		string++;
	}
	return h; // or return h % C;
}

hash_t hash_data(const unsigned char* data, size_t len)
{
	hash_t h = FIRSTH;
	for (size_t i = 0; i < len; ++i) {
		h = (h * A) ^ (data[i] * B);
	}
	return h; // or return h % C;
}

namespace internal
{
	void zero_memory(void* buffer, size_t length)
	{
		char* buf = static_cast<char*>(buffer);
		for (size_t i = 0; i < length; i++)
			*(buf++) = 0;
	}
} // namespace internal
} // namespace ink

#endif
