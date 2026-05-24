#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80050528-0x80050654
void UI_DrawLapCount(s16 posX, int posY, int param_3, struct Driver *d)
{
	s16 type;
	s16 currLap;
	int numLaps;
	int flags;
	char message[24];
	char *str;

	struct GameTracker *gGT;
	int numPlyrCurrGame;

	gGT = sdata->gGT;
	numLaps = gGT->numLaps;
	numPlyrCurrGame = gGT->numPlyrCurrGame;

	currLap = d->lapIndex + 1;

	if (currLap > numLaps)
		currLap = numLaps;

	// 3P or 4P
	type = FONT_SMALL;
	flags = PERIWINKLE;

	// 1P or 2P
	if (numPlyrCurrGame < 3)
	{
		// LAP
		DECOMP_DecalFont_DrawLine(sdata->lngStrings[0x60 / 4], posX, posY, FONT_SMALL, (JUSTIFY_RIGHT | PERIWINKLE));

		sprintf(&message[0], &sdata->s_intDividing[0], currLap, numLaps);
		str = &message[0];
		type = FONT_BIG;
		flags = (JUSTIFY_RIGHT | PERIWINKLE);
	}
	else
	{
		str = &sdata->s_printDividing[0];
		str[0] = currLap + '0';
		str[2] = numLaps + '0';

		type = FONT_SMALL;
		flags = PERIWINKLE;
	}

	// draw string
	DECOMP_DecalFont_DrawLine(str, posX, (posY + 8), type, flags);
}

void DECOMP_UI_DrawLapCount(s16 posX, int posY, int param_3, struct Driver *d)
{
	UI_DrawLapCount(posX, posY, param_3, d);
}
