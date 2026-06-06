#include "../platform.h"
#include "ctr_scratchpad.h"

#include <macros.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include "platform/native_win32.h"
#elif defined(__GNUC__)
#include <errno.h>
#include <sys/mman.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#ifndef CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
#define CTR_NATIVE_MEMPACK_RETAIL_PRESSURE 1
#endif

// TODO(aalhendi): Re-audit LOAD_ReadFile_ex, LOAD_DramFileCallback, LEV/PTR
// callbacks, and hub swapping before removing the expanded arena escape hatch.
#if CTR_NATIVE_MEMPACK_RETAIL_PRESSURE
// NOTE(aalhendi): Retail pressure mode exposes the NTSC-U 926 mempack window
// inside a 2 MiB backing store.
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  0x200000u
#define CTR_NATIVE_MEMPACK_START_OFFSET 0xba9f0u
#define CTR_NATIVE_MEMPACK_SIZE         0x144e10u
#else
#define CTR_NATIVE_MEMPACK_BUFFER_SIZE  (8u * 1024u * 1024u)
#define CTR_NATIVE_MEMPACK_START_OFFSET 0u
#define CTR_NATIVE_MEMPACK_SIZE         CTR_NATIVE_MEMPACK_BUFFER_SIZE
#endif

global_variable char s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
global_variable struct PlatformMempackArena s_mempackArena;

void Platform_InitScratchpad(void)
{
#if defined(CTR_NATIVE)
	void *scratchpad = (void *)CTR_SCRATCHPAD_ADDR;
	size_t scratchpadSize = CTR_SCRATCHPAD_MAP_SIZE;

#if defined(_WIN32)
	void *mapped = VirtualAlloc(scratchpad, scratchpadSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (mapped == NULL)
	{
		fprintf(stderr, "[CTR Native] Failed to map PS1 scratchpad at %p: GetLastError=%lu\n", scratchpad, GetLastError());
		abort();
	}
#elif defined(__GNUC__)
#ifdef MAP_FIXED_NOREPLACE
	int fixedFlag = MAP_FIXED_NOREPLACE;
#else
	int fixedFlag = MAP_FIXED;
#endif

	void *mapped = mmap(scratchpad, scratchpadSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | fixedFlag, -1, 0);
	if (mapped == MAP_FAILED)
	{
		fprintf(stderr, "[CTR Native] Failed to map PS1 scratchpad at %p: %s\n", scratchpad, strerror(errno));
		abort();
	}
#else
#error "Platform_InitScratchpad needs a fixed-address virtual-memory mapper for this platform"
#endif

	if (mapped != scratchpad)
	{
		fprintf(stderr, "[CTR Native] PS1 scratchpad mapped at %p, expected %p\n", mapped, scratchpad);
		abort();
	}

	memset(mapped, 0, scratchpadSize);
#endif
}

internal void Platform_ConfigureMempackArena(void)
{
	s_mempackArena.base = &s_mempackMemory[0];
	s_mempackArena.start = &s_mempackMemory[CTR_NATIVE_MEMPACK_START_OFFSET];
	s_mempackArena.endOfMemory = &s_mempackMemory[CTR_NATIVE_MEMPACK_BUFFER_SIZE];
	s_mempackArena.size = CTR_NATIVE_MEMPACK_SIZE;
	s_mempackArena.backingSize = CTR_NATIVE_MEMPACK_BUFFER_SIZE;

	// NOTE(aalhendi): Native still uses PS1-shaped OT links for many render
	// paths. MEMPACK must stay below 0x01000000 so CtrGpu_PrimToOTLink24 can
	// pack linked primitive pointers without losing address bits.
	s_mempackArena.lowAddressValid =
	    ((u32)s_mempackArena.base < 0x01000000) && ((u32)s_mempackArena.start < 0x01000000) && ((u32)s_mempackArena.endOfMemory <= 0x01000000);
}

const struct PlatformMempackArena *Platform_InitMempackArena(void)
{
	memset(s_mempackMemory, 0, sizeof(s_mempackMemory));
	Platform_ConfigureMempackArena();

	return &s_mempackArena;
}

internal void Platform_RepairResidentPointers(s32 activeMempackIndex)
{
	if ((activeMempackIndex < 0) || (activeMempackIndex >= 4))
		activeMempackIndex = 0;

	// NOTE(aalhendi): Native keeps retail-shaped global data, but pointer aliases
	// must target this process's static storage. This also moves GCC's
	// initializer-only memcard helper global out of the live state graph so
	// checkpoints capture the actual memcard buffer.
	sdata = &sdata_static;
	sdata_static.gGT = &sdata_static.gameTracker;
	sdata_static.gGamepads = &sdata_static.gamepadSystem;
	sdata_static.PtrMempack = &sdata_static.mempack[activeMempackIndex];
	sdata_static.ptrToMemcardBuffer1 = (int)&sdata_static.memcardBytes[0];
	sdata_static.ptrToMemcardBuffer2 = &sdata_static.memcardBytes[0];
}
