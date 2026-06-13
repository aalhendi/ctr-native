#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80061488-0x8006163c.
int VehPhysGeneral_GetBaseSpeed(struct Driver *driver)
{
	int netSpeed;
	int statAdditional;
	int speedAdditional;
	statAdditional = (int)driver->const_Speed_ClassStat;

	int netWumpaFruitCount = (int)driver->numWumpas;
	if (netWumpaFruitCount > 9)
	{
		netWumpaFruitCount = 9;
	}

	int turboMultiplier = (int)driver->turboConst;
	if (turboMultiplier > 5)
	{
		turboMultiplier = 5;
	}

	int netSpeedStat = CTR_MipsSubLo(CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(driver->const_AccelSpeed_ClassStat, driver->const_Speed_ClassStat), 0xc), 5), 1);

	speedAdditional =
	    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsDiv(CTR_MipsMulLo(netWumpaFruitCount, netSpeedStat), 10), CTR_MipsMulLo(turboMultiplier, netSpeedStat)), 0xc);

	if ((driver->actionsFlagSet & 0x800000) != 0)
	{
		speedAdditional = CTR_MipsAddLo(speedAdditional, driver->const_MaskSpeed);
	}

	if (driver->reserves != 0)
	{
		statAdditional = CTR_MipsAddLo(statAdditional, driver->fireSpeedCap);

		int netSpeedCap = CTR_MipsSubLo(
		    CTR_MipsAddLo(driver->const_SingleTurboSpeed, CTR_MipsSll(CTR_MipsSubLo(driver->const_SacredFireSpeed, driver->const_SingleTurboSpeed), 1)),
		    driver->fireSpeedCap);
		if (netSpeedCap < 0)
		{
			netSpeedCap = 0;
		}

		if (netSpeedCap < speedAdditional)
		{
			speedAdditional = netSpeedCap;
		}
	}

	int subtract = 0;

	if (driver->instTntRecv != 0)
		subtract = CTR_MipsSra(driver->const_DamagedSpeed, 1);

	if (
	    // burn, squish, or raincloud
	    (driver->burnTimer != 0) || (driver->squishTimer != 0) || (driver->driverRankItemValue == 0))
	{
		subtract = driver->const_DamagedSpeed;
	}

	if (driver->clockReceive != 0)
	{
		// NOTE(aalhendi) Retail scales clock damage by rank: stronger near the front, still nonzero near the back.
		int clockEffect = CTR_MipsSra(CTR_MipsMulLo(driver->const_DamagedSpeed, CTR_MipsSubLo(0x14, driver->driverRank)), 4);

		if (subtract < clockEffect)
			subtract = clockEffect;
	}

	netSpeed = CTR_MipsSubLo(CTR_MipsAddLo(statAdditional, speedAdditional), subtract);

	if (0x6400 < netSpeed)
	{
		netSpeed = 0x6400;
	}

	return netSpeed;
}
