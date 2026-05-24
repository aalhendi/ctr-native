#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800284d0-0x80028690
int DECOMP_OtherFX_Play_LowLevel(u32 soundID, char boolAntiSpam, u32 flags)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ChannelStats *channel;
	int count;
	s16 id;
	struct OtherFX *ptrOtherFX;
	u32 LR = (flags) & 0xff;
	u32 distortion = (flags >> 8) & 0xff;
	u32 volume = (flags >> 0x10) & 0xff;
	u16 echo = (flags >> 0x18) & 0xff;
	struct ChannelAttr channelAttr;

	if (sdata->boolAudioEnabled == 0)
		return 0;

	id = soundID & 0xffff;

	// quit if out of bounds
	if (id >= sdata->ptrHowlHeader->numOtherFX)
		return 0;

	// get pointer to cseq audio, given soundID
	ptrOtherFX = &sdata->howl_metaOtherFX[id];

	// quit if effect is not loaded
	if (sdata->howl_spuAddrs[ptrOtherFX->spuIndex].spuAddr == 0)
		return 0;

	DECOMP_howl_InitChannelAttr_OtherFX(ptrOtherFX, &channelAttr, volume, LR, distortion);

	channelAttr.reverb = echo;

	DECOMP_Smart_EnterCriticalSection();

	// does this ever happen?
	// breakpoint 8002b5b4 in OG CTR
	if (
	    // if can not play with duplicates (at all)
	    (boolAntiSpam == 2) && (DECOMP_Channel_FindSound(id) != 0 // if sound is already playing
	                            ))
	{
		DECOMP_Smart_ExitCriticalSection();
		return 0;
	}

	// This function allows duplicates of functions,
	// but not within 10 frames of each other, depending on boolAntiSpam
	channel = DECOMP_Channel_AllocSlot_AntiSpam(id, boolAntiSpam, 0x7c, &channelAttr);

	if (channel == 0)
	{
		// NOTE(aalhendi): Retail falls through to a PSX null-space read here.
		// Native returns the no-sound result explicitly instead of crashing.
		DECOMP_Smart_ExitCriticalSection();
		return 0;
	}

	if ((ptrOtherFX->flags & 2) != 0)
	{
		channel->flags |= 4;
	}

	// type otherFX
	channel->type = 1;
	channel->unk2 = 0;
	channel->echo = echo;
	channel->vol = volume;
	channel->distort = distortion;
	channel->LR = LR;
	channel->timeLeft = ptrOtherFX->duration;

	// soundID, shift in CountSounds for
	// this specific instance of the sound
	count = DECOMP_CountSounds();
	channel->soundID = (count << 0x10) | id;

	// save the frame that the channel started, frameTimer_MainFrame_ResetDB
	channel->startFrame = gGT->frameTimer_MainFrame_ResetDB;

	DECOMP_Smart_ExitCriticalSection();
	return channel->soundID;
}

int OtherFX_Play_LowLevel(u32 soundID, u8 boolAntiSpam, u32 flags)
{
	return DECOMP_OtherFX_Play_LowLevel(soundID, boolAntiSpam, flags);
}
