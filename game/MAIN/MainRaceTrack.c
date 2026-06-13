#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003cf7c-0x8003cfc0.
void MainRaceTrack_StartLoad(s16 levelID)
{
	// clear backup,
	// keep music,
	// destroy "most" fx, let menu fx play to end
	howl_StopAudio(1, 0, 0);

	ElimBG_Deactivate(sdata->gGT);

	LOAD_LevelFile(levelID);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003cfc0-0x8003d024.
void MainRaceTrack_RequestLoad(s16 levelID)
{
	// Turn off HUD
	sdata->gGT->hudFlags &= 0xfe;

	if (RaceFlag_IsFullyOffScreen() == 1)
	{
		RaceFlag_BeginTransition(1);
	}
	RaceFlag_ResetTextAnim();

	sdata->Loading.stage = -4;
	sdata->Loading.Lev_ID_To_Load = levelID;
	return;
}
