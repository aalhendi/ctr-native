#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044ff8-0x80045134
u8 *RECTMENU_DrawTime(int milliseconds)
{
	// 32 is added to milliseconds every frame,
	// 960 per second, the rest is basic math

#ifdef REBUILD_PC
	// NOTE(aalhendi): Retail writes DAT_80099264; native names the same slot.
	char *str = &sdata->ghostStrTrackTime[0];
#else
	char *str = 0x1f800000;
#endif

	// build a string
	sprintf(

	    str,

#ifndef REBUILD_PS1
	    // Format
	    // Minute:Seconds:Milliseconds
	    &rdata.s_timeString[0],
#else
	    "%ld:%ld%ld:%ld%ld",
#endif

	    milliseconds / 0xe100,              // minutes
	    (milliseconds / 0x2580) % 6,        // seconds / 10
	    (milliseconds / 0x3c0) % 10,        // seconds
	    ((milliseconds * 10) / 0x3c0) % 10, // milliseconds / 10
	    ((milliseconds * 100) / 0x3c0) % 10 // milliseconds
	);

	return str;
}

u8 *DECOMP_RECTMENU_DrawTime(int milliseconds)
{
	return RECTMENU_DrawTime(milliseconds);
}
