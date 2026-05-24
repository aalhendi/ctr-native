#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e690-0x8002e724
void DECOMP_OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags)
{
	int local = *soundID_Count;

	if (
	    // if this sound is already playing
	    (local != 0) &&

	    // if soundID doesn't match new ID
	    ((local & 0xffff) != newSoundID))
	{
		DECOMP_OtherFX_Stop1(local);

		*soundID_Count = 0;
		local = 0;
	}

	if (newSoundID != (u32)-1)
	{
		// if this is a new sound
		if (local == 0)
		{
			*soundID_Count = DECOMP_OtherFX_Play_LowLevel(newSoundID & 0xffff, 0, modifyFlags);
		}
		// if not a new sound,
		// modification of old sound
		else
		{
			DECOMP_OtherFX_Modify(local, modifyFlags);
		}
	}
}

void OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags)
{
	DECOMP_OtherFX_RecycleNew(soundID_Count, newSoundID, modifyFlags);
}
