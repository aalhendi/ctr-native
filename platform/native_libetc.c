/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/LIBETC.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <psx/libetc.h>

void (*vsync_callback)(void) = NULL;
global_variable int s_videoMode = -1;

int VSyncCallback(void (*func)(void))
{
	int old = (int)vsync_callback;

	vsync_callback = func;
	return old;
}

int StopCallback(void)
{
	return 0;
}

int ResetCallback(void)
{
	int old = (int)vsync_callback;

	vsync_callback = NULL;
	return old;
}

int SetVideoMode(int mode)
{
	int old = s_videoMode;

	s_videoMode = mode;
	return old;
}

int GetVideoMode(void)
{
	return s_videoMode;
}
