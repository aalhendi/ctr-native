#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012568-0x80012598.
int BOTS_Adv_NumTimesLostEvent(int numLost)
{
	// if you lost more than 10 times
	// the difficulty will not get lower.
	if ((u16)numLost > 10)
	{
		// the array apparently has 12, not sure why it stopped at 11.
		numLost = 10;
	}

	return data.advDifficulty[numLost];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800123e0-0x80012440
void BOTS_SetGlobalNavData(u16 index)
{
	sdata->lastPathIndex = index;

	sdata->nav_NumPointsOnPath = sdata->NavPath_ptrHeader[index]->numPoints;

	sdata->nav_ptrFirstPoint = sdata->NavPath_ptrNavFrameArray[index];

	sdata->nav_ptrLastPoint = &sdata->nav_ptrFirstPoint[sdata->nav_NumPointsOnPath];

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012440-0x80012560
void BOTS_InitNavPath(struct GameTracker *gGT, s16 index)
{
	struct NavHeader *nh = 0;
	struct NavHeader **LevNavTable = sdata->gGT->level1->LevNavTable;

	if (LevNavTable != 0)
	{
		// nullptr on Nitro Court
		nh = LevNavTable[index];
	}

	// if path exists
	if (nh != 0)
	{
		// grab the data
		sdata->NavPath_ptrHeader[index] = nh;

		sdata->NavPath_ptrNavFrameArray[index] = NAVHEADER_GETFRAME(sdata->NavPath_ptrHeader[index]);

		// if data is outdated
		if (sdata->NavPath_ptrHeader[index]->magicNumber != -0x1303)
		{
			// never mind then
			sdata->NavPath_ptrHeader[index]->numPoints = 0;
		}
	}

	// if no path data is found
	else
	{
		// use a template, which cancels AIs
		sdata->NavPath_ptrHeader[index] = &sdata->blank_NavHeader;

		sdata->NavPath_ptrNavFrameArray[index] = NAVHEADER_GETFRAME(&sdata->blank_NavHeader);

		sdata->NavPath_ptrHeader[index]->numPoints = 0;
	}

	// save number of points
	sdata->nav_NumPointsOnPath = sdata->NavPath_ptrHeader[index]->numPoints;

	// global last point
	sdata->nav_ptrLastPoint = &sdata->NavPath_ptrNavFrameArray[index][sdata->nav_NumPointsOnPath];

	// header last point
	sdata->NavPath_ptrHeader[index]->last = sdata->nav_ptrLastPoint;

	// global first point
	sdata->nav_ptrFirstPoint = sdata->NavPath_ptrNavFrameArray[index];

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012560-0x80012568.
void BOTS_EmptyFunc(void)
{
}

static void BOTS_Adv_LerpDifficulty(s16 *dst, s16 factor)
{
	s16 *lo = sdata->difficultyParams[1];
	s16 *hi = sdata->difficultyParams[0];

	for (s32 i = 0; i < 14; i++)
	{
		dst[i] = lo[i] + (s16)((factor * (hi[i] - lo[i])) / 0xf0);
	}
}

static void BOTS_Adv_CopySpawnOrder(s32 first, s32 second)
{
	*(s32 *)&sdata->kartSpawnOrderArray[0] = first;
	*(s32 *)&sdata->kartSpawnOrderArray[4] = second;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012598-0x80013374.
void BOTS_Adv_AdjustDifficulty(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode1 = gGT->gameMode1;
	u32 gameMode2 = gGT->gameMode2;
	s32 currDifficulty;
	s16 cupDifficulty = 0;

	// NOTE(aalhendi): Retail stores params1 in slot 1 and params2 in slot 0.
	if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		sdata->difficultyParams[1] = data.BossDifficulty[gGT->bossID].params1;
		sdata->difficultyParams[0] = data.BossDifficulty[gGT->bossID].params2;
	}
	else
	{
		sdata->difficultyParams[1] = data.ArcadeDifficulty[gGT->levelID].params1;
		sdata->difficultyParams[0] = data.ArcadeDifficulty[gGT->levelID].params2;
	}

	if ((gameMode1 & ARCADE_MODE) != 0)
	{
		currDifficulty = (u16)gGT->arcadeDifficulty;

		if ((gameMode2 & CHEAT_SUPERHARD) != 0)
		{
			currDifficulty = 0x140;
		}

		cupDifficulty = (s16)((u16)gGT->arcadeDifficulty + 0x50);
	}
	else if ((gameMode1 & ADVENTURE_CUP) != 0)
	{
		s32 track = gGT->cup.trackIndex;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]);
		s32 maxDifficulty = track * 5;

		if (gGT->cup.cupID == 4)
		{
			lostModifier -= 0xe1;

			if ((gameMode2 & CHEAT_ADV) != 0)
			{
				maxDifficulty = track * 7;
				lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]) - 0x141;
			}
		}
		else
		{
			lostModifier -= 0xcd;

			if ((gameMode2 & CHEAT_ADV) != 0)
			{
				maxDifficulty = track * 7;
				lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]) - 300;
			}
		}

		currDifficulty = maxDifficulty - lostModifier;
		cupDifficulty = (s16)(currDifficulty + 0x50);
	}
	else if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		s32 bossID = gGT->bossID;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostBossRace[bossID]) - 0xe1;
		s32 maxDifficulty = bossID * 5;

		if ((gameMode2 & CHEAT_ADV) != 0)
		{
			maxDifficulty = bossID * 7;
			lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostBossRace[bossID]) - 0x141;
		}

		currDifficulty = maxDifficulty - lostModifier;
	}
	else
	{
		s16 numTrophies = (s16)gGT->currAdvProfile.numTrophies + 1;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostRacePerLev[gGT->levelID]) - 0x3c;
		s32 maxDifficulty = numTrophies * 0x23;

		if (maxDifficulty < 0)
		{
			maxDifficulty += 3;
		}

		maxDifficulty >>= 2;

		if ((gameMode2 & CHEAT_ADV) != 0)
		{
			maxDifficulty = numTrophies * 0xc;
			lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostRacePerLev[gGT->levelID]) - 100;
		}

		currDifficulty = maxDifficulty - lostModifier;
	}

	if ((s16)currDifficulty < 0)
	{
		currDifficulty = 0;
	}

	BOTS_Adv_LerpDifficulty(sdata->arcade_difficultyParams, (s16)currDifficulty);

	if (((gameMode1 & ADVENTURE_CUP) != 0) || ((gameMode2 & CUP_ANY_KIND) != 0))
	{
		BOTS_Adv_LerpDifficulty(sdata->cup_difficultyParams, cupDifficulty);
	}

	sdata->unk_counter_upTo450 = 0;

	if ((sdata->const_0x30215400 == 0) && (sdata->const_0x493583fe == 0))
	{
		sdata->const_0x30215400 = 0x30215400;
		sdata->const_0x493583fe = 0x493583fe;
	}

	for (s16 i = 0; i < 3; i++)
	{
		LIST_Clear(&sdata->navBotList[i]);

		if ((gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) == 0)
		{
			BOTS_InitNavPath(gGT, i);
		}
	}

	BOTS_SetGlobalNavData(0);

	gGT->numBotsNextGame = 0;

	if (((gameMode2 & CUP_ANY_KIND) == 0) || (gGT->cup.trackIndex == 0))
	{
		if (gGT->numPlyrCurrGame == 2)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.VS_2P_1, data.kartSpawnOrder.VS_2P_2);
		}
		else if ((u8)gGT->numPlyrCurrGame > 2)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.VS_3P_4P_1, data.kartSpawnOrder.VS_3P_4P_2);
		}

		if ((gameMode1 & (RELIC_RACE | TIME_TRIAL)) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.time_trial_1, data.kartSpawnOrder.time_trial_2);
		}
		else if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.crystal_challenge_1, data.kartSpawnOrder.crystal_challenge_2);
		}
		else if ((gameMode1 & ADVENTURE_BOSS) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.boss_challenge_1, data.kartSpawnOrder.boss_challenge_2);
		}
		else if (((gameMode1 & ADVENTURE_CUP) != 0) && (gGT->cup.cupID == 4))
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.purple_cup_1, data.kartSpawnOrder.purple_cup_2);
		}
		else if ((gameMode1 & ADVENTURE_MODE) == 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.arcade_1, data.kartSpawnOrder.arcade_2);
		}
	}

	u8 pathOrder[8];
	pathOrder[0] = 0;
	pathOrder[4] = 0;
	pathOrder[3] = 2;
	pathOrder[7] = 2;

	u8 firstPath = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 1;
	pathOrder[1] = firstPath;
	pathOrder[5] = firstPath ^ 1;

	u8 secondPath = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 1;
	pathOrder[2] = secondPath + 1;
	pathOrder[6] = (secondPath ^ 1) + 1;

	for (s16 i = 0; i < 8; i++)
	{
		sdata->driver_pathIndexIDs[i] = pathOrder[(u8)sdata->kartSpawnOrderArray[i]];
	}

	if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		sdata->driver_pathIndexIDs[0] = 0;
		sdata->driver_pathIndexIDs[1] = 1;
	}

	if ((gameMode1 & BATTLE_MODE) != 0)
	{
		for (s16 i = 0; i < 4; i++)
		{
			sdata->driver_pathIndexIDs[i] = (RngDeadCoed((u32 *)&sdata->const_0x30215400) & 0xfff) / 0x555;
		}
	}

	if ((((gameMode1 & ADVENTURE_CUP) == 0) && ((gameMode2 & CUP_ANY_KIND) == 0)) || (gGT->cup.trackIndex == 0))
	{
		u8 accelOrder[8];
		u32 accel = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 3;
		u32 rearAccel = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 3;

		for (s16 i = 0; i < 4; i++)
		{
			accelOrder[i] = accel;
			accel = (accel + 1) & 3;

			accelOrder[i + 4] = rearAccel + 4;
			rearAccel = (rearAccel - 1) & 3;
		}

		for (s16 i = 0; i < 8; i++)
		{
			sdata->accelerateOrder[i] = accelOrder[(u8)sdata->kartSpawnOrderArray[i]];
		}
	}

	if ((((gameMode1 & ADVENTURE_CUP) != 0) || ((gameMode2 & CUP_ANY_KIND) != 0)) && (gGT->cup.trackIndex > 0))
	{
		s16 bestPoints = -1;
		s16 bestDriverIndex = 0;
		s16 topAccelIndex = 0;

		for (s16 i = 0; i < 8; i++)
		{
			if (((u8)gGT->numPlyrCurrGame <= i) && (bestPoints < gGT->cup.points[i]))
			{
				bestPoints = (s16)gGT->cup.points[i];
				bestDriverIndex = i;
			}

			if (sdata->accelerateOrder[i] == 0)
			{
				topAccelIndex = i;
			}
		}

		char topAccel = sdata->accelerateOrder[topAccelIndex];
		sdata->accelerateOrder[topAccelIndex] = sdata->accelerateOrder[bestDriverIndex];
		sdata->accelerateOrder[bestDriverIndex] = topAccel;
	}
}

#define MAX_KARTS 8

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013374-0x80013444.
void BOTS_UpdateGlobals(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if (gGT->numBotsNextGame != 0)
	{
		EngineSound_NearestAIs();
	}

	sdata->bestHumanRank = NULL;
	sdata->bestRobotRank = NULL;
	struct Driver *worstRobotDriver = NULL;

	for (int i = MAX_KARTS - 1; i >= 0; i--)
	{
		struct Driver *d = gGT->driversInRaceOrder[i];

		if (d == NULL)
			continue;

		if ((d->actionsFlagSet & 0x100000) != 0)
		{
			if (sdata->bestRobotRank == 0)
				worstRobotDriver = d;

			sdata->bestRobotRank = d;
		}
		else
		{
			sdata->bestHumanRank = d;
		}
	}

	if (sdata->bestHumanRank == NULL)
	{
		sdata->bestHumanRank = worstRobotDriver;
	}

	sdata->unk_counter_upTo450++;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013444-0x800135d8
void BOTS_SetRotation(struct Driver *bot, int param_2)
{
	struct NavFrame *nf = bot->botData.botNavFrame;

	CTR_SET_VEC3(bot->botData.unk5bc.ai_velAxis, 0, 0, 0);

	// ======== Get Driver Position =============

	bot->botData.estimatePos[0] = (s16)CTR_MipsSra(bot->posCurr.x, 8);
	bot->botData.estimatePos[1] = (s16)CTR_MipsSra(bot->posCurr.y, 8);
	bot->botData.estimatePos[2] = (s16)CTR_MipsSra(bot->posCurr.z, 8);

	// ======== Compare to Nav Position =============

	int dx = CTR_MipsSubLo(nf->pos[0], bot->botData.estimatePos[0]);
	int dy = CTR_MipsSubLo(nf->pos[1], bot->botData.estimatePos[1]);
	int dz = CTR_MipsSubLo(nf->pos[2], bot->botData.estimatePos[2]);

	// ======== Calculate Distance =============

	// xz dist from driver to nav
	int xzDist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dz, dz)));
	bot->botData.distToNextNavXZ = xzDist;
	// xyz distance from driver to nav
	int xyzDist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dy, dy)), CTR_MipsMulLo(dz, dz)));
	bot->botData.distToNextNavXYZ = xyzDist;

	// ======== Calculate Rotation =============

	int rot = ratan2(CTR_MipsSll(dy, 0xc), CTR_MipsSll(bot->botData.distToNextNavXZ, 0xc));
	bot->botData.estimateRotCurrY = (u8)CTR_MipsSra(rot, 4);
	bot->botData.unk5a8 = 0;

	// "if BOTS_ThTick_Drive or BOTS_Driver_Convert"
	if (param_2 == 0)
	{
		bot->botData.estimateRotNav[0] = nf->rot[0];
		rot = ratan2(CTR_MipsNegLo(dx), CTR_MipsNegLo(dz));
		bot->botData.estimateRotNav[1] = (u8)CTR_MipsSra(CTR_MipsAddLo(rot, 0x800), 4);
		bot->botData.estimateRotNav[2] = nf->rot[1];
	}
	else
	{
		bot->botData.estimateRotNav[1] = (u8)CTR_MipsSra(CTR_MipsAddLo(sdata->gGT->level1->DriverSpawn[0].rot[1], 0x400), 4);
	}

	s16 v = (s16)CTR_MipsSll(bot->botData.estimateRotNav[1], 4);

	// Keep every AI-facing yaw cache in sync with the nav estimate.
	bot->botData.ai_rotY_608 = v;
	bot->angle = v;
	bot->rotCurr.y = v;
	bot->rotPrev.y = v;
	bot->botData.ai_rot4[1] = v;

	bot->botData.botFlags |= 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800135d8-0x8001372c.
void BOTS_LevInstColl(struct Thread *param_1)
{
	s16 currPos[6];
	s16 prevPos[3];
	struct Driver *d = (struct Driver *)param_1->object;
	struct ScratchpadStruct *sps = (struct ScratchpadStruct *)0x1f800108;

	// scratchpad stuff
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.searchFlags = 1;
	sps->Input1.modelID = DYNAMIC_ROBOT_CAR;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Input1.hitRadius = 0x19;

	// grab driver stuff
	currPos[0] = (s16)CTR_MipsSra(d->posCurr.x, 8);
	currPos[1] = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.y, 8), 0x19);
	currPos[2] = (s16)CTR_MipsSra(d->posCurr.z, 8);
	prevPos[0] = (s16)CTR_MipsSra(d->posPrev.x, 8);
	prevPos[1] = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posPrev.y, 8), 0x19);
	prevPos[2] = (s16)CTR_MipsSra(d->posPrev.z, 8);

	COLL_FIXED_BotsSearch(currPos, prevPos, (s16 *)sps);

	if (sps->boolDidTouchHitbox)
	{
		sps->Union.QuadBlockColl.searchFlags &= 0xfff7;

		if ((sps->bspHitbox->flag & 0x80) != 0)
		{
			struct InstDef *instDef = sps->bspHitbox->data.hitbox.instDef;
			struct Instance *inst = instDef->ptrInstance;
			if (inst != NULL)
			{
				struct MetaDataMODEL *mdm = COLL_LevModelMeta(instDef->modelID);
				if (mdm != NULL)
				{
					if (mdm->LInC != NULL)
					{
						mdm->LInC(inst, param_1, sps);
					}
				}
			}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001372c-0x80013838.
void BOTS_ThTick_RevEngine(struct Thread *botThread)
{
	struct Driver *botDriver = (struct Driver *)botThread->object;
	struct MaskHeadWeapon *mask = botDriver->botData.maskObj;

	if (botDriver->botData.ai_posBackup[1] < botDriver->posCurr.y)
	{ // mask grabbed
		botDriver->posCurr.y = CTR_MipsSubLo(botDriver->posCurr.y, CTR_MipsSra(CTR_MipsSll(sdata->gGT->elapsedTimeMS, 9), 5));

		if (mask != NULL)
		{
			mask->pos[0] = (s16)CTR_MipsSra(botDriver->posCurr.x, 8);
			mask->pos[1] = (s16)CTR_MipsSra(botDriver->posCurr.y, 8);
			mask->pos[2] = (s16)CTR_MipsSra(botDriver->posCurr.z, 8);
		}

		VehPhysForce_TranslateMatrix(botThread, botDriver);
		VehFrameProc_Driving(botThread, botDriver);
		VehEmitter_DriverMain(botThread, botDriver);
	}
	else
	{ // not a mask grab
		if (mask != NULL)
		{
			mask->scale = 0x1000;
			mask->duration = 0;
			mask->rot[2] &= 0xfffe;
		}

		botDriver->botData.maskObj = NULL;
		botDriver->kartState = KS_ENGINE_REVVING;
		botDriver->clockReceive = 0;
		botDriver->squishTimer = 0;

		ThTick_SetAndExec(botThread, BOTS_ThTick_Drive);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013838-0x80013a70.
void BOTS_MaskGrab(struct Thread *botThread)
{
	int midpoint;
	struct NavFrame *frame;
	struct NavFrame *nextframe;
	struct Driver *bot;
	struct MaskHeadWeapon *mask;


	bot = botThread->object;          // get object from thread
	frame = bot->botData.botNavFrame; // pointer to nav point
	nextframe = frame + 1;            // pointer to next nav point after this

	// if the next nav point is a farther address than last point
	if (sdata->NavPath_ptrHeader[bot->botData.botPath]->last <= nextframe)
	{
		// set next nav point to first nav point
		nextframe = sdata->NavPath_ptrNavFrameArray[bot->botData.botPath];
	}

	bot->kartState = KS_MASK_GRABBED;

	bot->botData.unk5a8 = CTR_MipsSll(CTR_MipsDiv(frame->unk[1], 2), 8);

	// midpointX between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos[0], CTR_MipsDiv(CTR_MipsSubLo(nextframe->pos[0], frame->pos[0]), 2)), 8);
	bot->botData.ai_posBackup[0] = midpoint;
	bot->posPrev.x = midpoint;

	// midpointY between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos[1], CTR_MipsDiv(CTR_MipsSubLo(nextframe->pos[1], frame->pos[1]), 2)), 8);
	bot->botData.ai_posBackup[1] = midpoint;
	bot->posPrev.y = midpoint;
	bot->quadBlockHeight = midpoint;

	// midpointZ between nav frames
	midpoint = CTR_MipsSll(CTR_MipsAddLo(frame->pos[2], CTR_MipsDiv(CTR_MipsSubLo(nextframe->pos[2], frame->pos[2]), 2)), 8);
	bot->botData.ai_posBackup[2] = midpoint;
	bot->posPrev.z = midpoint;

	bot->botData.unk5bc.ai_mulDrift = 0;
	bot->botData.unk5bc.ai_squishCooldown = 0;
	bot->botData.unk5bc.unk5cc = 0;
	bot->botData.unk5bc.ai_speedY = 0;
	bot->botData.unk5bc.ai_speedLinear = 0;
	CTR_SET_VEC3(bot->botData.unk5bc.ai_velAxis, 0, 0, 0);

	// turn on 1st flag of actions flag set (means racer is on the ground)
	bot->actionsFlagSet |= 1;

	bot->botData.botFlags &= 0xffffffb0;

	bot->rotCurr.x = (s16)CTR_MipsSll(frame->rot[0], 4);
	bot->rotCurr.y = (s16)CTR_MipsSll(frame->rot[1], 4);
	bot->rotCurr.z = (s16)CTR_MipsSll(frame->rot[2], 4);

	bot->turbo_MeterRoomLeft = 0;
	bot->reserves = 0;
	bot->clockReceive = 0;
	bot->squishTimer = 0;
	bot->turbo_outsideTimer = 0;
	bot->matrixArray = 0;
	bot->matrixIndex = 0;

	// Clear bot ghost and airborne action bits.
	bot->actionsFlagSet &= 0xfff7ffbf;

	// if driver is not ghost
	if (botThread->modelIndex != DYNAMIC_GHOST)
	{
		// enable collision for this thread
		botThread->flags &= 0xffffefff;
	}

	// posY, plus height to be dropped from
	bot->posCurr.x = bot->botData.ai_posBackup[0];
	bot->posCurr.y = CTR_MipsAddLo(bot->botData.ai_posBackup[1], 0x10000);
	bot->posCurr.z = bot->botData.ai_posBackup[2];

	mask = VehPickupItem_MaskUseWeapon(bot, 1);
	bot->botData.maskObj = mask;

	if (mask != 0)
	{
		mask->duration = 0x1e00;
		mask->rot[2] |= 1;
	}

	// execute, then assign per-frame to BOTS_ThTick_RevEngine
	ThTick_SetAndExec(botThread, BOTS_ThTick_RevEngine);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013a70-0x80013c18.
void BOTS_Killplane(struct Thread *botThread)
{
	s16 i;
	char boolOverride;
	u8 currNav;
	u8 backCount;
	u8 override;
	struct NavFrame *frame;
	struct Driver *bot;

	// get object from thread
	bot = botThread->object;

	boolOverride = false;

	// check for Tiny Arena
	if (strcmp(sdata->gGT->levelName, rdata.s_asphalt2_thisAppearsTwice) == 0)
	{
		// edge-case override?
		switch (bot->unknown_lap_related[1])
		{
		case 0x94:
			override = 0x84;
			break;
		case 0xa0:
			override = 0x80;
			break;
		default:
			override = 0xff;
		}

		if (override != 0xff)
		{
			// pointer to nav point
			frame = bot->botData.botNavFrame;

			// goBackCount
			backCount = frame->goBackCount;
			boolOverride = (backCount < (override - 1));

			while ((boolOverride || (override + 1 < backCount)))
			{
				// nav path index
				i = bot->botData.botPath;

				// go back to previous point
				frame -= 1;

				// if this is less than address of first nav point
				if (frame < sdata->NavPath_ptrNavFrameArray[i])
				{
					// go to last nav point
					frame = &sdata->NavPath_ptrHeader[i]->last[-1];
				}

				backCount = frame->goBackCount;
				boolOverride = (backCount < (override - 1));
			}
			bot->botData.botNavFrame = frame;
			boolOverride = true;
		}
	}

	// if not Tiny Arena, or goBackCount didn't happen
	if (!boolOverride)
	{
		// pointer to navFrame
		frame = bot->botData.botNavFrame;

		// current nav point (player turned AI)
		currNav = bot->unknown_lap_related[1];

		// goBackCount
		backCount = frame->goBackCount;

		while ((backCount == currNav || ((frame->flags & 0x4000) != 0)))
		{
			// nav path index
			i = bot->botData.botPath;

			// go back one navFrame
			frame -= 1;

			// if you go back to far
			if (frame < sdata->NavPath_ptrNavFrameArray[i])
			{
				// loop back to last navFrame
				frame = &sdata->NavPath_ptrHeader[i]->last[-1];
			}
			backCount = frame->goBackCount;
			currNav = bot->unknown_lap_related[1];
		}
		// save ptr to nav frame
		bot->botData.botNavFrame = frame;
	}

	BOTS_MaskGrab(botThread);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80013c18-0x80016b00.

void BOTS_ThTick_Drive(struct Thread *botThread)
{
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	struct Driver *botDriver = (struct Driver *)botThread->object;     // iVar17
	struct Instance *botInstance = (struct Instance *)botThread->inst; // iVar22
	struct GameTracker *gGT = sdata->gGT;

	// local_34 = gGT->elapsedTimeMS;

#if 0
	if(botDriver->driverID != 0)
		return;
#endif

	botDriver->turbo_MeterRoomLeft = 0;
	botDriver->forwardDir = 0;

	botInstance->flags &= 0xffff9fff;

	if (botDriver->botData.weaponCooldown != 0)
	{
		botDriver->botData.weaponCooldown = (s16)CTR_MipsSubLo((u16)botDriver->botData.weaponCooldown, 1);
	}

	if (botDriver->ChangeState_param2 == 0)
	{
		if (((botDriver->actionsFlagSet & 0x2000000) == 0) && (botDriver->botData.weaponCooldown != 0))
		{
			botDriver->botData.weaponCooldown = (s16)CTR_MipsSubLo((u16)botDriver->botData.weaponCooldown, 1);
		}
	}
	else
	{
		BOTS_ChangeState(botDriver, botDriver->ChangeState_param2, (struct Driver *)botDriver->ChangeState_param3, botDriver->ChangeState_param4);
	}

	int elapsedMilliseconds = gGT->elapsedTimeMS; // local_34

	botDriver->reserves = (s16)CTR_MipsSubLo((u16)botDriver->reserves, elapsedMilliseconds);
	if (botDriver->reserves < 0)
		botDriver->reserves = 0;

	botDriver->turbo_outsideTimer = (s16)CTR_MipsSubLo((u16)botDriver->turbo_outsideTimer, elapsedMilliseconds);
	if (botDriver->turbo_outsideTimer < 0)
		botDriver->turbo_outsideTimer = 0;

	botDriver->squishTimer = (s16)CTR_MipsSubLo((u16)botDriver->squishTimer, elapsedMilliseconds);
	if (botDriver->squishTimer < 0)
		botDriver->squishTimer = 0;

	botDriver->burnTimer = (s16)CTR_MipsSubLo((u16)botDriver->burnTimer, elapsedMilliseconds);
	if (botDriver->burnTimer < 0)
		botDriver->burnTimer = 0;

	botDriver->clockReceive = (s16)CTR_MipsSubLo((u16)botDriver->clockReceive, elapsedMilliseconds);
	if (botDriver->clockReceive < 0)
		botDriver->clockReceive = 0;

	botDriver->botData.unk5bc.drift_unk1 = 0;
	botDriver->kartState = KS_NORMAL;

	botDriver->actionsFlagSetPrevFrame = botDriver->actionsFlagSet;
	botDriver->actionsFlagSet &= ~(0x80000402);

	int speedApprox = botDriver->speedApprox; // iVar4

	if (speedApprox < 0)
		speedApprox = CTR_MipsNegLo(speedApprox);

	if ((botDriver->actionsFlagSet & 1) == 0)
	{
		speedApprox = CTR_MipsAddLo(speedApprox, 0xf00);
	}
	else
	{
		speedApprox = CTR_MipsSra(CTR_MipsSll(speedApprox, 1), 1);
	}

	// NOTE(aalhendi): Retail initializes this boost scale before catch-up can overwrite it.
	u32 local_38 = 1;
	s16 unkSpeedThing =
	    (s16)CTR_MipsSra(CTR_MipsSll(CTR_MipsAddLo(CTR_MipsMulLo(speedApprox, 0x89), CTR_MipsMulLo(botDriver->unkSpeedValue2, 0x177)), 3), 0xC); // sVar7
	botDriver->unkSpeedValue2 = unkSpeedThing;

	if ((botDriver->actionsFlagSetPrevFrame & 8) == 0)
	{
		int baseSpeed = botDriver->baseSpeed;
		if (baseSpeed < 0)
			baseSpeed = CTR_MipsNegLo(baseSpeed);

		if (baseSpeed < 0x201)
		{
			baseSpeed = botDriver->speedApprox;
			if (baseSpeed < 0)
				baseSpeed = CTR_MipsNegLo(baseSpeed);

			if (baseSpeed < 0x201)
				goto give_this_label_a_better_name;
		}

		botDriver->unkSpeedValue1 = (s16)CTR_MipsSubLo((u16)botDriver->unkSpeedValue1, unkSpeedThing);
	}
give_this_label_a_better_name:

	if ((botDriver->unkSpeedValue1 < 1) && ((botDriver->tireColor & 1) == 0))
	{
		botDriver->unkSpeedValue1 = 0x1e00;
		botDriver->tireColor = 0x2e606061;
	}
	else
	{
		botDriver->tireColor = 0x2e808080;
	}

	struct NavFrame *navFrameCurr = (struct NavFrame *)&botDriver->botData.estimatePos[0]; // psVar19
	struct NavFrame *navFrameNext;                                                         // psVar21

	if ((botDriver->botData.botFlags & 1) == 0)
	{
		navFrameCurr = botDriver->botData.botNavFrame;
		navFrameNext = navFrameCurr + 1;

		int pathID = botDriver->botData.botPath;

		if (navFrameNext >= sdata->NavPath_ptrHeader[pathID]->last)
			navFrameNext = sdata->NavPath_ptrNavFrameArray[pathID];
	}
	else
	{
		navFrameNext = botDriver->botData.botNavFrame;
	}

	struct DriverCollisionSearch cpwb_param_2;
	cpwb_param_2.bucket.pos[0] = (short)CTR_MipsSra(botDriver->posCurr.x, 8);
	cpwb_param_2.bucket.pos[1] = (short)CTR_MipsSra(botDriver->posCurr.y, 8);
	cpwb_param_2.bucket.pos[2] = (short)CTR_MipsSra(botDriver->posCurr.z, 8);
	cpwb_param_2.bucket.th = NULL;
	cpwb_param_2.bucket.radius = 0x7fffffff;

	struct Thread *uVar12;

	if ((botThread->flags & 0x1800) == 0)
	{
		if (botThread->modelIndex == DYNAMIC_PLAYER)
		{
			PROC_CollidePointWithBucket(botThread->siblingThread, &cpwb_param_2.bucket);

			uVar12 = gGT->threadBuckets[ROBOT].thread;
		}
		else
		{
			if (botThread->modelIndex != DYNAMIC_ROBOT_CAR)
				goto give_this_label_a_better_name2;

			uVar12 = botThread->siblingThread;
		}

		PROC_CollidePointWithBucket(uVar12, &cpwb_param_2.bucket);
	}
give_this_label_a_better_name2:

	struct Thread *t = cpwb_param_2.bucket.th;
	if (t != NULL)
	{
		int iVar4 = CTR_MipsAddLo(botThread->driver_HitRadius, t->driver_HitRadius);
		if (cpwb_param_2.bucket.radius < CTR_MipsMulLo(iVar4, iVar4))
		{
			Vec3 xyz;
			xyz.x = CTR_MipsAddLo(botDriver->xSpeed, botDriver->botData.unk5bc.ai_accelAxis[0]);
			xyz.y = CTR_MipsAddLo(botDriver->ySpeed, botDriver->botData.unk5bc.ai_accelAxis[1]);
			xyz.z = CTR_MipsAddLo(botDriver->zSpeed, botDriver->botData.unk5bc.ai_accelAxis[2]);
			VehPhysCrash_AnyTwoCars(botThread, &cpwb_param_2, &xyz);
		}
	}

	// puVar5 = gGT

	int deltaPosThisFrame; // iVar4

	if (botDriver->botData.ai_progress_cooldown == 0)
	{
		int trafficLightsTimer = gGT->trafficLightsTimer;

		if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE) // check is oxide
		{
			// Pretend there is less time (oxide is a cheater)
			trafficLightsTimer = CTR_MipsSubLo(trafficLightsTimer, 0x1e0);
		}

		if (0 < trafficLightsTimer)
			goto give_this_label_a_better_name3; // if race not started, then skip

		if ((gGT->boolDemoMode != 0) && ((botDriver->botData.botFlags & 0x100) == 0) && (botThread->modelIndex == DYNAMIC_PLAYER))
		{
			botDriver->botData.botFlags |= 0x100;
			CAM_EndOfRace(&gGT->cameraDC[botDriver->driverID], botDriver);
		}

		if ((botDriver->botData.botFlags & 0x200) == 0)
		{
			// first frame of race
			botDriver->botData.botFlags |= 0x200;

			if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
			{ // if oxide, then talk
				Voiceline_RequestPlay(0, 0xf, 0x10);
			}

			if (( // if in front row & 25% chance
			        (sdata->kartSpawnOrderArray[botDriver->driverID] < 3) && ((RngDeadCoed((u_int *)&sdata->const_0x30215400) & 0xFF) < 0x40)) ||
			    (data.characterIDs[botDriver->driverID] == NITROS_OXIDE))
			{ // start the race with a boost
				VehFire_Increment(botDriver, 0x2d0, 1, 0x180);

				VehFire_Audio(botDriver, 0x180);
			}
		}

		// between these start and end tags, there's confusing variable lifetime that needs to be looked at closer.

		unsigned int botFlags = botDriver->botData.botFlags; // uVar8

		// if path change is requested, but not started (BOSS ONLY)
		if ((botFlags & 0xc0) == 0x40)
		{
			s16 changeOpcode = navFrameNext->pathChangeOpcode;
			s16 newPathID = (s16)CTR_MipsSra(changeOpcode, 0xa);
			s16 newFrameIndex = changeOpcode & 0x3ff;
			s16 cap = (s16)CTR_MipsSll(3, 0xA); // 0xC00, cant have path[3]

			if ((changeOpcode < cap) && (newPathID == botDriver->botData.desiredPath_BossOnly))
			{
				// record that change is requested (boss only)
				botDriver->botData.botFlags = botFlags | 0x80;

				short oldPathID = botDriver->botData.botPath;
				botDriver->botData.botPath = newPathID;

				LIST_RemoveMember(&sdata->navBotList[oldPathID], &botDriver->botData.item);
				LIST_AddFront(&sdata->navBotList[newPathID], &botDriver->botData.item);

				struct NavFrame *firstNavFrameOnPath = sdata->NavPath_ptrNavFrameArray[newPathID];
				botDriver->botData.botNavFrame = &firstNavFrameOnPath[newFrameIndex];

				BOTS_SetRotation(botDriver, 0);

				navFrameNext = botDriver->botData.botNavFrame;
				navFrameCurr = (struct NavFrame *)&botDriver->botData.estimatePos[0];
			}
		}
		else
		{
			// Only after first 15 seconds of the race
			if (450 < sdata->unk_counter_upTo450)
			{
				struct Driver *otherDriver = NULL; // iVar4
				if ((botFlags & 1) == 0)
				{
					int iVar3 = 1000;
					short sVar7 = 1000;
					struct BotData *botData;

					for (botData = (struct BotData *)LIST_GetFirstItem(&sdata->navBotList[botDriver->botData.botPath]); botData != NULL;
					     botData = (struct BotData *)LIST_GetNextItem((struct Item *)botData), sVar7 = (short)iVar3)
					{
						struct Driver *driverFromBotData = (struct Driver *)((char *)botData - offsetof(struct Driver, botData));

						if (driverFromBotData == botDriver)
							continue;


						// Find the signed index difference between nav frames on this path.
						int iVar13 = (int)(botData->botNavFrame - botDriver->botData.botNavFrame);

						// if "other" botData driver is behind "this" botDriver driver,
						if (iVar13 < 0)
						{
							// assume number of points "away" is large (add track length)
							iVar13 = CTR_MipsAddLo(iVar13, sdata->NavPath_ptrHeader[botDriver->botData.botPath]->numPoints);
						}

						// find closest "other" botData driver
						// that is AHEAD of "this" botDriver driver
						if (iVar13 < iVar3)
						{
							// save number of points ahead,
							// and save pointer to other driver
							iVar3 = iVar13;
							otherDriver = driverFromBotData;
						}
					}

					// If two drivers are within 3 navframe points of each other
					if ((otherDriver != NULL) && (sVar7 < 3))
					{
						int diff = CTR_MipsSubLo(botDriver->distanceToFinish_curr, otherDriver->distanceToFinish_curr);

						// if "this" bot is closer to finish than "other" bot, do nothing,
						// because "diff" will be incremented largely and fail the <0x200 check
						if (diff < 0)
						{
							diff = CTR_MipsAddLo(diff, CTR_MipsSll(gGT->level1->ptr_restart_points->distToFinish, 3));
						}

						// if "this" bot driver is behind "other" driver, and very close to them,
						// while also being on the same nav path, then change the nav path
						if (diff < 0x200)
						{
							s16 changeOpcode = navFrameNext->pathChangeOpcode;
							s16 newPathID = (s16)CTR_MipsSra(changeOpcode, 0xa);
							s16 newFrameIndex = changeOpcode & 0x3ff;
							s16 cap = (s16)CTR_MipsSll(3, 0xA); // 0xC00, cant have path[3]

							if (changeOpcode < cap)
							{
								short oldPathID = botDriver->botData.botPath;
								botDriver->botData.botPath = newPathID;

								LIST_RemoveMember(&sdata->navBotList[oldPathID], &botDriver->botData.item);
								LIST_AddFront(&sdata->navBotList[newPathID], &botDriver->botData.item);

								struct NavFrame *firstNavFrameOnPath = sdata->NavPath_ptrNavFrameArray[newPathID];
								botDriver->botData.botNavFrame = &firstNavFrameOnPath[newFrameIndex & 0x3ff];

								BOTS_SetRotation(botDriver, 0);

								navFrameNext = botDriver->botData.botNavFrame;
								navFrameCurr = (struct NavFrame *)&botDriver->botData.estimatePos[0];
							}
						}
					}
				}
			}
		}

		// puVar5 = gGT but different?

		if ((botDriver->actionsFlagSet & 1) == 0)
		{
			int ZYsqr = CTR_MipsAddLo(CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedLinear, botDriver->botData.unk5bc.ai_speedLinear),
			                          CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedY, botDriver->botData.unk5bc.ai_speedY));

			if (0x2b110000 < ZYsqr) // sqrZY
			{
				int ZY = SquareRoot0_stub(ZYsqr);

				int zVel = CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedLinear, 0x6900); // iVar3

#if 0 // in the OG game
				if (ZY == 0)
				{
					//trap(0x1c00);
				}
				if ((ZY == -1) && (zVel == -0x80000000))
				{
					//trap(0x1800);
				}
#endif
				int yVel = CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedY, 0x6900); // iVar15
#if 0                                                                                  // in the OG game
				if (ZY == 0)
				{
					//trap(0x1c00);
				}
				if ((ZY == -1) && (yVel == -0x80000000))
				{
					//trap(0x1800);
				}
#endif
				botDriver->botData.unk5bc.ai_speedLinear = CTR_MipsDiv(zVel, ZY);
				botDriver->botData.unk5bc.ai_speedY = CTR_MipsDiv(yVel, ZY);
			}
		}
		else
		{
			botDriver->actionsFlagSet &= ~(0x80000);

			struct Driver *bestDriverRank = sdata->bestHumanRank; // iVar4

			if ((gGT->gameMode1 & RELIC_RACE) != 0)
			{
				bestDriverRank = sdata->bestRobotRank;
			}

			int botVelocity; // iVar3

			if ((bestDriverRank == NULL) || (bestDriverRank == botDriver))
			{
				botVelocity = gGT->constVal_9000;
			}
			else
			{
				int driverRank = botDriver->driverRank; // uVar8
				bool isInAdvArcadeOrVSCup = false;      // bVar1

				if (((gGT->gameMode1 & ADVENTURE_CUP) != 0) || ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
				{
					isInAdvArcadeOrVSCup = true;
				}

				if (gGT->drivers[0]->driverRank < botDriver->driverRank)
				{
					driverRank--;
				}

				if (1 < gGT->numPlyrCurrGame && gGT->drivers[1]->driverRank <= driverRank)
				{
					driverRank--;
				}

				if (isInAdvArcadeOrVSCup)
				{
					driverRank = sdata->accelerateOrder[botDriver->driverID];
					if (sdata->accelerateOrder[0] < 2)
					{
						driverRank--;
					}
					if (sdata->accelerateOrder[1] < 2)
					{
						driverRank--;
					}
				}

				short difficultyStat; // sVar7

				if (bestDriverRank->lapIndex == 0)
				{
					difficultyStat = sdata->arcade_difficultyParams[0xB];
				}
				else
				{
					if (bestDriverRank->lapIndex == gGT->numLaps - 1)
					{
						difficultyStat = sdata->arcade_difficultyParams[0xD];
					}
					else
					{
						difficultyStat = sdata->arcade_difficultyParams[0xC];
					}
				}

				int distToFinish = CTR_MipsSll(gGT->level1->ptr_restart_points->distToFinish, 3); // iVar3

				int lapIndex = bestDriverRank->lapIndex; // uVar20

				if ((bestDriverRank->actionsFlagSet & 0x1000000) != 0)
				{
					lapIndex--;
				}

				int botLapIndex = botDriver->lapIndex; // uVar18

				if ((botDriver->actionsFlagSet & 0x1000000) != 0)
				{
					botLapIndex--;
				}

				// not super meaningful, probably just a difficulty statistic
				int complexDifficultyStat = CTR_MipsSubLo(
				    CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsSubLo(distToFinish, bestDriverRank->distanceToFinish_curr), CTR_MipsMulLo(lapIndex, distToFinish)),
				                  CTR_MipsAddLo(CTR_MipsSubLo(distToFinish, botDriver->distanceToFinish_curr), CTR_MipsMulLo(botLapIndex, distToFinish))),
				    CTR_MipsAddLo(sdata->arcade_difficultyParams[driverRank], difficultyStat));

				int otherDifficultyStat; // iVar13
				if (isInAdvArcadeOrVSCup && ((driverRank & 0xffff) == 0))
				{
					if (complexDifficultyStat < 1)
					{
						otherDifficultyStat = sdata->arcade_difficultyParams[0x9];
					}
					else
					{
						if (botDriver->lapIndex < gGT->numLaps - 1)
						{
							otherDifficultyStat = sdata->cup_difficultyParams[0x8];
						}
						else
						{
							otherDifficultyStat = sdata->cup_difficultyParams[0x8] + sdata->cup_difficultyParams[0xA];
						}
					}
				}
				else
				{
					if (complexDifficultyStat < 1)
					{
						otherDifficultyStat = sdata->arcade_difficultyParams[0x9];
					}
					else
					{
						if (gGT->numLaps - 1 <= botDriver->lapIndex)
						{
							otherDifficultyStat = sdata->arcade_difficultyParams[0x8] + sdata->arcade_difficultyParams[0xA];
						}
						else
						{
							otherDifficultyStat = sdata->arcade_difficultyParams[0x8];
						}
					}
				}
				botVelocity = complexDifficultyStat; // iVar3
				if (complexDifficultyStat < 0)
					botVelocity = CTR_MipsNegLo(complexDifficultyStat);
				int iVar9 = otherDifficultyStat;
				if (otherDifficultyStat < 0)
					iVar9 = CTR_MipsNegLo(otherDifficultyStat);

				botVelocity = CTR_MipsSra(CTR_MipsMulLo(iVar9, CTR_MipsDiv(CTR_MipsSll(CTR_MipsAddLo(botVelocity, 0x80), 0xc), 0xa00)), 0xc);
				local_38 = 0 < complexDifficultyStat;
				botVelocity = CTR_MipsAddLo(botVelocity, CTR_MipsMulLo(CTR_MipsDiv(CTR_MipsSll(botVelocity, 3), 100), CTR_MipsSubLo(7, driverRank)));

				if (iVar9 < botVelocity)
					botVelocity = iVar9;
				if (otherDifficultyStat < 0)
					botVelocity = CTR_MipsNegLo(iVar9);

				int bestDriverWumpaCount = bestDriverRank->numWumpas; // iVar15, assume max of 9?
				if (9 < bestDriverWumpaCount)
					bestDriverWumpaCount = 9;

				int turboMult = bestDriverRank->turboConst; // iVar13, max of 5
				if (5 < turboMult)
					turboMult = 5;

				int netSpeedStat = CTR_MipsSubLo(
				    CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(bestDriverRank->const_AccelSpeed_ClassStat, bestDriverRank->const_Speed_ClassStat), 0xc), 5), 1);

				int additionalVelocity = CTR_MipsAddLo(
				    CTR_MipsDiv(CTR_MipsSll(bestDriverRank->const_Speed_ClassStat, 3), 10),
				    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsDiv(CTR_MipsMulLo(bestDriverWumpaCount, netSpeedStat), 10), CTR_MipsMulLo(turboMult, netSpeedStat)),
				                0xc));

				if (additionalVelocity > 0x6900)
					additionalVelocity = 0x6900;

				botVelocity = CTR_MipsAddLo(botVelocity, additionalVelocity);

				if (botVelocity < 0x5dc1)
				{
					if (botVelocity < 0x1c20)
						botVelocity = 0x1c20;
				}
				else
					botVelocity = 24000;
			}

			if (botDriver->reserves == 0)
				botDriver->botData.botFlags &= 0xffffffef;
			else
			{
				if ((botDriver->botData.botFlags & 0x10) == 0)
					botVelocity = CTR_MipsAddLo(botVelocity, botDriver->fireSpeedCap);
				else
					botVelocity = CTR_MipsAddLo(botDriver->fireSpeedCap, 10000);
			}

			if (botDriver->clockReceive != 0 || botDriver->squishTimer != 0)
				botVelocity = CTR_MipsSra(CTR_MipsMulLo(botVelocity, 0xc00), 0xc);

			if ((botDriver->botData.botFlags & 2) == 0)
			{
				if (botDriver->instTntRecv != NULL || botDriver->thCloud != NULL)
				{
					int damagedVelocityPenalty = CTR_MipsSra(botDriver->const_DamagedSpeed, 1); // iVar4

					botVelocity = CTR_MipsSubLo(botVelocity, damagedVelocityPenalty);
				}
			}
			else
			{
				botDriver->botData.unk5bc.ai_turboMeter = 0;

				if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
				{
					int damagedVelocityPenalty = CTR_MipsSra(botDriver->const_DamagedSpeed, 2); // iVar4

					botVelocity = CTR_MipsSubLo(botVelocity, damagedVelocityPenalty);
				}
				else
				{
					botVelocity = CTR_MipsSubLo(botVelocity, botDriver->const_DamagedSpeed);
				}
			}
			if (botVelocity < 0)
			{
				botVelocity = 0;
			}

			struct Terrain *botTerrain = botDriver->terrainMeta1; // iVar15

			botDriver->botData.unk5bc.ai_speedLinear =
			    CTR_MipsSubLo(botDriver->botData.unk5bc.ai_speedLinear,
			                  CTR_MipsSra(CTR_MipsMulLo(botDriver->const_PedalFriction_Forward, botTerrain->accel_impact), 8)); // iVar4

			if (botDriver->botData.unk5bc.ai_speedLinear < 0)
				botDriver->botData.unk5bc.ai_speedLinear = 0;

			if (0x6900 < botVelocity)
				botVelocity = 0x6900;

			int velocityAccountingForTerrain = CTR_MipsSra(CTR_MipsMulLo(botVelocity, botTerrain->unk_0x34[2]), 8); // iVar4

			if ((botTerrain->unk_0x34[1] & 0x80) == 0)
			{
			LAB_80014b34:
				if (botDriver->botData.unk5bc.ai_speedLinear < velocityAccountingForTerrain)
				{
				LAB_80014b48:
					short accel; // sVar7
					if (botDriver->reserves < 1)
					{
						accel = botDriver->const_Accel_ClassStat;
					}
					else
					{
						accel = botDriver->const_Accel_Reserves;
					}
					botVelocity = CTR_MipsSra(CTR_MipsMulLo(accel, botTerrain->unk_0x34[3]), 8);

					if (botDriver->botData.botAccel != 0)
					{
						botDriver->botData.botAccel--;
						botVelocity = CTR_MipsSra(CTR_MipsMulLo(botVelocity, CTR_MipsSubLo(0x100, CTR_MipsMulLo(sdata->AI_AccelFrameSteps,
						                                                                                        sdata->accelerateOrder[botDriver->driverID]))),
						                          8);
					}

					botDriver->botData.unk5bc.ai_speedLinear = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_speedLinear, botVelocity);
				}
			}
			else
			{
				botVelocity = CTR_MipsSra(botDriver->botData.unk5bc.ai_speedLinear, 1);

				if (botDriver->botData.unk5bc.ai_speedLinear < velocityAccountingForTerrain)
					goto LAB_80014b48;

				botDriver->botData.unk5bc.ai_speedLinear = botVelocity;
				if (botVelocity < velocityAccountingForTerrain)
				{
					botDriver->botData.unk5bc.ai_speedLinear = velocityAccountingForTerrain;
					goto LAB_80014b34;
				}
			}

			int levelID = gGT->levelID; // iVar3

			if ((levelID == HOT_AIR_SKYWAY || levelID == PAPU_PYRAMID) || levelID == POLAR_PASS)
			{
				int botPathIndex = botDriver->botData.botPath; // iVar3

				if (levelID == POLAR_PASS)
				{
					botPathIndex = CTR_MipsAddLo(botPathIndex, 3);
				}
				else
				{
					if (levelID == PAPU_PYRAMID)
					{
						botPathIndex = CTR_MipsAddLo(botPathIndex, 6);
					}
					else if (levelID == SLIDE_COLISEUM)
					{
						botPathIndex = CTR_MipsAddLo(botPathIndex, 9);
					}
				}

				int navFrameIndexOnPath = (int)(navFrameCurr - sdata->NavPath_ptrNavFrameArray[botDriver->botData.botPath]);

				if ((data.botsThrottle[botPathIndex] <= navFrameIndexOnPath) && (navFrameIndexOnPath < CTR_MipsAddLo(data.botsThrottle[botPathIndex], 0xb)) &&
				    (9000 < botDriver->botData.unk5bc.ai_speedLinear))
				{
					botDriver->botData.unk5bc.ai_turboMeter = 0;

					botDriver->botData.unk5bc.ai_speedLinear =
					    CTR_MipsSubLo(botDriver->botData.unk5bc.ai_speedLinear, CTR_MipsAddLo(100, botDriver->const_Accel_ClassStat));
				}
			}

			if ((unsigned char)0x80u < navFrameCurr->rot[3])
			{
				velocityAccountingForTerrain = CTR_MipsAddLo(velocityAccountingForTerrain, botDriver->unk47E);

				if (botDriver->botData.unk5bc.ai_speedLinear < velocityAccountingForTerrain)
				{
					unsigned int var = (unsigned int)navFrameCurr->rot[3];
					int sinOfAngle = MATH_Sin(CTR_MipsSll(var, 4));

					botDriver->botData.unk5bc.ai_speedLinear =
					    CTR_MipsSubLo(botDriver->botData.unk5bc.ai_speedLinear,
					                  CTR_MipsSra(CTR_MipsMulLo(botDriver->const_Gravity, sinOfAngle), 0xc)); // force on a slope due to gravity
				}

				botDriver->fireSpeed = velocityAccountingForTerrain;
			}
		}

		if (0x6400 < botDriver->botData.unk5bc.ai_speedLinear)
		{
			botDriver->botData.unk5bc.ai_speedLinear = 0x6400;
		}

		deltaPosThisFrame = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedLinear, elapsedMilliseconds), 5); // iVar4
		if (deltaPosThisFrame < 0)
		{
			deltaPosThisFrame = 0;
		}
		deltaPosThisFrame = CTR_MipsAddLo(deltaPosThisFrame, botDriver->botData.unk5a8);
	}
	else
	{
	give_this_label_a_better_name3:
		botInstance->scale[0] = 0xccc;
		botInstance->scale[1] = 0xccc;
		botInstance->scale[2] = 0xccc;

		botDriver->botData.unk5bc.ai_speedLinear = 0;

		if (botDriver->botData.ai_progress_cooldown != 0)
		{
			botDriver->botData.ai_progress_cooldown--;
		}

		deltaPosThisFrame = botDriver->botData.unk5a8;
	}

	int navFrameFlags = navFrameCurr->flags;
	int navFrameSpecialBits = navFrameCurr->specialBits;
	// local_3c == 0
	if ((navFrameSpecialBits & 0x80) != 0)
	{
		botDriver->botData.botFlags |= 0x20;
	}

	int gravity;
	if ((botDriver->botData.botFlags & 0x20) == 0)
	{
		gravity = botDriver->const_Gravity;
	}
	else
	{
		gravity = CTR_MipsDiv(CTR_MipsMulLo(botDriver->const_Gravity, 41), 100);
	}

	botDriver->botData.unk5bc.ai_speedY =
	    CTR_MipsSubLo(botDriver->botData.unk5bc.ai_speedY, CTR_MipsSra(CTR_MipsMulLo(gravity, elapsedMilliseconds), 5)); // iVar3

	if (botDriver->botData.unk5bc.ai_speedY < -0x5000)
		botDriver->botData.unk5bc.ai_speedY = -0x5000;

	botDriver->botData.ai_posBackup[1] =
	    CTR_MipsAddLo(botDriver->botData.ai_posBackup[1], CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedY, elapsedMilliseconds), 5));

	short navDist; // sVar7

	if ((botDriver->actionsFlagSet & 1) == 0)
	{
		// unk[1] may be distToNextNavXZ
		navDist = navFrameCurr->unk[1];
		// navDist = botDriver->botData.distToNextNavXZ;
	}
	else
	{
		// unk[0] may be distToNextNavXYZ
		navDist = navFrameCurr->unk[0];
		// navDist = botDriver->botData.distToNextNavXYZ;
	}

	int local_3c = 0;
	char local_30 = 0;
	if ((navFrameCurr->specialBits & 0x10) != 0)
	{
		local_3c = navFrameCurr->specialBits;
	}
	int iVar15 = CTR_MipsSra(deltaPosThisFrame, 8);
	int iVar3;

	for (iVar3 = navDist; iVar3 <= iVar15; /* decrement inside loop */)
	{
		navFrameCurr = navFrameNext;

		deltaPosThisFrame = CTR_MipsSubLo(deltaPosThisFrame, CTR_MipsSll(iVar3, 8));

		int index = botDriver->botData.botPath; // index = iVar13

		navFrameNext = NAVFRAME_GETNEXTFRAME(navFrameCurr);

		iVar15 = CTR_MipsSubLo(iVar15, iVar3);

		if (navFrameNext >= sdata->NavPath_ptrHeader[index]->last)
			navFrameNext = sdata->NavPath_ptrNavFrameArray[index];

		if ((CTR_MipsSra(botDriver->botData.ai_posBackup[1], 8) < navFrameNext->pos[1]) && ((navFrameCurr->flags & 0x200) != 0))
		{
			BOTS_Killplane(botThread);
		}

		navFrameFlags |= navFrameCurr->flags;
		navFrameSpecialBits |= navFrameCurr->specialBits;

		int uVar8;
		if ((botDriver->botData.botFlags & 0x80) == 0)
		{
			uVar8 = 0xfffffffe;
		}
		else
		{
			uVar8 = 0xffffff3e;
		}

		botDriver->botData.botFlags &= uVar8;

		short unk; // sVar7

		// MUST be unsigned,
		// this makes a range-value check, and makes negatives positive,
		// so its really checking value between 0x31 and 0x31+0x9e
		unsigned char compare = navFrameCurr->rot[3] - 0x31;
		if ((local_30 == 0) && (0x9e < compare))
		{
			if ((botDriver->actionsFlagSet & 1) == 0)
			{
				unk = navFrameCurr->unk[1];
			}
			else
			{
				unk = navFrameCurr->unk[0];
			}
		}
		else
		{
			local_30 = 1;

			unk = navFrameCurr->unk[1];
		}

		iVar3 = unk;

		if ((navFrameCurr->specialBits & 0x10) != 0)
		{
			local_3c = navFrameCurr->specialBits;
		}
	}

	botDriver->botData.unk5a8 = deltaPosThisFrame;

	int actionFlagsBuildup = CTR_MipsSll(navFrameFlags & 2, 10); // uVar20

	if ((navFrameFlags & 0x2000) != 0)
	{
		actionFlagsBuildup |= 0x10000;
	}

	if ((navFrameFlags & 4) != 0)
	{
		actionFlagsBuildup |= 0x1000;
	}

	botDriver->actionsFlagSet &= ~(0x11800);
	botDriver->actionsFlagSet |= actionFlagsBuildup;

	struct Terrain *terrain = VehAfterColl_GetTerrain(((u_char)navFrameCurr->flags) >> 3);

	botDriver->terrainMeta1 = terrain;

	if ((navFrameCurr->specialBits & 0x20) != 0)
	{
		short vertSplit;
		if ((navFrameCurr->specialBits & 0xf) == 0)
		{
			vertSplit = gGT->level1->splitLines[0];
		}
		else
		{
			vertSplit = gGT->level1->splitLines[1];
		}

		botInstance->vertSplit = vertSplit;

		botInstance->flags |= 0x4000;
	}

	if (((navFrameCurr->specialBits & 0x30) == 0) && (botThread->modelIndex != DYNAMIC_GHOST))
	{
		int transparency = CTR_MipsMulLo(navFrameCurr->specialBits & 0xf, 0x9c00);

		botDriver->alphaScaleBackup = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)botDriver->alphaScaleBackup, 100), transparency), 8);

		botInstance->alphaScale = (s16)CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)botInstance->alphaScale, 100), transparency), 8);
	}

	if (((botDriver->actionsFlagSet & 0x1000) == 0) || ((actionFlagsBuildup & 0x1800) == 0))
	{
		botDriver->turbo_MeterRoomLeft = 0;
		botDriver->botData.unk5bc.ai_turboMeter = 0;
		botDriver->botData.unk5bc.ai_fireLevel = 0;
	}
	else
	{
		botDriver->actionsFlagSet |= 0x80;
		botDriver->botData.unk5bc.ai_turboMeter = (s16)CTR_MipsAddLo((u16)botDriver->botData.unk5bc.ai_turboMeter, elapsedMilliseconds);
		int uVar6;
		// this ugly tree of ifs may have been a switch perhaps?
		if ((botDriver->botData.unk5bc.ai_turboMeter < 0xb41) || (5 < botDriver->botData.unk5bc.ai_fireLevel))
		{
			if ((botDriver->botData.unk5bc.ai_turboMeter < 0x961) || (uVar6 = 5, 4 < botDriver->botData.unk5bc.ai_fireLevel))
			{
				if ((botDriver->botData.unk5bc.ai_turboMeter < 0x781) || (3 < botDriver->botData.unk5bc.ai_fireLevel))
				{
					if ((0x5a0 < botDriver->botData.unk5bc.ai_turboMeter) && (uVar6 = 3, botDriver->botData.unk5bc.ai_fireLevel < 3))
						goto LAB_800153d0;
					if ((botDriver->botData.unk5bc.ai_turboMeter < 0x3c1) || (1 < botDriver->botData.unk5bc.ai_fireLevel))
					{
						if ((0x1e0 < botDriver->botData.unk5bc.ai_turboMeter) && (uVar6 = 1, botDriver->botData.unk5bc.ai_fireLevel < 1))
						{
							goto LAB_800153d0;
						}
					}
					else
					{
						// trigger a turbo boost?
						botDriver->botData.unk5bc.ai_fireLevel = 2;
						botDriver->turbo_MeterRoomLeft = 0;
						VehFire_Increment(botDriver, 0xf0, 2, CTR_MipsSll(local_38, 7));
					}
				}
				else
				{
					// trigger a turbo boost?
					botDriver->botData.unk5bc.ai_fireLevel = 4;
					botDriver->turbo_MeterRoomLeft = 0;
					VehFire_Increment(botDriver, 0x1e0, 2, CTR_MipsSll(local_38, 8));
				}
			}
			else
			{
			LAB_800153d0:
				// trigger a turbo boost?
				botDriver->botData.unk5bc.ai_fireLevel = uVar6;
				botDriver->turbo_MeterRoomLeft = 0xa0;
			}
		}
		else
		{
			// trigger a turbo boost?
			botDriver->botData.unk5bc.ai_fireLevel = 6;
			botDriver->turbo_MeterRoomLeft = 0;
			VehFire_Increment(botDriver, 0x2d0, 2, CTR_MipsMulLo(local_38, 0x180));
		}
	}

	if ((navFrameFlags & 0x100) != 0)
	{
		VehFire_Increment(botDriver, 0x78, 1, 0x900);

		botDriver->botData.botFlags |= 0x10;
	}

	if ((navFrameFlags & 0x1) != 0)
	{
		VehFire_Increment(botDriver, 0x2d0, 1, 0x180);

		botDriver->botData.botFlags |= 0x10;
	}

	if ((botDriver->botData.botFlags & 1) == 0)
	{
		botDriver->botData.botNavFrame = navFrameCurr;

		short botPath = botDriver->botData.botPath;

		navFrameNext = NAVFRAME_GETNEXTFRAME(navFrameCurr);

		if (navFrameNext >= sdata->NavPath_ptrHeader[botPath]->last)
			navFrameNext = sdata->NavPath_ptrNavFrameArray[botPath];
	}

	int percentage; // iVar13
	if (iVar3 == 0)
		percentage = 0;
	else
	{
		percentage = CTR_MipsDiv(CTR_MipsSll(iVar15, 0xc), iVar3);
	}

	botDriver->posPrev.x = botDriver->posCurr.x;
	botDriver->posPrev.y = botDriver->posCurr.y;
	botDriver->posPrev.z = botDriver->posCurr.z;

	botDriver->rotPrev.x = botDriver->rotCurr.x;
	botDriver->rotPrev.y = botDriver->rotCurr.y;
	botDriver->rotPrev.z = botDriver->rotCurr.z;

	botDriver->botData.ai_posBackup[0] = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos[0], CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos[0], navFrameCurr->pos[0]), percentage), 0xc)), 8);
	botDriver->quadBlockHeight = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos[1], CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos[1], navFrameCurr->pos[1]), percentage), 0xc)), 8);
	botDriver->botData.ai_posBackup[2] = CTR_MipsSll(
	    CTR_MipsAddLo(navFrameCurr->pos[2], CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSubLo(navFrameNext->pos[2], navFrameCurr->pos[2]), percentage), 0xc)), 8);

	if ((botDriver->botData.botFlags & 0x8) != 0)
	{
		botDriver->botData.unk5bc.ai_accelAxis[1] = 0;
		botDriver->botData.unk5bc.ai_velAxis[0] = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[0], botDriver->botData.unk5bc.ai_accelAxis[0]);
		botDriver->botData.unk5bc.ai_velAxis[1] = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[1], botDriver->botData.unk5bc.ai_accelAxis[1]);
		botDriver->botData.unk5bc.ai_velAxis[2] = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[2], botDriver->botData.unk5bc.ai_accelAxis[2]);
		int preAccelX = botDriver->botData.unk5bc.ai_accelAxis[0]; // iVar3
		int preAccelZ = botDriver->botData.unk5bc.ai_accelAxis[2]; // iVar15
		botDriver->botData.unk5bc.ai_accelAxis[0] = CTR_MipsSra(botDriver->botData.unk5bc.ai_accelAxis[0], 1);
		botDriver->botData.unk5bc.ai_accelAxis[2] = CTR_MipsSra(botDriver->botData.unk5bc.ai_accelAxis[2], 1);
		botDriver->botData.unk5bc.ai_velAxis[1] = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[1], botDriver->botData.unk5bc.ai_accelAxis[1]);

		botDriver->botData.unk5bc.ai_velAxis[0] = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[0], preAccelX);
		botDriver->botData.unk5bc.ai_velAxis[2] = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[2], preAccelZ);

		int preX = botDriver->botData.unk5bc.ai_velAxis[0]; // iVar3
		if (preX != 0)
		{
			if (preX < 1)
			{
				botDriver->botData.unk5bc.ai_velAxis[0] = CTR_MipsAddLo(preX, 0x444);
				if (0 < CTR_MipsAddLo(preX, 0x444))
				{
					botDriver->botData.unk5bc.ai_velAxis[0] = 0;
				}
				else
				{
					if (botDriver->botData.unk5bc.ai_accelAxis[0] == 0)
					{
						botDriver->botData.unk5bc.ai_accelAxis[0] = 0x444;
					}
				}
			}
			else
			{
				botDriver->botData.unk5bc.ai_velAxis[0] = CTR_MipsSubLo(preX, 0x444);
				if (CTR_MipsSubLo(preX, 0x444) < 0)
				{
					botDriver->botData.unk5bc.ai_velAxis[0] = 0;
				}
				else
				{
					if (botDriver->botData.unk5bc.ai_accelAxis[0] == 0)
					{
						botDriver->botData.unk5bc.ai_accelAxis[0] = -0x444;
					}
				}
			}
		}
		int preZ = botDriver->botData.unk5bc.ai_velAxis[2]; // iVar3
		if (preZ != 0)
		{
			if (preZ < 1)
			{
				botDriver->botData.unk5bc.ai_velAxis[2] = CTR_MipsAddLo(preZ, 0x444);
				if (0 < CTR_MipsAddLo(preZ, 0x444))
				{
					botDriver->botData.unk5bc.ai_velAxis[2] = 0;
				}
				else
				{
					if (botDriver->botData.unk5bc.ai_accelAxis[2] == 0)
					{
						botDriver->botData.unk5bc.ai_accelAxis[2] = 0x444;
					}
				}
			}
			else
			{
				botDriver->botData.unk5bc.ai_velAxis[2] = CTR_MipsSubLo(preZ, 0x444);
				if (CTR_MipsSubLo(preZ, 0x444) < 0)
				{
					botDriver->botData.unk5bc.ai_velAxis[2] = 0;
				}
				else
				{
					if (botDriver->botData.unk5bc.ai_accelAxis[2] == 0)
					{
						botDriver->botData.unk5bc.ai_accelAxis[2] = -0x444;
					}
				}
			}
		}
		if ((botDriver->botData.unk5bc.ai_velAxis[0] == 0) && (botDriver->botData.unk5bc.ai_velAxis[2] == 0))
		{
			botDriver->botData.botFlags &= 0xfffffff7;
		}
	}

	if ((botDriver->botData.botFlags & 0x9) == 0)
	{
		botDriver->botData.ai_quadblock_checkpointIndex = navFrameCurr->goBackCount;
	}
	else
	{
		int deltaRotY = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[1], 4), CTR_MipsSll(navFrameCurr->rot[1], 4)) & 0xfff;
		if (0x7ff < deltaRotY)
		{
			deltaRotY = CTR_MipsSubLo(deltaRotY, 0x1000);
		}

		botDriver->botData.ai_rot4[1] = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[1], 4), CTR_MipsSra(CTR_MipsMulLo(deltaRotY, percentage), 0xc)) & 0xfff;

		s16 sVar7;
		s16 top[3];
		top[0] = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.ai_posBackup[0], botDriver->botData.unk5bc.ai_velAxis[0]), 8);
		sVar7 = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.ai_posBackup[1], botDriver->botData.unk5bc.ai_velAxis[1]), 8);
		top[1] = (s16)CTR_MipsSubLo(sVar7, 0x100);
		top[2] = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.ai_posBackup[2], botDriver->botData.unk5bc.ai_velAxis[2]), 8);

		s16 bot[3];
		bot[0] = top[0];
		bot[1] = (s16)CTR_MipsAddLo(sVar7, 0x80);
		bot[2] = top[2];


		/*
		    from my understanding:
		    first param: 98 is x, 96 is y, 94 is z
		    second param: 92 is padding, 90 is x, 8e is y, 8c is z
		*/

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.qbFlagsWanted = 0x1000;
		sps->Union.QuadBlockColl.qbFlagsIgnored = 0x10;
		sps->Union.QuadBlockColl.searchFlags = 2;

		COLL_SearchBSP_CallbackQUADBLK((u_int *)&top[0], (u_int *)&bot[0], sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			botDriver->quadBlockHeight = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos[1], 8);

			botDriver->botData.ai_quadblock_checkpointIndex = sps->Set2.ptrQuadblock->checkpointIndex;

			VehPhysForce_RotAxisAngle(&botInstance->matrix, &sps->Set2.normalVec[0], botDriver->botData.ai_rot4[1]);

			botDriver->AxisAngle3_normalVec[0] = sps->Set2.normalVec[0];
			botDriver->AxisAngle3_normalVec[1] = sps->Set2.normalVec[1];
			botDriver->AxisAngle3_normalVec[2] = sps->Set2.normalVec[2];

			// this line is cringe.
			botInstance->bitCompressed_NormalVector_AndDriverIndex =
			    (((u16)sps->Set2.normalVec[0] >> 6) & 0xff) | (((u16)sps->Set2.normalVec[1] & 0x3fc0) << 2) |
			    ((((u16)sps->Set2.normalVec[2] >> 6) & 0xff) << 0x10) | CTR_MipsSll(CTR_MipsAddLo(botDriver->driverID, 1), 0x18);

			if ((sps->Set2.ptrQuadblock->quadFlags & 0x200) != 0)
			{
				BOTS_Killplane(botThread);
			}
		}
	}
	deltaPosThisFrame = CTR_MipsSra(deltaPosThisFrame, 8);
	if (botDriver->botData.ai_posBackup[1] < botDriver->quadBlockHeight)
	{
		int oldBotFlags = botDriver->botData.botFlags; // uVar8
		botDriver->botData.botFlags &= 0xffffffdf;

		if ((navFrameCurr->flags & 0x200) == 0)
		{
			if ((oldBotFlags & 2) == 0)
			{
				if ((botDriver->actionsFlagSet & 1) == 0)
				{
					if (botDriver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
					{
						int mapped = VehCalc_MapToRange(CTR_MipsSubLo(botDriver->jumpHeightCurr, botDriver->jumpHeightPrev), 0x300, 0x1400, 0x4b, 200); // uVar8

						int volume; // uVar20
						if (mapped < 0)
						{
							volume = 0;
						}
						else
						{
							volume = mapped & 0xff;

							if (0xff < mapped)
							{
								volume = 0xff;
							}
						}
						int flags;
						if ((botDriver->actionsFlagSet & 0x10000) == 0)
						{
							flags = CTR_MipsSll(volume, 0x10) | 0x8080;
						}
						else
						{
							flags = CTR_MipsSll(volume, 0x10) | 0x1008080;
						}

						OtherFX_Play_LowLevel(7, 1, flags);
					}
					int iVar3 = navFrameCurr->unk[1];
					if (iVar3 != 0)
					{
#if 0
						if (iVar3 == 0)
						{
							trap(0x1c00);
						}
						if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->unk[0] == -0x80000000))
						{
							trap(0x1800);
						}
#endif
						botDriver->botData.unk5a8 = CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->unk[0]), iVar3), 8);
					}
					short sVar7 = botDriver->jump_LandingBoost;

					if (sVar7 < 0x5a1)
					{
						if (0x3c0 < sVar7)
						{
							deltaPosThisFrame = CTR_MipsMulLo(local_38, 0x60);
							goto doFireIncrement;
						}
						else if (0x280 < sVar7)
						{
							deltaPosThisFrame = 0;
							goto doFireIncrement;
						}
					}
					else
					{
						deltaPosThisFrame = CTR_MipsMulLo(local_38, 0xc0);
					doFireIncrement:
						VehFire_Increment(botDriver, 0x2d0, 2, deltaPosThisFrame);
					}

					botDriver->actionsFlagSet |= 2;
				}
				deltaPosThisFrame = CTR_MipsSubLo(botDriver->quadBlockHeight, botDriver->posPrev.y);

				botDriver->botData.ai_posBackup[1] = botDriver->quadBlockHeight;

				botDriver->botData.unk5bc.ai_speedY = deltaPosThisFrame;

				if ((navFrameFlags & 0x400) != 0 || (botDriver->instTntRecv != NULL))
				{
					int oldActionsFlags = botDriver->actionsFlagSet;

					botDriver->actionsFlagSet |= 0x400;

					botDriver->botData.unk5bc.ai_speedY = CTR_MipsAddLo(deltaPosThisFrame, 0x1400);

					if (botThread->modelIndex == DYNAMIC_PLAYER)
					{
						OtherFX_Play_Echo(8, 1, ((u32)oldActionsFlags >> 0x10) & 1);
					}
				}
				if (16000 < botDriver->botData.unk5bc.ai_speedY)
				{
					botDriver->botData.unk5bc.ai_speedY = 16000;
				}

				if ((navFrameFlags & 0x1800) == 0)
				{
					botDriver->botData.unk5bc.drift_unk1 = 0;

					botDriver->kartState = KS_NORMAL;
				}
				else
				{
					botDriver->kartState = KS_DRIFTING;

					if ((navFrameFlags & 0x800) == 0)
					{
						botDriver->botData.unk5bc.drift_unk1 = 0x2aa;
					}
					else
					{
						botDriver->botData.unk5bc.drift_unk1 = -0x2aa;
					}
				}
			}
			else
			{
				if (((botDriver->actionsFlagSet & 1) == 0))
				{
					int iVar3 = navFrameCurr->unk[1];
					if (iVar3 != 0)
					{
#if 0
						if (iVar3 == 0)
						{
							trap(0x1c00);
						}
						if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->unk[0] == -0x80000000))
						{
							trap(0x1800);
						}
#endif
						botDriver->botData.unk5a8 = CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->unk[0]), iVar3), 8);
					}
				}
				deltaPosThisFrame = CTR_MipsSra(CTR_MipsNegLo(botDriver->botData.unk5bc.ai_speedY), 1);

				botDriver->botData.unk5bc.ai_speedY = deltaPosThisFrame;

				if (deltaPosThisFrame < 0)
				{
					botDriver->botData.unk5bc.ai_speedY = CTR_MipsNegLo(deltaPosThisFrame);
				}

				u8 bVar10 = (u8)CTR_MipsAddLo(botDriver->botData.unk626, 1);
				botDriver->botData.unk626 = bVar10;

				if (botDriver->botData.unk5ba == 1)
				{
					botDriver->actionsFlagSet |= 0x1800;
				}
				else
				{
					if (botDriver->botData.unk5ba == 2 && (2 < bVar10))
					{
						botDriver->botData.ai_progress_cooldown = 10;
						botDriver->botData.unk5bc.ai_speedLinear = 0;
						botDriver->botData.botFlags &= 0xfffffff9;
					}
				}

				botDriver->botData.ai_posBackup[1] = botDriver->quadBlockHeight;
			}
		}
		else
		{
			BOTS_Killplane(botThread);
		}

		botDriver->jump_LandingBoost = 0;
		botDriver->actionsFlagSet |= 1;
	}
	else
	{
		if ((local_30 == 0) && ((botDriver->actionsFlagSet & 1) != 0))
		{
			int iVar3 = navFrameCurr->unk[0];
			if (iVar3 != 0)
			{
#if 0
				if (iVar3 == 0)
				{
					trap(0x1c00);
				}
				if ((iVar3 == -1) && (deltaPosThisFrame * navFrameCurr->unk[1] == -0x80000000))
				{
					trap(0x1800);
				}
#endif
				botDriver->botData.unk5a8 = CTR_MipsSll(CTR_MipsDiv(CTR_MipsMulLo(deltaPosThisFrame, navFrameCurr->unk[1]), iVar3), 8);
			}
		}

		botDriver->actionsFlagSet &= 0xfffffffe;
		botDriver->actionsFlagSet |= 0x80000;

		botDriver->jump_LandingBoost = (s16)CTR_MipsAddLo((u16)botDriver->jump_LandingBoost, elapsedMilliseconds);
	}

	int iVar4_lifetime_3 = 0x18;

	if (botDriver->botData.unk5bc.drift_unk1 != 0)
	{
		iVar4_lifetime_3 = 0x60;
	}

	s16 iVar3_lifetime_2 = (s16)CTR_MipsSubLo((u16)botDriver->botData.unk5bc.ai_mulDrift, iVar4_lifetime_3);
	if (botDriver->botData.unk5bc.drift_unk1 < botDriver->botData.unk5bc.ai_mulDrift)
	{
		botDriver->botData.unk5bc.ai_mulDrift = iVar3_lifetime_2;

		if (iVar3_lifetime_2 < botDriver->botData.unk5bc.drift_unk1)
		{
			botDriver->botData.unk5bc.ai_mulDrift = botDriver->botData.unk5bc.drift_unk1;
		}
	}
	else
	{
		iVar4_lifetime_3 = (s16)CTR_MipsAddLo((u16)botDriver->botData.unk5bc.ai_mulDrift, iVar4_lifetime_3);

		botDriver->botData.unk5bc.ai_mulDrift = iVar4_lifetime_3;

		if (botDriver->botData.unk5bc.drift_unk1 < iVar4_lifetime_3)
		{
			botDriver->botData.unk5bc.ai_mulDrift = botDriver->botData.unk5bc.drift_unk1;
		}
	}
	botDriver->multDrift = botDriver->botData.unk5bc.ai_mulDrift;

	if ((botDriver->botData.botFlags & 2) != 0)
	{
		char newKartState; // uVar2
		short sVar7 = botDriver->botData.unk5ba;
		if (sVar7 == 2)
		{
			botDriver->botData.unk5bc.rotXZ = (s16)CTR_MipsSubLo((u16)botDriver->botData.unk5bc.rotXZ, 1);
		}
		else
		{
			if (sVar7 < 3)
			{
				if (sVar7 == 1)
				{
					u16 squishCooldownMaybe = (u16)botDriver->botData.unk5bc.ai_squishCooldown;
					int alsoSquishCooldownMaybe = CTR_MipsSubLo(botDriver->botData.unk5bc.ai_squishCooldown, 0xc);
					botDriver->botData.unk5bc.ai_squishCooldown = alsoSquishCooldownMaybe;
					botDriver->botData.unk5bc.ai_mulDrift = (s16)CTR_MipsAddLo((u16)botDriver->botData.unk5bc.ai_mulDrift, squishCooldownMaybe);
					if (alsoSquishCooldownMaybe < 0x100)
					{
						botDriver->botData.unk5bc.ai_squishCooldown = 0;
						botDriver->botData.unk5bc.ai_mulDrift = 0;
						botDriver->botData.unk5bc.ai_speedLinear = 0;
						botDriver->botData.botFlags &= 0xfffffff9;
					}
					newKartState = 3;
					botDriver->kartState = newKartState;
				}
			}
			else
			{
				if (sVar7 == 3)
				{
					u16 sVar7 = (u16)botDriver->botData.unk5bc.ai_squishCooldown;
					int iVar4 = CTR_MipsSubLo(botDriver->botData.unk5bc.ai_squishCooldown, 0xc);
					botDriver->botData.unk5bc.ai_squishCooldown = iVar4;
					botDriver->botData.unk5bc.ai_mulDrift = (s16)CTR_MipsAddLo((u16)botDriver->botData.unk5bc.ai_mulDrift, sVar7);
					if (iVar4 < 0x200)
					{
						botDriver->botData.unk5bc.ai_squishCooldown = 0;
						botDriver->botData.unk5bc.ai_mulDrift = 0;
					}
					iVar4 = (s16)CTR_MipsSubLo((u16)botDriver->botData.unk5bc.rotXZ, elapsedMilliseconds);
					botDriver->botData.unk5bc.rotXZ = iVar4;
					if (iVar4 <= 0)
					{
						botDriver->botData.unk5bc.ai_speedY = 0x1400;
						botDriver->botData.botFlags &= 0xfffffff9;
						botDriver->actionsFlagSet |= 0x400;
					}
				}
				else
				{
					if (sVar7 == 5)
					{
						struct Thread *plant = botDriver->plantEatingMe;

						struct Instance *plantInst = plant->inst;

						if (botDriver->botData.unk5bc.rotXZ < 0xb40)
						{
							SVECTOR v;
							v.vx = 0xfa;
							if ((plant->object != NULL) && (((struct Plant *)plant->object)->LeftOrRight != 0))
							{
								v.vx = -0xfa;
							}
							v.vy = 0;
							v.vz = 0x2ee;

							SetRotMatrix(&plantInst->matrix);
							SetTransMatrix(&plantInst->matrix);

							VECTOR v2;
							long l3;

							RotTrans(&v, &v2, &l3);

							gGT->pushBuffer[botDriver->driverID].pos[0] = v2.vx;
							gGT->pushBuffer[botDriver->driverID].pos[1] = CTR_MipsAddLo(plantInst->matrix.t[1], 0xc0);
							gGT->pushBuffer[botDriver->driverID].pos[2] = v2.vz;

							int camDriverXDelta = CTR_MipsSubLo(v2.vx, plantInst->matrix.t[0]);
							int camY = gGT->pushBuffer[botDriver->driverID].pos[1];
							int driverY = plantInst->matrix.t[1];
							int camDriverZDelta = CTR_MipsSubLo(v2.vz, plantInst->matrix.t[2]);

							int rotY = ratan2(camDriverXDelta, camDriverZDelta);
							gGT->pushBuffer[botDriver->driverID].rot[1] = rotY;

							int rotX = SquareRoot0_stub(
							    CTR_MipsAddLo(CTR_MipsMulLo(camDriverXDelta, camDriverXDelta), CTR_MipsMulLo(camDriverZDelta, camDriverZDelta)));
							rotX = ratan2(CTR_MipsSubLo(camY, driverY), rotX);

							gGT->pushBuffer[botDriver->driverID].rot[0] = CTR_MipsSubLo(0x800, rotX);
							gGT->pushBuffer[botDriver->driverID].rot[2] = 0;
						}

						botDriver->botData.unk5bc.ai_speedLinear = 0;
						int iVar4 = (s16)CTR_MipsSubLo((u16)botDriver->botData.unk5bc.rotXZ, elapsedMilliseconds);
						botDriver->botData.unk5bc.rotXZ = iVar4;
						newKartState = 5;
						if (iVar4 < 1)
						{
							botDriver->botData.botFlags &= 0xfffffff9;
							botInstance->flags &= 0xffffff7f;
							botDriver->botData.ai_progress_cooldown = 1;
							BOTS_MaskGrab(botThread);
							newKartState = 5;
						}
						botDriver->kartState = newKartState;
					}
				}
			}
		}
	}

	if (botDriver->forcedJump_trampoline != 0)
	{
		int turtleJumpForce = CTR_MipsAddLo(CTR_MipsSll(botDriver->const_JumpForce, 1), botDriver->const_JumpForce); // iVar4

		if (botDriver->forcedJump_trampoline != 2)
		{
			turtleJumpForce = CTR_MipsDiv(turtleJumpForce, 2);
		}

		if (botDriver->botData.unk5bc.ai_speedY < turtleJumpForce)
		{
			botDriver->botData.unk5bc.ai_speedY = turtleJumpForce;
		}

		botDriver->forcedJump_trampoline = 0;
	}

	if (((navFrameSpecialBits & 0x10) != 0) &&
	    ((0x1c1f < botDriver->botData.unk5bc.ai_speedLinear) || (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)))
	{
		int iVar4 = (local_3c & 0xf);
		botDriver->botData.unk5bc.ai_speedY = sdata->NavPath_ptrHeader[botDriver->botData.botPath]->rampPhys2[iVar4];
		botDriver->botData.unk5bc.ai_speedLinear = sdata->NavPath_ptrHeader[botDriver->botData.botPath]->rampPhys1[iVar4];

		if (data.characterIDs[botDriver->driverID] == NITROS_OXIDE)
		{
			botDriver->botData.unk5bc.ai_squishCooldown = 0;
			botDriver->botData.unk5bc.ai_mulDrift = 0;
			botDriver->botData.botFlags &= 0xfffffff9;
		}
	}

	// MUST be unsigned, same reason as the other -0x31
	unsigned char cmp1 = navFrameCurr->rot[3] - 0x31;
	unsigned char cmp2 = navFrameNext->rot[3] - 0x31;

	if ((0x9e < cmp1) && (0x9e < cmp2))
	{
		if (((botDriver->botData.botFlags & 1) == 0) && ((botDriver->actionsFlagSet & 1) != 0))
		{
			// Lerp nav-frame rotation through the shortest signed 12-bit delta.
			int uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[0], 4), CTR_MipsSll(navFrameCurr->rot[0], 4)) & 0xfff;
			if (0x7ff < uVar8)
			{
				uVar8 = CTR_MipsSubLo(uVar8, 0x1000);
			}

			botDriver->botData.ai_rot4[0] = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[0], 4), CTR_MipsSra(CTR_MipsMulLo(uVar8, percentage), 0xc)) & 0xfff;

			uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[2], 4), CTR_MipsSll(navFrameCurr->rot[2], 4)) & 0xfff;
			if (0x7ff < uVar8)
			{
				uVar8 = CTR_MipsSubLo(uVar8, 0x1000);
			}

			botDriver->botData.ai_rot4[2] = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[2], 4), CTR_MipsSra(CTR_MipsMulLo(uVar8, percentage), 0xc)) & 0xfff;
		}

		int other_uVar8 = CTR_MipsSubLo(CTR_MipsSll(navFrameNext->rot[1], 4), CTR_MipsSll(navFrameCurr->rot[1], 4)) & 0xfff;
		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
		}

		botDriver->botData.ai_rot4[1] = CTR_MipsAddLo(CTR_MipsSll(navFrameCurr->rot[1], 4), CTR_MipsSra(CTR_MipsMulLo(other_uVar8, percentage), 0xc)) & 0xfff;

		if ((botDriver->botData.botFlags & 1) != 0)
		{
			other_uVar8 = 0;
		}

		other_uVar8 = CTR_MipsSubLo(CTR_MipsSll(other_uVar8, 1), botDriver->turnAngleCurr) & 0xfff;

		bool bVar1 = other_uVar8 < 0x21;
		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
			bVar1 = other_uVar8 < 0x21;
		}
		if (bVar1)
		{
			if (other_uVar8 < -0x20)
			{
				other_uVar8 = -0x20;
			}
		}
		else
		{
			other_uVar8 = 0x20;
		}
		other_uVar8 = (s16)CTR_MipsAddLo((u16)botDriver->turnAngleCurr, other_uVar8);
		botDriver->turnAngleCurr = other_uVar8;

		s16 sVar7;
		if (botDriver->botData.unk5bc.drift_unk1 == 0)
		{
			other_uVar8 &= 0xfff;
			sVar7 = other_uVar8;
			botDriver->botData.unk5bc.ai_simpTurnState = sVar7;
			if (0x7ff < other_uVar8)
			{
				botDriver->botData.unk5bc.ai_simpTurnState = (s16)CTR_MipsSubLo(sVar7, 0x1000);
			}
			sVar7 = (s16)CTR_MipsSra(botDriver->botData.unk5bc.ai_simpTurnState, 2);
		}
		else
		{
			int uVar11 = CTR_MipsNegLo((u16)botDriver->botData.unk5bc.ai_mulDrift) & 0xfff;

			botDriver->botData.unk5bc.ai_simpTurnState = uVar11;
			if (0x7ff < uVar11)
			{
				botDriver->botData.unk5bc.ai_simpTurnState = (s16)CTR_MipsSubLo(uVar11, 0x1000);
			}

			sVar7 = (s16)CTR_MipsSra(botDriver->botData.unk5bc.ai_simpTurnState, 3);
		}
		botDriver->botData.unk5bc.ai_simpTurnState = sVar7;

		other_uVar8 = CTR_MipsSubLo(botDriver->botData.unk5bc.ai_simpTurnState, botDriver->wheelRotation) & 0xfff;
		bool other_bVar1 = other_uVar8 < 0x21;

		if (0x7ff < other_uVar8)
		{
			other_uVar8 = CTR_MipsSubLo(other_uVar8, 0x1000);
			other_bVar1 = other_uVar8 < 0x21;
		}
		sVar7 = other_uVar8;
		if (other_bVar1)
		{
			if (other_uVar8 < -0x20)
			{
				sVar7 = -0x20;
			}
		}
		else
		{
			sVar7 = 0x20;
		}

		botDriver->wheelRotation = (s16)CTR_MipsAddLo((u16)botDriver->wheelRotation, sVar7);
		botDriver->simpTurnState = (s8)(u8)botDriver->botData.unk5bc.ai_simpTurnState;
	}

	botDriver->rotCurr.x = botDriver->botData.ai_rot4[0];
	botDriver->rotCurr.y = botDriver->botData.ai_rot4[1];
	botDriver->rotCurr.z = botDriver->botData.ai_rot4[2];

	int badnessRecieveTimer = botDriver->clockReceive; // iVar4

	if (badnessRecieveTimer == 0)
	{
		badnessRecieveTimer = botDriver->squishTimer;
		if (badnessRecieveTimer == 0)
		{
			if (botDriver->thCloud != NULL)
			{
				badnessRecieveTimer = CTR_MipsSll(gGT->timer, 2);

				goto badEffectKartWiggle;
			}
		}
	}
	else
	{
	badEffectKartWiggle:
		int wiggleX = CTR_MipsSra(CTR_MipsMulLo(MATH_Sin(CTR_MipsMulLo(badnessRecieveTimer, 0xc)), 50), 10);
		int wiggleZ = CTR_MipsSra(CTR_MipsMulLo(MATH_Cos(CTR_MipsMulLo(badnessRecieveTimer, 0xc)), 50), 10);
		SVECTOR wiggle = {
		    .vx = (s16)wiggleX,
		    .vy = 0,
		    .vz = (s16)wiggleZ,
		    .pad = 0,
		};

		gte_ldv0(&wiggle);
		gte_mvmva(1, 0, 0, 3, 0);

		botDriver->botData.unk5bc.ai_turboMeter = 0;

		botDriver->rotCurr.x = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.x, (s16)MFC2(25));
		botDriver->rotCurr.y = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.y, (s16)MFC2(26));
		botDriver->rotCurr.z = (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.z, (s16)MFC2(27));
	}

	if ((botDriver->botData.botFlags & 9) == 0)
	{
		s16 rot[3];
		rot[0] = (s16)CTR_MipsSll(navFrameCurr->rot[0], 4);
		rot[1] = (s16)CTR_MipsSll(navFrameCurr->rot[1], 4);
		rot[2] = (s16)CTR_MipsSll(navFrameCurr->rot[2], 4);
		MATRIX m;

		ConvertRotToMatrix(&m, rot);

		botDriver->AxisAngle3_normalVec[0] = m.m[0][1];
		botDriver->AxisAngle3_normalVec[1] = m.m[1][1];
		botDriver->AxisAngle3_normalVec[2] = m.m[2][1];

		botInstance->bitCompressed_NormalVector_AndDriverIndex = (((u16)m.m[0][1] >> 6) & 0xff) | (((u16)m.m[1][1] & 0x3fc0) << 2) |
		                                                         ((((u16)m.m[2][1] >> 6) & 0xff) << 0x10) |
		                                                         CTR_MipsSll(CTR_MipsAddLo(botDriver->driverID, 1), 0x18);
	}

	ConvertRotToMatrix(&botInstance->matrix, &botDriver->rotCurr.x);

	// c is row-major (i.e., ticking the rightmost indeces has smaller memory address delta vs ticking the leftmost indeces)
	botDriver->AxisAngle2_normalVec[0] = botInstance->matrix.m[0][1];
	botDriver->AxisAngle2_normalVec[1] = botInstance->matrix.m[1][1];
	int uVar6 = botInstance->matrix.m[2][1];
	botDriver->AxisAngle2_normalVec[2] = uVar6;

	botDriver->angle = botDriver->rotCurr.y;

	botDriver->speedApprox = botDriver->botData.unk5bc.ai_speedLinear;

	botDriver->speed = botDriver->botData.unk5bc.ai_speedLinear;
	botDriver->jumpHeightPrev = botDriver->jumpHeightCurr;
	botDriver->axisRotationX = botDriver->botData.ai_rot4[1] & 0xfff;

	int iVar4_lifetime_2 = MATH_Cos(navFrameCurr->rot[3]);

	botDriver->jumpHeightCurr = (s16)CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedY, iVar4_lifetime_2), 0xc);

	iVar4_lifetime_2 = MATH_Cos(botDriver->axisRotationX);

	botDriver->zSpeed = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedLinear, iVar4_lifetime_2), 0xc);

	iVar4_lifetime_2 = MATH_Sin(botDriver->axisRotationX);

	int uVar11 = botDriver->rotCurr.z & 0xfff;

	botDriver->ySpeed = botDriver->botData.unk5bc.ai_speedY;
	botDriver->rotCurr.z = uVar11;

	botDriver->xSpeed = CTR_MipsSra(CTR_MipsMulLo(botDriver->botData.unk5bc.ai_speedLinear, iVar4_lifetime_2), 0xc);
	if (0x7ff < uVar11)
	{
		botDriver->rotCurr.z = (s16)CTR_MipsSubLo(uVar11, 0x1000);
	}
	botDriver->rotCurr.y =
	    (s16)CTR_MipsAddLo((u16)botDriver->rotCurr.y, CTR_MipsAddLo((u16)botDriver->botData.unk5bc.ai_mulDrift, (u16)botDriver->turnAngleCurr));

	botDriver->posCurr.x = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[0], botDriver->botData.ai_posBackup[0]);
	botDriver->posCurr.y = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[1], botDriver->botData.ai_posBackup[1]);
	botDriver->posCurr.z = CTR_MipsAddLo(botDriver->botData.unk5bc.ai_velAxis[2], botDriver->botData.ai_posBackup[2]);

	botInstance->matrix.t[0] = CTR_MipsSra(botDriver->posCurr.x, 8);
	botInstance->matrix.t[1] = CTR_MipsAddLo(CTR_MipsSra(botDriver->posCurr.y, 8), botDriver->Screen_OffsetY);
	botInstance->matrix.t[2] = CTR_MipsSra(botDriver->posCurr.z, 8);

	badnessRecieveTimer = botDriver->clockReceive;

	if (badnessRecieveTimer == 0)
	{
		badnessRecieveTimer = botDriver->squishTimer;
		if (badnessRecieveTimer == 0)
		{
			if (0x100 < botDriver->speedApprox)
			{
				botDriver->hazardTimer = (s16)CTR_MipsSubLo((u16)botDriver->hazardTimer, elapsedMilliseconds);
			}

			u16 uVar11 = (u16)botDriver->hazardTimer & 0xfffe;
			botDriver->hazardTimer = (s16)uVar11;

			if ((s16)uVar11 >= 0)
			{
				botDriver->hazardTimer = -2;
			}
			goto LAB_8001686c;
		}
	}

	if ((botDriver->actionsFlagSet & 1) == 0)
	{
		int iVar4 = botDriver->speedApprox;
		if (iVar4 < 0)
		{
			iVar4 = CTR_MipsNegLo(iVar4);
		}

		if (iVar4 < 0x101)
		{
			goto LAB_800167fc;
		}

		if (badnessRecieveTimer < 0)
		{
			badnessRecieveTimer = CTR_MipsNegLo(badnessRecieveTimer);
		}
	LAB_8001680c:
		botDriver->hazardTimer = (s16)CTR_MipsNegLo(badnessRecieveTimer);
	}
	else
	{
		int iVar4 = botDriver->speedApprox;

		if (iVar4 < 0)
		{
			iVar4 = CTR_MipsNegLo(iVar4);
		}

		if (iVar4 < 0x101)
		{
		LAB_800167fc:
			badnessRecieveTimer = botDriver->hazardTimer;
			if (0 < badnessRecieveTimer)
			{
				goto LAB_8001680c;
			}
		}
		else
		{
			if (badnessRecieveTimer < 0)
			{
				badnessRecieveTimer = CTR_MipsNegLo(badnessRecieveTimer);
			}
			botDriver->hazardTimer = (s16)badnessRecieveTimer;
		}
	}

	botDriver->hazardTimer = (s16)((u16)botDriver->hazardTimer | 1); // increment it by 1 if even.

LAB_8001686c:

	if (((navFrameSpecialBits & 0x40) != 0) || ((botDriver->botData.botFlags & 9) != 0))
	{
		BOTS_LevInstColl(botThread);
	}

	if ((navFrameFlags & 0x8000) != 0)
	{
		botInstance->flags |= 0x2000;
	}

	VehPhysForce_TranslateMatrix(botThread, botDriver);

	VehPhysForce_RotAxisAngle(&botDriver->matrixMovingDir, &botDriver->AxisAngle2_normalVec[0], botDriver->angle);

	VehFrameProc_Driving(botThread, botDriver);

	if (((botDriver->botData.botFlags & 2) != 0) && (botDriver->botData.unk5ba == 2))
	{
		s16 rot[3];
		rot[0] = (s16)CTR_MipsSll(botDriver->botData.unk5bc.rotXZ, 8);
		rot[2] = 0;
		rot[1] = (s16)CTR_MipsMulLo(botDriver->botData.unk5bc.rotXZ, 0xe0);

		ConvertRotToMatrix(&sdata->rotXZ, &rot[0]);

		MATH_MatrixMul(&sdata->rotXYZ, &botInstance->matrix, &sdata->rotXZ);

		MATH_MatrixMul(&botInstance->matrix, &sdata->rotXYZ, &data.identity);

		botInstance->matrix.t[1] = CTR_MipsAddLo(botInstance->matrix.t[1], 0x20);
	}

	if ((botDriver->botData.botFlags & 4) == 0)
	{
		VehEmitter_DriverMain(botThread, botDriver);
	}

	if (botInstance->thread->modelIndex == DYNAMIC_PLAYER)
	{
		EngineSound_Player(botDriver);
	}

	s16 camRot = CTR_MipsSubLo(botDriver->angle, botDriver->botData.ai_rotY_608) & 0xfff;

	botDriver->rotCurr.w = (s16)CTR_MipsNegLo(camRot);

	if (0x7ff < camRot)
	{
		camRot |= 0xf000;
	}

	botDriver->botData.ai_rotY_608 = (s16)CTR_MipsAddLo((u16)botDriver->botData.ai_rotY_608, CTR_MipsSra(camRot, 3));
	botDriver->botData.ai_rotY_608 &= 0xfff;

	if (botThread->modelIndex == DYNAMIC_PLAYER)
	{
		s16 posTop[3];
		s16 posBot[3];

		posTop[0] = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.ai_posBackup[0], botDriver->botData.unk5bc.ai_velAxis[0]), 8);
		posBot[1] = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.ai_posBackup[1], botDriver->botData.unk5bc.ai_velAxis[1]), 8);
		posTop[1] = (s16)CTR_MipsSubLo(posBot[1], 0x100);
		posBot[1] = (s16)CTR_MipsAddLo(posBot[1], 0x40);
		posTop[2] = (s16)CTR_MipsSra(CTR_MipsAddLo(botDriver->botData.ai_posBackup[2], botDriver->botData.unk5bc.ai_velAxis[2]), 8);
		posBot[0] = posTop[0];
		posBot[2] = posTop[2];

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.qbFlagsWanted = 0x1000;
		sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
		sps->Union.QuadBlockColl.searchFlags = 2;

		COLL_SearchBSP_CallbackQUADBLK((u_int *)&posTop[0], (u_int *)&posBot[0], sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			botDriver->underDriver = sps->Set2.ptrQuadblock;
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80016b00-0x80016ec8
u32 BOTS_ChangeState(struct Driver *driverVictim, int damageType, struct Driver *driverAttacker, int reason)
{
	driverVictim->ChangeState_param2 = 0;

	if (driverVictim->kartState == KS_MASK_GRABBED)
	{
		return 0;
	}

	driverVictim->kartState = KS_NORMAL;

	switch (damageType)
	{
	case 0:
		if ((driverVictim->botData.botFlags & 2) != 0)
		{
			return 0;
		}
		break;
	case 1:
	case 4:
		if ((driverVictim->botData.botFlags & 2) == 0)
		{
			driverVictim->botData.unk5ba = 1;
			driverVictim->botData.unk5bc.ai_turboMeter = 0;

			if ((data.characterIDs[driverVictim->driverID] != NITROS_OXIDE) || ((driverVictim->actionsFlagSet & 1) != 0))
			{
				driverVictim->reserves = 0;
				driverVictim->turbo_outsideTimer = 0;
				driverVictim->botData.unk5bc.unk5cc = 0;

				int newSpeed;

				if (data.characterIDs[driverVictim->driverID] == NITROS_OXIDE)
				{
					newSpeed = CTR_MipsSra(driverVictim->botData.unk5bc.ai_speedLinear, 1);
				}
				else
				{
					newSpeed = CTR_MipsSra(driverVictim->botData.unk5bc.ai_speedLinear, 2);
				}

				driverVictim->botData.unk5bc.ai_speedLinear = newSpeed;
			}

			driverVictim->botData.unk5bc.ai_squishCooldown = 0x300;
			driverVictim->botData.ai_progress_cooldown = 0;

			driverVictim->botData.botFlags |= 2;
		}

		if (damageType == 4)
		{
			if (driverVictim->instSelf->thread->modelIndex == DYNAMIC_PLAYER && driverVictim->burnTimer == 0)
			{
				OtherFX_Play(0x69, 1);
			}

			driverVictim->burnTimer = 0xf00;
		}
		break;
	case 2:
		driverVictim->botData.unk626 = 0;
		driverVictim->botData.unk5bc.ai_turboMeter = 0;
		driverVictim->botData.unk5ba = 2;
		driverVictim->reserves = 0;
		driverVictim->turbo_outsideTimer = 0;
		driverVictim->botData.unk5bc.unk5cc = 0;
		driverVictim->botData.unk5bc.ai_speedY = sdata->AI_VelY_WhenBlasted_0x3000;

		if ((driverVictim->botData.botFlags & 2) == 0)
		{
			driverVictim->botData.unk5bc.ai_speedLinear = CTR_MipsSra(driverVictim->botData.unk5bc.ai_speedLinear, 3);
			driverVictim->botData.ai_posBackup[1] = CTR_MipsAddLo(driverVictim->botData.ai_posBackup[1], 0x4000);
		}

		driverVictim->botData.ai_progress_cooldown = 0;
		driverVictim->matrixArray = 0;
		driverVictim->botData.botFlags |= 2;

		if (driverAttacker == NULL)
		{
			return 1;
		}

		if ((driverAttacker->actionsFlagSet & 0x100000) == 0)
		{
			Voiceline_RequestPlay(1, data.characterIDs[driverVictim->driverID], 0x10);
		}
		break;
	case 3:
		driverVictim->botData.unk5bc.ai_turboMeter = 0;

		if (driverVictim->instSelf->thread->modelIndex == DYNAMIC_PLAYER && driverVictim->botData.unk5bc.ai_squishCooldown == 0)
		{
			OtherFX_Play(0x5a, 1);
		}

		driverVictim->botData.unk5ba = 3;
		driverVictim->botData.unk5bc.ai_squishCooldown = 0x300;
		driverVictim->botData.unk5bc.rotXZ = 0xf00;
		driverVictim->squishTimer = 0xf00;
		driverVictim->reserves = 0;
		driverVictim->turbo_outsideTimer = 0;
		driverVictim->botData.unk5bc.unk5cc = 0;
		driverVictim->botData.unk5bc.ai_speedY = 0;
		driverVictim->botData.ai_progress_cooldown = 0;
		driverVictim->botData.unk5bc.ai_speedLinear = CTR_MipsSra(driverVictim->botData.unk5bc.ai_speedLinear, 1);
		driverVictim->botData.botFlags |= 6;
		break;
	case 5:
		driverVictim->botData.unk5bc.ai_turboMeter = 0;
		driverVictim->botData.unk5bc.ai_speedLinear = 0;
		driverVictim->botData.unk5bc.ai_speedY = 0;
		driverVictim->botData.unk5bc.unk5cc = 0;
		driverVictim->instSelf->flags |= 0x80;
		driverVictim->botData.unk5bc.rotXZ = 0xd20;
		driverVictim->botData.unk5ba = 5;
		driverVictim->kartState = KS_MASK_GRABBED;
		driverVictim->botData.botFlags |= 6;
		driverVictim->instSelf->thread->flags |= 0x1000;
		break;
	default:
		driverVictim->botData.ai_progress_cooldown = 0x3c;
	}

	if (driverAttacker != NULL && damageType != 0)
	{
		driverAttacker->numTimesAttacked++;
		switch (damageType)
		{
		case 1:
			driverAttacker->numTimesBombsHitSomeone++;
			break;
		case 3:
			driverAttacker->numTimesMissileHitSomeone++;
			break;
		case 4:
			driverAttacker->numTimesMovingPotionHitSomeone++;
			break;
		}
	}
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80016ec8-0x8001702c
void BOTS_CollideWithOtherAI(struct Driver *robot_1, struct Driver *robot_2)
{
	struct NavFrame *nfCurr;
	struct NavFrame *nfNext;

	// first determine which driver bumps forward and which bumps backwards
	if (robot_1->driverRank < robot_2->driverRank)
	{
		struct Driver *temp = robot_2;
		robot_2 = robot_1;
		robot_1 = temp;
	}
	// robot_1 = iVar2
	// robot_2 = param_2

	s16 *uVar3;
	s16 *estimatePos;
	if ((robot_1->botData.botFlags & 1) == 0)
	{
		// nav path index
		s16 botPathIndex = robot_1->botData.botPath;

		// pointer to navFrame
		nfCurr = robot_1->botData.botNavFrame;
		nfNext = nfCurr + 1;

		// iVar4
		estimatePos = &nfCurr->pos[0];

		// if you go out of bounds
		if (sdata->NavPath_ptrHeader[botPathIndex]->last <= (struct NavFrame *)nfNext)
		{
			// loop back to first navFrame
			nfNext = sdata->NavPath_ptrNavFrameArray[botPathIndex];
		}
	}
	else
	{
		// pointer to nav frame
		nfNext = robot_1->botData.botNavFrame;

		// iVar4
		estimatePos = robot_1->botData.estimatePos;
	}

	uVar3 = &nfNext->pos[0];

	s16 pos[3];
	// position of one driver
	pos[0] = (s16)CTR_MipsSra(robot_1->posCurr.x, 8);
	pos[1] = (s16)CTR_MipsSra(robot_1->posCurr.y, 8);
	pos[2] = (s16)CTR_MipsSra(robot_1->posCurr.z, 8);

	// two navFrame structs, and position pointer
	int res1 = CAM_MapRange_PosPoints(uVar3, estimatePos, &pos[0]);

	// position of other driver
	pos[0] = (s16)CTR_MipsSra(robot_2->posCurr.x, 8);
	pos[1] = (s16)CTR_MipsSra(robot_2->posCurr.y, 8);
	pos[2] = (s16)CTR_MipsSra(robot_2->posCurr.z, 8);

	// two navFrame structs, and position pointer
	int res2 = CAM_MapRange_PosPoints(uVar3, estimatePos, &pos[0]);

	// reduce speed of one AI,
	// the AI that is closer to the previous nav point,
	// who therefore is the driver in the back of the collision

	if (res1 < res2)
	{
		int speed = CTR_MipsSubLo(robot_2->botData.unk5bc.ai_speedLinear, 3000);
		speed = ((speed < 0) ? 0 : speed); // clamp to 0

		robot_1->botData.unk5bc.ai_speedLinear = speed;
	}
	else
	{
		int speed = CTR_MipsSubLo(robot_1->botData.unk5bc.ai_speedLinear, 3000);
		speed = ((speed < 0) ? 0 : speed); // clamp to 0

		robot_2->botData.unk5bc.ai_speedLinear = speed;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001702c-0x80017164.
void BOTS_GotoStartingLine(struct Driver *d)
{
	int accelDuration;
	s16 rotY;

	sdata->unk_counter_upTo450 = 0;

	VehBirth_TeleportSelf(d, 3, 0);

	// get position where driver should spawn
	u8 spawnPos = sdata->kartSpawnOrderArray[d->driverID];

	d->botData.unk5bc.unk5cc = 0;
	d->botData.unk5bc.ai_speedY = 0;
	d->botData.unk5bc.ai_speedLinear = 0;
	CTR_SET_VEC3(d->botData.unk5bc.ai_accelAxis, 0, 0, 0);
	CTR_SET_VEC3(d->botData.unk5bc.ai_velAxis, 0, 0, 0);

	d->botData.ai_posBackup[0] = d->posCurr.x;
	d->botData.ai_posBackup[1] = d->posCurr.y;
	d->botData.ai_posBackup[2] = d->posCurr.z;

	d->botData.unk5a8 = 0;

	// current navFrame pointer, first navFrame on path
	d->botData.botNavFrame = sdata->NavPath_ptrNavFrameArray[d->botData.botPath];

	BOTS_SetRotation(d, 1);

	// time until full acceleration from start
	accelDuration = sdata->AI_AccelFrameCount;

	// get acceleration order from spawn order
	u8 accel = sdata->accelerateOrder[spawnPos];

	d->rotCurr.z = 0;
	d->rotPrev.z = 0;
	d->botData.ai_rot4[2] = 0;
	d->rotCurr.x = 0;
	d->rotPrev.x = 0;
	d->botData.ai_rot4[0] = 0;
	d->turnAngleCurr = 0;

	// turn on 21st flag of actions flag set, means driver is AI
	d->actionsFlagSet |= 0x100000;

	// calculate Y rotation
	rotY = (s16)CTR_MipsSll((u8)d->botData.estimateRotNav[1], 4);

	// every possible Y rotation
	d->botData.ai_rotY_608 = rotY;
	d->angle = rotY;
	d->rotCurr.y = rotY;
	d->rotPrev.y = rotY;
	d->botData.ai_rot4[1] = rotY;

	// acceleration from start-line to full speed
	d->botData.botAccel = CTR_MipsMulLo(accelDuration, accel);

	// cooldown before next weapon
	int rng = RngDeadCoed(&sdata->const_0x30215400);
	d->botData.weaponCooldown = (s16)CTR_MipsAddLo(CTR_MipsSra(rng, 8) & 0xff, 300);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80017164-0x80017318.
struct Driver *BOTS_Driver_Init(int driverID)
{
	struct Thread *t;
	struct Driver *d;

	s8 initialNavPathIndex = sdata->driver_pathIndexIDs[driverID];
	s8 navPathIndex;
	s16 navPathPointsCount; // = sdata->NavPath_ptrHeader[navPathIndex]->numPoints;

	navPathIndex = initialNavPathIndex;
	while (1)
	{
		navPathPointsCount = sdata->NavPath_ptrHeader[navPathIndex]->numPoints;
		if (1 < navPathPointsCount)
			break; // success

		navPathIndex--;

		// If subtracted below zero,
		// go back to highest index (2)
		if (navPathIndex < 0)
			navPathIndex = 2;

		// If all 3 are checked, quit
		if (navPathIndex == initialNavPathIndex)
			return NULL;
	}

	// path data found
	t = PROC_BirthWithObject(
	    // creation flags
	    SIZE_RELATIVE_POOL_BUCKET(0x62c, NONE, LARGE, ROBOT),

	    BOTS_ThTick_Drive, // behavior
	    0,                 //"robotcar",	// debug name
	    0                  // thread relative
	);

	d = t->object;
	memset(d, 0x0, 0x62c);
	VehBirth_NonGhost(t, driverID);
	sdata->gGT->drivers[driverID] = d;
	t->modelIndex = DYNAMIC_ROBOT_CAR;

	d->botData.botPath = navPathIndex;
	d->botData.botNavFrame = sdata->NavPath_ptrNavFrameArray[navPathIndex];
	d->actionsFlagSet |= 0x100000;
	LIST_AddFront(&sdata->navBotList[navPathIndex], (struct Item *)(&d->botData));

	sdata->gGT->numBotsNextGame++;
	BOTS_GotoStartingLine(d);
	return d;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80017318-0x800175cc.
void BOTS_Driver_Convert(struct Driver *d)
{
	// if already AI, quit
	if ((d->actionsFlagSet & 0x100000) != 0)
		return;

	UI_RaceEnd_GetDriverClock(d);

	s8 initialNavPathIndex = sdata->driver_pathIndexIDs[d->driverID];
	s8 navPathIndex;
	s16 navPathPointsCount; // = sdata->NavPath_ptrHeader[navPathIndex]->numPoints;

	navPathIndex = initialNavPathIndex;
	while (1)
	{
		navPathPointsCount = sdata->NavPath_ptrHeader[navPathIndex]->numPoints;
		if (1 < navPathPointsCount)
			break; // success

		navPathIndex--;

		// If subtracted below zero,
		// go back to highest index (2)
		if (navPathIndex < 0)
			navPathIndex = 2;

		// If all 3 are checked, quit
		if (navPathIndex == initialNavPathIndex)
			return;
	}

	memset(&d->botData, 0, sizeof(struct BotData));

	d->botData.unk5bc.ai_speedY = d->ySpeed;

	s32 speedApprox = d->speedApprox;

	d->botData.botPath = navPathIndex;

	speedApprox = ((speedApprox < 0) ? CTR_MipsNegLo(speedApprox) : speedApprox);

	d->botData.unk5bc.ai_speedLinear = speedApprox;

	struct NavFrame *firstNavFrame = sdata->NavPath_ptrNavFrameArray[navPathIndex];

	d->botData.unk5a8 = 0;
	d->turnAngleCurr = 0;
	d->multDrift = 0;
	d->ampTurnState = 0;
	d->set_0xF0_OnWallRub = 0;

	d->botData.botNavFrame = firstNavFrame;

	d->instSelf->thread->funcThTick = BOTS_ThTick_Drive;

	if ((sdata->gGT->gameMode1 & 0x20) != 0)
	{ // you are in battle mode
		struct NavFrame *nf = NAVHEADER_GETFRAME(sdata->NavPath_ptrHeader[navPathIndex]);
		d->posCurr.x = CTR_MipsSll(nf->pos[0], 8);
		d->posCurr.y = CTR_MipsSll(nf->pos[1], 8);
		d->posCurr.z = CTR_MipsSll(nf->pos[2], 8);
	}

	LIST_AddFront(&sdata->navBotList[navPathIndex], (struct Item *)&d->botData);

	BOTS_SetRotation(d, 0);

	GAMEPAD_JogCon2(d, 0, 0);

	u32 oldActionFlagsSet = d->actionsFlagSet;

	d->actionsFlagSet = (oldActionFlagsSet & 0xfffffff3) | 0x100000;

	if ((oldActionFlagsSet & 0x2000000) != 0)
	{
		CAM_EndOfRace(&sdata->gGT->cameraDC[d->driverID], d);
	}

	int damageType;
	switch (d->kartState)
	{
	default:
		return;
	case KS_SPINNING:
		damageType = 1;
		break;
	case KS_BLASTED:
		damageType = 2;
		break;
	}

	BOTS_ChangeState(d, damageType, NULL, 0);
}
