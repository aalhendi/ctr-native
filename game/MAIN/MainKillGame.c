#include <common.h>

// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x8003c41c-0x8003c480.
void MainKillGame_StopCTR(void)
{
	EnterCriticalSection();
	DrawSyncCallback((void (*)(void))sdata->MainDrawCb_DrawSyncPtr);
	ExitCriticalSection();
	StopCallback();

#ifndef CTR_NATIVE
	MEMCARD_CloseCard();
#else
	// NOTE(aalhendi): Native skips PSX memcard event teardown.
#endif

	PadStopCom();
	ResetGraph(3);
	VSyncCallback(0);

	Timer_Destroy();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003c480-0x8003c508 for the retail shutdown path.
void MainKillGame_LaunchSpyro2(void)
{
	CTR_ErrorScreen(0, 0, 0);

	Music_Stop();

	// clear backup, destroy music, destroy all fx
	howl_StopAudio(1, 1, 1);

	Bank_DestroyAll();

	howl_Disable();

	VSync(0x1e);

	MainKillGame_StopCTR();

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native cannot chain-load the Spyro executable.

	while (1)
	{
	}

#else

	_96_remove();
	_96_init();

	LoadExec(rdata.s_PathTo_SpyroExe, 0x801fff00, 0);

#endif
}
