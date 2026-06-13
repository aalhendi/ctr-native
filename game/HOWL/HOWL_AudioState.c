#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d2f4-0x8002d4cc
void Audio_SetState(u32 state)
{
	u8 XA_type;
	u8 XA_index;

	int iVar1;

	struct GameTracker *gGT = sdata->gGT;
	u16 level;

	XA_type = CDSYS_XA_TYPE_MUSIC;

	switch (state)
	{
	// stop/pause cseq music (main: case 2)
	case 1:
		sdata->boolNeedXASeek = 0;

		CDSYS_XAPauseRequest();

		Music_Restart();

		// erase backup, keep music, stop all fx
		howl_StopAudio(1, 0, 1);
		break;
	case 2:
	case 7:

		CseqMusic_StopAll();

		Music_Adjust(0, 0, 0, 0);

		break;
	case 5:

		CseqMusic_StopAll();

		level = gGT->levelID;

		// Level ID on Adventure Arena
		if (level - 0x19U < 5)
		{
			// convert levelID to a bitshifted flag
			Music_Adjust(0, 0, &sdata->advHubSongSet, 1 << (level - 0x19U));
		}
		break;
	case 9:
		XA_index = sdata->desiredXA_RaceIntroIndex;
		sdata->desiredXA_RaceIntroIndex += 1;
		sdata->desiredXA_RaceIntroIndex &= 3;
		goto PLAY_XA;

	case 10:

		Music_Stop();

		CseqMusic_StopAll();
		break;
	case 11:

		sdata->WrongWayDirection_bool = false;

		sdata->framesDrivingSameDirection = 0;

		Voiceline_ToggleEnable(1);
		break;

	case 12:

	// last lap, distToFinish < 9000
	case 15:

		Voiceline_ToggleEnable(0);

		sdata->boolNeedXASeek = 1;
		break;

	// if you are on last lap
	case 13:
		sdata->boolNeedXASeek = 0;

		Music_LowerVolume();

		// MUSIC_LAST_LAP
		XA_index = 6;

		goto PLAY_XA;
	case 14:

		sdata->WrongWayDirection_bool = false;

		sdata->framesDrivingSameDirection = 0;

		Music_RaiseVolume();

		Voiceline_ToggleEnable(1);

		break;
	case 16:
		sdata->boolNeedXASeek = 0;

		Music_Restart();

		// set XA
		XA_index = sdata->desiredXA_RaceEndIndex;

		if (61 < XA_index)
		{
			XA_type = CDSYS_XA_TYPE_EXTRA;
		}

	PLAY_XA:

		CDSYS_XAPlay(XA_type, XA_index);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d4cc-0x8002d50c
void Audio_SetState_Safe(int state)
{
	// If this sound isn't already playing
	if (state != sdata->unkAudioState)
	{
		Voiceline_EmptyFunc();

		Audio_SetState(state);

		// set which sound is playing
		sdata->unkAudioState = state;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d50c-0x8002d554
void Audio_AdvHub_SwapSong(int levelID)
{
	if ((sdata->unkAudioState == 5) &&
	    // If you're on a map in the Adventure Arena
	    (levelID - GEM_STONE_VALLEY < 5))
	{
		CseqMusic_AdvHubSwap(0, &sdata->advHubSongSet, (1 << (levelID - GEM_STONE_VALLEY)));
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d554-0x8002d67c
void Audio_SetMaskSong(u32 tempo)
{
	char i;
	u8 isMaskUsed;
	u32 songID;
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode = gGT->gameMode1;

	// Assume no player is using a mask
	isMaskUsed = false;

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		// if player is using mask weapon
		if ((gGT->drivers[i]->actionsFlagSet & 0x800000) != 0)
		{
			// There is at least one player using a mask
			isMaskUsed = true;
		}
	}

	// If any player is using a mask
	if (isMaskUsed)
	{
		// Uka song is playing
		songID = 2;

		if (
		    // If Uka song is playing
		    ((gameMode & UKA_SONG) != 0) ||

		    (
		        // Aku song is playing
		        songID = 1,

		        // If Aku song is playing
		        (gameMode & AKU_SONG) != 0))
		{
			Music_Adjust(songID, tempo, 0, 0);
		}
	}

	// If no players are using mask
	else
	{
		Music_Adjust(0, tempo, 0, 0);

		if ((gameMode & (UKA_SONG | AKU_SONG)) != 0)
		{
			gGT->gameMode1 &= ~(UKA_SONG | AKU_SONG);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d67c-0x8002dc4c; CTR_NATIVE guards demo null-driver reads.
void Audio_Update1(void)
{
	char i;
	int raceOrderIndex;
	s16 uVar1;
	u32 uVar2;
	struct Driver *d = 0;
	u32 maskTempo;
	int iVar7;
	struct GameTracker *gGT = sdata->gGT;

	switch (sdata->unkAudioState - 1)
	{
	case 5:
		if ((sdata->XA_State == 0) || (sdata->XA_Playing_Category != 0))
		{
			Audio_SetState_Safe(5);
		}
		break;
	case 7:
		Garage_LerpFX();
		break;
	case 8:
		if (sdata->XA_State == 0)
		{
			// 9 means intro cutscene
			// 10 means traffic lights
			// 11 means racing

			// Change state to traffic lights
			Audio_SetState_Safe(10);
		}
		break;
	case 9:
		// If traffic lights finish counting down
		// from 0x3840 to zero
		if (gGT->trafficLightsTimer < 1)
		{
			// 9 means intro cutscene
			// 10 means traffic lights
			// 11 means racing

			// Change State to 11, which means racing
			Audio_SetState_Safe(11);
		}
		break;
	case 10:
		Audio_SetMaskSong(0);

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && ((d->actionsFlagSet & 0x100000) == 0))
			{
				break;
			}
			d = 0;
		}

		Voiceline_Update();

		Level_AmbientSound();

#if defined(CTR_NATIVE)
		// Demo mode converts P1 to ACTION_BOT, leaving retail's human-driver
		// scan empty. PS1 can read low RAM through a null pointer; host cannot.
		if (d == NULL)
		{
			break;
		}
#endif

		// if race has more than 2 laps
		if ((2 < sdata->gGT->numLaps) &&
		    // if you are on 2nd to last lap
		    ((d->lapIndex == sdata->gGT->numLaps - 2U)) &&
		    // distToFinish is small
		    (d->distanceToFinish_curr < 9000))
		{
			Audio_SetState_Safe(12);
		}
		break;
	case 11:
		Audio_SetMaskSong(0);

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && ((d->actionsFlagSet & 0x100000) == 0))
			{
				break;
			}
			d = 0;
		}

#if defined(CTR_NATIVE)
		if (d == NULL)
		{
			if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == 0)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
			{
				sdata->boolNeedXASeek = 0;
			}

			Level_AmbientSound();
			break;
		}
#endif

		// if need to XASeek
		if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == 0)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
		{
			// far from finish line
			if (2000 < d->distanceToFinish_curr)
			{
				// (FINAL LAP! music)
				CDSYS_XASeek(1, 0, 6);
			}

			// dont need to XASeek
			sdata->boolNeedXASeek = 0;
		}

		Level_AmbientSound();

		// if driver is on final lap
		if (d->lapIndex == gGT->numLaps - 1U)
		{
			// Play final lap sound
			Audio_SetState_Safe(13);
		}
		break;
	case 12:
		maskTempo = 0;

		// if XA has been playing more than a second
		if (0xe1 < sdata->XA_CurrOffset)
		{
			Music_RaiseVolume();

			maskTempo = 20;
		}

		Audio_SetMaskSong(maskTempo);

		Level_AmbientSound();

		if (sdata->XA_State == 0)
		{
			Audio_SetState_Safe(14);
		}
		break;
	case 13:
		Audio_SetMaskSong(20);

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && ((d->actionsFlagSet & 0x100000) == 0))
			{
				break;
			}
			d = 0;
		}

		Voiceline_Update();

		Level_AmbientSound();

#if defined(CTR_NATIVE)
		if (d == NULL)
		{
			break;
		}
#endif

		if (
		    // if driver's lap is the last lap
		    (d->lapIndex == gGT->numLaps - 1U) &&

		    // if finish line is close
		    (d->distanceToFinish_curr < 9000))
		{
			Audio_SetState_Safe(15);
		}

		break;
	case 14:
		raceOrderIndex = -1;

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && (d->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
			{
				raceOrderIndex = i;
				break;
			}
			d = 0;
		}

#if defined(CTR_NATIVE)
		if (d == NULL)
		{
			if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == 0)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
			{
				sdata->boolNeedXASeek = 0;
			}

			Level_AmbientSound();
			break;
		}
#endif

		// if need to XASeek
		if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == 0)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
		{
			// far from finish line
			if (2000 < d->distanceToFinish_curr)
			{
				CDSYS_XASeek(1, 0, 4);
			}

			// dont need to XASeek
			sdata->boolNeedXASeek = 0;
		}

		Level_AmbientSound();

		// if the race is over for this racer
		if ((d->actionsFlagSet & 0x2000000) != 0)
		{
			// if did not just open N Tropy
			if ((gGT->gameModeEnd & NTROPY_JUST_OPENED) == 0)
			{
				// If you did not just beat N Tropy in Time Trial
				if ((gGT->gameModeEnd & NTROPY_JUST_BEAT) == 0)
				{
					// defeat music
					uVar1 = 5;

					if ((raceOrderIndex == 0) || ((gGT->gameMode1 & ADVENTURE_CUP) != 0) || ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
					{
						OtherFX_Play(0x5f, 0);

						// victory (fanfare) music
						uVar1 = 4;
					}
				}

				// If you just beat N Tropy in Time Trial
				else
				{
					// Check if N Tropy has been beaten on all tracks
					uVar2 = GAMEPROG_CheckGhostsBeaten(1);

					// If there is a track where N Tropy has not been beaten
					if ((uVar2 & 0xffff) == 0)
					{
						// OtherFX_Play
						OtherFX_Play(0x5f, 0);

						// count how many times N Tropy has been beaten
						// during this playthrough
						iVar7 = sdata->nTropyVoiceCount;
						sdata->nTropyVoiceCount = iVar7 + 1;

						// pick an N Tropy XA voiceline
						uVar1 = data.nTropyXA[iVar7];

						// if beaten more than 5 times
						if (5 < sdata->nTropyVoiceCount)
						{
							// go back to 0
							sdata->nTropyVoiceCount = 0;
						}
					}

					// If N Tropy has been beaten on all tracks
					else
					{
						OtherFX_Play(0x5f, 0);

						// N Tropy Unlocked XA
						// "you've earned the right to choose me in character selection"
						uVar1 = 0x43;
					}
				}
			}

			// if n tropy just opened
			else
			{
				// OtherFX_Play
				OtherFX_Play(0x5f, 0);

				// N Tropy opened
				// "Think your fast eh, well, lets see if you can beat my fastest time"
				uVar1 = 0x46;
			}

			// desired XA
			sdata->desiredXA_RaceEndIndex = uVar1;

			Audio_SetState_Safe(0x10);
		}
		break;
	case 15:

		Level_AmbientSound();

		if (sdata->XA_State == 0)
		{
			Audio_SetMaskSong(0);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002dc4c-0x8002dcac
void Audio_SetDefaults(void)
{
	sdata->audioDefaults[0] = 0;
	sdata->audioDefaults[1] = 0;
	sdata->audioDefaults[2] = 0;
	sdata->audioDefaults[3] = 0;

	sdata->audioDefaults[4] = 0x80;
	sdata->audioDefaults[5] = 0x80;
	sdata->audioDefaults[6] = 0;
	sdata->audioDefaults[7] = 0;
	sdata->audioDefaults[8] = 0;

	// erase both SoundFadeInput
	sdata->SoundFadeInput[0].unk = 0;
	sdata->SoundFadeInput[0].desiredVolume = 0;
	sdata->SoundFadeInput[0].currentVolume = 0;
	sdata->SoundFadeInput[0].soundID_soundCount = 0;
	sdata->SoundFadeInput[1].unk = 0;
	sdata->SoundFadeInput[1].desiredVolume = 0;
	sdata->SoundFadeInput[1].currentVolume = 0;
	sdata->SoundFadeInput[1].soundID_soundCount = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002dcac-0x8002dd24
void Audio_SetReverbMode(int levelID, u32 isBossRace, int bossID)
{
	u32 reverb;
	// if audio is enabled
	if (sdata->boolAudioEnabled != false)
	{
		// If this is not a boss race
		if (isBossRace == 0)
		{
			// Level ID < 30
			// If Level ID is any level you can drive on,
			// including adventure maps
			if (levelID < INTRO_RACE_TODAY)
			{
				// get reverb based on level ID
				reverb = data.reverbMode[levelID];
			}

			// If this is not a level you can drive on:
			// menu, cutscene, etc
			else
			{
				reverb = 4;
			}
		}

		// If this is a boss race
		else
		{
			// if invalid bossID
			if (5 < bossID)
			{
				// quit
				return;
			}

			// get reverb based on boss
			reverb = sdata->reverbModeBossID[bossID];
		}

		SetReverbMode(reverb);
	}
}
