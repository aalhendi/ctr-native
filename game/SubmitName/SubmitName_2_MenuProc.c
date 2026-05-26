#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b144-0x8004b230.
void SubmitName_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;

	s16 selection = SubmitName_DrawMenu(0x13f);
	menu->rowSelected = selection;

	// not finished yet
	if (selection == 0)
	{
		return;
	}

	// if name entered for Time Trial
	if (sdata->data10_bbb[0xd] == 1)
	{
		// if hit CANCEL
		if (selection < 0)
		{
			// end of race menu with "Save Ghost" option
			extern struct RectMenu menu224;
			sdata->ptrDesiredMenu = &menu224;
		}

		// if hit SAVE
		else
		{
			// GhostMode
			SelectProfile_ToggleMode(0x31);
			sdata->ptrDesiredMenu = &data.menuGhostSelection;
		}
	}

	// if name entered for Adventure
	else if (sdata->data10_bbb[0xd] == 0)
	{
		// if hit CANCEL
		if (selection < 0)
		{
			// Change active Menu back to Adv char select
			sdata->ptrDesiredMenu = CS_Garage_GetMenuPtr();
			CS_Garage_ZoomOut(1);
		}
		else
		{
			// make backup of name entered
			memmove(sdata->advProgress.name, gGT->prevNameEntered, sizeof(gGT->prevNameEntered));

			// AdventureMode
			SelectProfile_ToggleMode(1);
			sdata->ptrDesiredMenu = &data.menuFourAdvProfiles;
		}
	}
}
