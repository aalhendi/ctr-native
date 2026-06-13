#include <common.h>

#if defined(CTR_NATIVE)
static void CS_Credits_RestorePodiumAudioForNativeHandoff(void)
{
	if ((D233.CutsceneManipulatesAudio == 0) || (howl_VolumeGet(0) != 0) || (D233.FXVolumeBackup == 0))
		return;

	// NOTE(aalhendi): Boss scripts can mute HOWL with opcode 0x30; normal
	// podium exits restore it in CS_DestroyPodium_StartDriving. Credits do not
	// return to driving, and their scripts do not apply this mute. Native can
	// still reach this handoff with a leaked FX mute, so restore the podium
	// backup before loading the hub/Scrapbook. A zero backup preserves
	// user-muted SFX.
	howl_VolumeSet(0, (u8)D233.FXVolumeBackup);
	howl_VolumeSet(1, (u8)D233.MusicVolumeBackup);
	howl_VolumeSet(2, (u8)D233.VoiceVolumeBackup);
}
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b93f4-0x800b9488
void CS_Credits_End(void)
{
	int levID;
	struct GameTracker *gGT = sdata->gGT;

	// erase 5 instances
	CS_Credits_DestroyCreditGhost();

	// kill thread
	creditsBSS.CreditThread->flags |= 0x800;

#if defined(CTR_NATIVE)
	CS_Credits_RestorePodiumAudioForNativeHandoff();
#endif

	// go to gemstone valley
	if (creditsBSS.boolAllBlue == 0)
	{
		levID = GEM_STONE_VALLEY;

		gGT->gameMode1 |= ADVENTURE_MODE;
	}

	// go to scrapbook
	else
	{
		sdata->mainMenuState = 5;

		levID = SCRAPBOOK;
	}

	MainRaceTrack_RequestLoad(levID);

	gGT->renderFlags &= 0xfffffffb;
}
