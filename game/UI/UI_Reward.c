#include <common.h>

static inline s32 UI_SaveLapTime_MipsMul(s32 lhs, s32 rhs)
{
	return (s32)((u32)lhs * (u32)rhs);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004c55c-0x8004c718.
void UI_SaveLapTime(int numLaps, int lapTime, s16 driverID)
{
	int numMinutes;
	int PlayerLapIndex;

	// PlayerLapIndex
	// Goes from 0 to 0xd, 0-6 are laps 1-7 for Player 1 in the p1 variables of the LapTimes struct
	// The 7-0xd range skips over to the p2 variables in the same struct for Player 2's lap 1-7
	// Like so: sdata->LapTimes.p1_Min1s[7] is sdata->LapTimes.p2_Min1s[0]
	PlayerLapIndex = ((int)driverID * 7 + numLaps);

	numMinutes = lapTime / 0xe100;

	// number of minutes
	sdata->LapTimes.p1_Min1s[PlayerLapIndex] = numMinutes;

	// if number of minutes is more than 9
	if (9 < numMinutes)
	{
		// rig to 9:59:99
		sdata->LapTimes.p1_Min1s[PlayerLapIndex] = 9;
		sdata->LapTimes.p1_Sec10s[PlayerLapIndex] = 5;
		sdata->LapTimes.p1_Sec1s[PlayerLapIndex] = 9;
		sdata->LapTimes.p1_Ms10s[PlayerLapIndex] = 9;
		sdata->LapTimes.p1_Ms1s[PlayerLapIndex] = 9;
		return;
	}

	// calculate proper lap time
	sdata->LapTimes.p1_Sec10s[PlayerLapIndex] = (lapTime / 0x2580) % 6;
	sdata->LapTimes.p1_Sec1s[PlayerLapIndex] = (lapTime / 0x3c0) % 10;
	sdata->LapTimes.p1_Ms10s[PlayerLapIndex] = (UI_SaveLapTime_MipsMul(lapTime, 10) / 0x3c0) % 10;
	sdata->LapTimes.p1_Ms1s[PlayerLapIndex] = (UI_SaveLapTime_MipsMul(lapTime, 100) / 0x3c0) % 10;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004c718-0x8004c850.
void UI_ThTick_CountPickup(struct Thread *bucket)
{
	struct GameTracker *gGT;
	s16 rotSpd;
	u32 flags;
	MATRIX *mat;

	gGT = sdata->gGT;

	// object from thread
	struct UiElement3D *obj = bucket->object;

	// instance from thread
	struct Instance *inst = bucket->inst;

	// instance color
	inst->colorRGBA = 0xffff0000;

	// if numPlyrCurrGame is 1
	if (gGT->numPlyrCurrGame == 1)
	{
		// if instance->model->modelID is not timebox
		if (inst->model->id != STATIC_TIME_CRATE_01)
		{
			// If player's wumpa is less than 10
			if (gGT->drivers[0]->numWumpas < 10)
			{
				// no shine
				inst->alphaScale = 0;
			}
			else
			{
				// wumpaShineResult
				inst->alphaScale = ((s16)sdata->wumpaShineResult + -0x80) * 0x10;
			}
			goto LAB_8004c7a4;
		}
	}

	// if numPlyrCurrGame is not 1
	else
	{
	LAB_8004c7a4:

		// if HUD item is not timecrate
		if (inst->model->id != STATIC_TIME_CRATE_01)
		{
			// rotation speed 0x80
			rotSpd = obj->rot[1] + 0x80;
			goto LAB_8004c7d4;
		}
	}

	// if wumpa or crystal,
	// rotation speed 0x40
	rotSpd = obj->rot[1] + 0x40;

LAB_8004c7d4:

	obj->rot[1] = rotSpd;
	mat = &inst->matrix;

	ConvertRotToMatrix(mat, (s16 *)obj->rot);

	MatrixRotate(mat, &obj->m, mat);

	// if hud is enabled, and this is not demo mode
	if ((*(int *)&gGT->bool_DrawOTag_InProgress & 0xff0100) == 0x100)
	{
		// make visible
		flags = inst->flags & 0xffffff7f;
	}

	else
	{
		// make invisible
		flags = inst->flags | 0x80;
	}

	inst->flags = flags;
	return;
}

// Draw various objects, like relic,
// key trophy, token, crystal, etc
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004c850-0x8004c914.
void UI_ThTick_Reward(struct Thread *bucket)

{
	u32 flags;
	MATRIX *mat;
	struct GameTracker *gGT;
	struct UiElement3D *obj;
	struct Instance *inst;

	gGT = sdata->gGT;

	// Get instance
	inst = bucket->inst;

	// Get object
	obj = bucket->object;

	// Spin on the Y axis
	obj->rot[1] += 0x40;

	Vector_SpecLightSpin2D(inst, (s16 *)obj->rot, (s16 *)obj->lightDir);

	// pointer to matrix
	mat = &inst->matrix;

	ConvertRotToMatrix(mat, (s16 *)obj->rot);

	MatrixRotate(mat, &obj->m, mat);

	if (
	    // if hud is enabled, and this is not demo mode
	    ((*(int *)&gGT->bool_DrawOTag_InProgress & 0xff0100) == 0x100) &&

	    // if any fade-in-from-black transition is over
	    (0xfff < gGT->pushBuffer_UI.fadeFromBlack_currentValue))
	{
		// make visible
		flags = inst->flags & 0xffffff7f;
	}
	else
	{
		// make invisible
		flags = inst->flags | 0x80;
	}
	inst->flags = flags;
	return;
}

// Handle CTR letters in HUD
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004c914-0x8004ca04.
void UI_ThTick_CtrLetters(struct Thread *bucket)

{
	int endOfRaceTransition;
	struct Instance *inst;
	struct UiElement3D *obj;
	s16 rot[3];

	// thread->instance
	inst = bucket->inst;

	// thread->object
	obj = bucket->object;

	// I know they dont look like they rotate in HUD,
	// believe, me, there is "rotation" to some degree,
	// that's why the lighting changes in the HUD

	// Rotate on the Y axis
	obj->rot[1] += 0x40;

	Vector_SpecLightSpin2D(inst, (s16 *)obj->rot, (s16 *)obj->lightDir);

	if (
	    // If you're in End-Of-Race menu
	    ((sdata->gGT->gameMode1 & END_OF_RACE) != 0) &&

	    (RaceFlag_IsTransitioning() != 0))
	{
		// Set Scale to zero, basically stop
		// drawing letters in the HUD
		inst->scale[0] = 0;
		inst->scale[1] = 0;
		inst->scale[2] = 0;
	}

	if (inst->scale[0] == 0x800)
	{
		rot[1] = 0;
	}
	else
	{
		endOfRaceTransition = (int)inst->scale[0] + -0x800;
		if (endOfRaceTransition < 0)
		{
			endOfRaceTransition = (int)inst->scale[0] + -0x401;
		}
		rot[1] = ((s16)(endOfRaceTransition >> 10) + 1) * 0x200;
	}
	rot[0] = 0;
	rot[2] = 0;

	ConvertRotToMatrix(&inst->matrix, &rot[0]);

	MatrixRotate(&inst->matrix, &obj->m, &inst->matrix);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004ca04-0x8004caa8.
void UI_ThTick_big1(struct Thread *bucket)

{
	s16 uVar1;
	u32 flags;
	struct UiElement3D *obj;
	struct Instance *inst;

	// Get object from thread
	obj = bucket->object;

	// Get instance from thread
	inst = bucket->inst;

	uVar1 = obj->rot[3];
	*(int *)&inst->matrix.m[0][0] = uVar1;
	*(int *)&inst->matrix.m[0][2] = 0;
	*(int *)&inst->matrix.m[1][1] = uVar1;
	*(int *)&inst->matrix.m[2][0] = 0;
	inst->matrix.m[2][2] = uVar1;

	MatrixRotate(&inst->matrix, &obj->m, &inst->matrix);

	// if hud is enabled, and this is not demo mode
	if ((*(int *)&sdata->gGT->bool_DrawOTag_InProgress & 0xff0100) == 0x100)
	{
		// make visible
		flags = inst->flags & 0xffffff7f;
	}

	else
	{
		// make invisible
		flags = inst->flags | 0x80;
	}

	inst->flags = flags;

	return;
}
