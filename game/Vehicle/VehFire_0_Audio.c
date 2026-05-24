#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005ab24-0x8005abfc.
void DECOMP_VehFire_Audio(struct Driver *driver, int speed_cap)
{
	u32 distortion;
	u32 volume;
	u32 extraFlags;

	// if turbo audio cooldown is not done
	if (driver->VehFire_AudioCooldown != 0)
	{
		return;
	}

	if (speed_cap >= 0x80)
	{
		// max volume
		volume = 0xff << 0x10;

		// distort
		distortion = 0x6c << 0x8;

		Voiceline_RequestPlay(0x10, data.characterIDs[driver->driverID], 0x10);

		goto Skip;
	}

	if (speed_cap >= 0x40)
	{
		// 3/4 volume
		volume = 0xc0 << 0x10;

		// no distort
		distortion = 0x80 << 0x8;

		goto Skip;
	}

	// half volume
	volume = 0x80 << 0x10;

	// distort
	distortion = 0x94 << 0x8;

Skip:

	// 50% L/R
	extraFlags = 0x80;

	// if echo is required
	if ((driver->actionsFlagSet & 0x10000) != 0)
	{
		// add echo
		extraFlags |= 0x1000000;
	}

	// 0xD = Turbo Boost Sound
	// 0x80 = balance L/R
	OtherFX_Play_LowLevel(0xd, 1, volume | distortion | extraFlags);

	// turbo audio cooldown 0.24s
	driver->VehFire_AudioCooldown = 0xf0;
}
