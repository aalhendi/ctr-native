#include <common.h>

struct VehEmitterSkidmark
{
	s16 x0;
	s16 y0;
	s16 z0;
	u8 color;
	u8 flags;
	s16 x1;
	s16 y1;
	s16 z1;
	s16 pad;
};

_Static_assert(sizeof(struct VehEmitterSkidmark) == 0x10);

static void VehEmitter_SetRotTransMatrix(MATRIX *m)
{
	gte_SetRotMatrix(m);
	gte_SetTransMatrix(m);
}

static void VehEmitter_RotVec(const SVECTOR *in, VECTOR *out)
{
	gte_ldv0(in);
	gte_rtv0();
	gte_stlvnl(out);
}

static void VehEmitter_RotTransVec(const SVECTOR *in, VECTOR *out)
{
	gte_ldv0(in);
	gte_rt();
	gte_stlvnl(out);
}

static struct VehEmitterSkidmark *VehEmitter_GetSkidmark(struct Driver *d, u8 frameIndex, int tireIndex)
{
	return (struct VehEmitterSkidmark *)(void *)&d->skidmarks[((frameIndex & 7) * 0x40) + (tireIndex * 0x10)];
}

static void VehEmitter_WriteSkidmark(struct Driver *d, u8 frameIndex, int tireIndex, int x, int y, int z, int widthX, int widthZ, u8 color, u8 flags)
{
	struct VehEmitterSkidmark *mark = VehEmitter_GetSkidmark(d, frameIndex, tireIndex);

	mark->x0 = (s16)(x + widthX);
	mark->y0 = (s16)y;
	mark->z0 = (s16)(z + widthZ);
	mark->color = color;
	mark->flags = flags;
	mark->x1 = (s16)(x - widthX);
	mark->y1 = (s16)y;
	mark->z1 = (s16)(z - widthZ);
}

static void VehEmitter_WriteSkidmarkPair(struct Driver *d, int tireIndex, int x, int y, int z, int lateralX, int lateralZ, int widthX, int widthZ, u8 color,
                                         u8 flags)
{
	u8 frame = (u8)d->skidmarkFrameIndex;

	VehEmitter_WriteSkidmark(d, frame, tireIndex, x, y, z, widthX, widthZ, color, flags);

	x += lateralX;
	z += lateralZ;
	VehEmitter_WriteSkidmark(d, (u8)(frame - 1), tireIndex, x, y, z, widthX >> 1, widthZ >> 1, color, flags);
}

static void VehEmitter_Skidmarks(struct Thread *thread, struct Driver *d, u32 terrainFlags)
{
	struct Instance *inst = thread->inst;
	MATRIX *m = &inst->matrix;
	u8 color = ((inst->flags & SPLIT_LINE) == 0) ? inst->unk50 : inst->unk51;
	u8 flags = ((terrainFlags & 8) == 0) ? 0 : 1;
	int sin = DECOMP_MATH_Sin(d->axisRotationX);
	int cos = DECOMP_MATH_Cos(d->axisRotationX);
	int lateralX = (sin * 15) >> 12;
	int lateralZ = (cos * 15) >> 12;
	int widthX = (cos * 10) >> 12;
	int widthZ = (-sin * 10) >> 12;
	VECTOR pos;

	color += 2;

	VehEmitter_SetRotTransMatrix(m);

	if ((d->actionsFlagSet & 0x800) != 0)
	{
		d->skidmarkEnableFlags |= 1;

		SVECTOR local = {-0x1e, 0, -0x14, 0};
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 0, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);

		d->skidmarkEnableFlags |= 2;

		local.vx = 0x1e;
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 1, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);
	}

	if ((d->actionsFlagSet & 0x1000) != 0)
	{
		d->skidmarkEnableFlags |= 4;

		SVECTOR local = {-0x1e, 0, 0x28, 0};
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 2, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);

		d->skidmarkEnableFlags |= 8;

		local.vx = 0x1e;
		VehEmitter_RotTransVec(&local, &pos);
		VehEmitter_WriteSkidmarkPair(d, 3, pos.vx - (lateralX >> 1), pos.vy, pos.vz - (lateralZ >> 1), lateralX, lateralZ, widthX, widthZ, color, flags);
	}
}

static void VehEmitter_MudSplash(struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	int count = ((d->actionsFlagSet & 2) == 0) ? 1 : 10;

	for (; count != 0; count--)
	{
		struct Particle *p = Particle_Init(0, gGT->iconGroup[0xd], &data.emSet_MudSplash[0]);

		if (p == NULL)
			continue;

		p->unk18 = d->instSelf->unk50;
		p->driverInst = d->instSelf;
		p->unk19 = d->driverID;

		p->axis[0].startVal += (int)p->axis[0].velocity * 0x10;
		p->axis[2].startVal += (int)p->axis[2].velocity * 0x10;
		p->axis[0].accel -= p->axis[0].velocity >> 4;
		p->axis[2].accel -= p->axis[2].velocity >> 4;
	}
}

static void VehEmitter_TerrainEffects(struct Thread *thread, struct Driver *d, struct Terrain *terrain, u32 terrainFlags, int absSpeedApprox)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = thread->inst;
	MATRIX *m = &inst->matrix;

	if (gGT->numPlyrCurrGame >= 2)
		return;

	int absSpeed = d->speed;
	if (absSpeed < 0)
		absSpeed = -absSpeed;

	if ((absSpeed > 0x500) && (d->currentTerrain == 0xe))
		VehEmitter_MudSplash(d);

	if (((terrainFlags & 0x40) != 0) && ((d->actionsFlagSet & 2) != 0) && (absSpeedApprox > 0x600))
	{
		int absJump = d->jumpHeightPrev;
		if (absJump < 0)
			absJump = -absJump;

		if (absJump > 0x1600)
		{
			VehEmitter_SetRotTransMatrix(m);
			VehEmitter_Sparks_Ground(d, &data.emSet_GroundSparks[0]);
		}
	}

	if (terrain->em_OddFrame != NULL)
	{
		struct ParticleEmitter *emSet = terrain->em_OddFrame;

		if ((terrain->em_EvenFrame != NULL) && ((gGT->timer & 1) != 0))
			emSet = terrain->em_EvenFrame;

		VehEmitter_SetRotTransMatrix(m);
		VehEmitter_Terrain_Ground(d, emSet);
	}

	int wallSound = 0x14;
	s32 engineVol = (s16)d->engineVol;

	if ((d->set_0xF0_OnWallRub == 0xf0) && (d->kartState != KS_MASK_GRABBED))
	{
		VehEmitter_SetRotTransMatrix(m);
		VehEmitter_Sparks_Wall(d, &data.emSet_WallSparks[0]);

		engineVol += 0x14;
		if (engineVol > 0xff)
			engineVol = 0xff;
	}
	else
	{
		if (d->set_0xF0_OnWallRub == 0)
			d->frameAgainstWall = 0;

		engineVol -= 0x14;
		if (engineVol < 0)
			engineVol = 0;

		if (engineVol == 0)
			wallSound = -1;
	}

	d->engineVol = (u16)engineVol;

	if (thread->modelIndex == DYNAMIC_PLAYER)
	{
		u32 flags = (u32)((s16)d->engineVol) << 16;

		if ((d->actionsFlagSet & 0x10000) != 0)
			flags |= 0x1008080;
		else
			flags |= 0x8080;

		OtherFX_RecycleNew((u32 *)&d->driverAudioPtrs[2], wallSound, flags);
	}
}

static void VehEmitter_TerrainAudioAndFeedback(struct Thread *thread, struct Driver *d, struct Terrain *terrain, u32 terrainFlags, int absSpeedApprox)
{
	if (thread->modelIndex != DYNAMIC_PLAYER)
		return;

	int soundID = -1;

	if (((d->actionsFlagSet & 1) != 0) && ((terrainFlags & 0x20) == 0))
		soundID = terrain->sound;

	int vol = DECOMP_VehCalc_MapToRange(absSpeedApprox, 0, 5000, 0, 200);
	int distort = DECOMP_VehCalc_MapToRange(absSpeedApprox, 0, 12000, 0x6c, 0xd2) << 8;

	if ((d->actionsFlagSet & 0x10000) != 0)
		distort |= 0x1000000;

	OtherFX_RecycleNew((u32 *)&d->driverAudioPtrs[1], soundID, ((u32)vol << 16) | (u32)distort | 0x80);

	if ((d->actionsFlagSet & 0x100000) == 0)
	{
		if (absSpeedApprox > 0x200)
		{
			DECOMP_GAMEPAD_ShockFreq(d, terrain->vibrationData[0], terrain->vibrationData[1]);
			DECOMP_GAMEPAD_ShockForce2(d, terrain->vibrationData[2], terrain->vibrationData[3]);
		}

		if ((d->actionsFlagSet & 2) != 0)
		{
			int absJump = d->jumpHeightPrev;
			if (absJump < 0)
				absJump = -absJump;

			if (absJump > 0x1600)
				DECOMP_GAMEPAD_ShockForce1(d, 3, 0xff);
		}
	}
}

static void VehEmitter_SkidmarkAudio(struct Thread *thread, struct Driver *d, struct Terrain *terrain, u32 terrainFlags, int absSpeedApprox)
{
	if (((d->actionsFlagSet & 1) == 0) || ((d->actionsFlagSet & 0x1800) == 0) || (absSpeedApprox < 0x201))
	{
		if (d->driverAudioPtrs[0] != NULL)
		{
			OtherFX_Stop1((int)d->driverAudioPtrs[0]);
			d->driverAudioPtrs[0] = NULL;
		}
		return;
	}

	if (thread->modelIndex == DYNAMIC_PLAYER)
	{
		int absTurn = d->simpTurnState;
		if (absTurn < 0)
			absTurn = -absTurn;

		int vol = DECOMP_VehCalc_MapToRange(absSpeedApprox, 2000, 12000, 0x14, 0xaa);
		int distort = DECOMP_VehCalc_MapToRange(absSpeedApprox, 2000, 12000, 0x92, 0x78);

		if (d->kartState == KS_DRIFTING)
		{
			int drift = d->unk3D4[0];
			if (drift < 0)
				drift = -drift;

			distort -= drift;
			if (distort < 0)
				distort = 0;
		}

		distort += absTurn;
		if (distort > 0x92)
			distort = 0x92;

		u32 flags = (u32)((vol + (absTurn >> 1)) << 16) | (u32)(distort << 8) | (0x80u - (((u32)(u8)d->simpTurnState << 24) >> 26));

		if ((d->actionsFlagSet & 0x10000) != 0)
			flags |= 0x1000000;

		OtherFX_RecycleNew((u32 *)&d->driverAudioPtrs[0], terrain->unk_0x30, flags);
	}

	VehEmitter_Skidmarks(thread, d, terrainFlags);
}

static int VehEmitter_ShouldSkipExhaust(struct Thread *thread, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

	if (thread->modelIndex == DYNAMIC_ROBOT_CAR)
	{
		if ((gGT->timer & 3) != (d->driverID & 3))
			return 1;
	}
	else
	{
		if (d->revEngineState == 2)
			return 1;

		u32 numPlyr = gGT->numPlyrCurrGame;

		if (numPlyr > 1)
		{
			if (((numPlyr != 2) || ((gGT->timer & 1) != d->driverID)) && ((gGT->timer & 3) != d->driverID))
				return 1;
		}

		if (d->unk381 == 0)
		{
			int meterLeft = d->turbo_MeterRoomLeft;

			if ((meterLeft < 0x81) || (((d->const_turboLowRoomWarning + 2) * 0x20) < meterLeft))
			{
				if (DECOMP_PROC_SearchForModel(thread->childThread, STATIC_TURBO_EFFECT) != NULL)
					return 1;
			}
		}
	}

	if (d->unk381 != 0)
		d->unk381--;

	return 0;
}

static void VehEmitter_ExhaustPair(struct Thread *thread, struct Driver *d)
{
	struct Instance *inst = thread->inst;
	MATRIX *m = &inst->matrix;
	SVECTOR local;
	VECTOR exhaustPos;
	VECTOR exhaustVel;

	gte_SetRotMatrix(m);

	local.vx = 0;
	local.vy = 0x400;
	local.vz = -0x400;
	local.pad = 0;
	VehEmitter_RotVec(&local, &exhaustVel);

	local.vx = (s16)((inst->scale[0] * 9) >> 3);
	local.vy = (s16)((inst->scale[1] * 7) >> 1);
	local.vz = (s16)((inst->scale[2] * -0x38) >> 4);
	VehEmitter_RotVec(&local, &exhaustPos);
	VehEmitter_Exhaust(d, &exhaustPos, &exhaustVel);

	local.vx = (s16)((inst->scale[0] * -0x12) >> 4);
	VehEmitter_RotVec(&local, &exhaustPos);
	VehEmitter_Exhaust(d, &exhaustPos, &exhaustVel);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059a18-0x8005ab24
void VehEmitter_DriverMain(struct Thread *thread, struct Driver *d)
{
	struct Terrain *terrain = d->terrainMeta1;
	u32 terrainFlags = terrain->flags;
	int absSpeedApprox = d->speedApprox;

	d->skidmarkEnableFlags = (d->skidmarkEnableFlags & 0xfffff) << 4;
	d->skidmarkFrameIndex = (d->skidmarkFrameIndex - 1) & 7;

	if (absSpeedApprox < 0)
		absSpeedApprox = -absSpeedApprox;

	VehEmitter_TerrainAudioAndFeedback(thread, d, terrain, terrainFlags, absSpeedApprox);
	VehEmitter_TerrainEffects(thread, d, terrain, terrainFlags, absSpeedApprox);

	if ((terrainFlags & 8) != 0)
		d->actionsFlagSet |= 0x1800;

	if ((d->matrixArray != 0) && (d->matrixArray < 4))
	{
		if (d->matrixArray == 1)
			d->actionsFlagSet |= 0x800;

		d->actionsFlagSet &= ~0x1000;
	}

	VehEmitter_SkidmarkAudio(thread, d, terrain, terrainFlags, absSpeedApprox);

	if (!VehEmitter_ShouldSkipExhaust(thread, d))
		VehEmitter_ExhaustPair(thread, d);

	if (d->burnTimer != 0)
	{
		d->alphaScaleBackup = 0x1000;
		d->instSelf->alphaScale = 0x1000;
	}

	if (d->invisibleTimer != 0)
		thread->inst->alphaScale = 0x1000;

	if ((d->kartState != KS_NORMAL) && (d->kartState != KS_DRIFTING))
		d->actionsFlagSet &= ~0x80000;

	if ((((u32)(u8)d->kartState - 4) < 2) || ((d->actionsFlagSet & 1) != 0))
	{
		DECOMP_GAMEPAD_JogCon2(d, 0x27, 0);

		if (d->unk3D4[0] == 0)
			return;

		int jogValue = ((sdata->gGT->timer & 3) == 0) ? 0x27 : 0xf0;
		DECOMP_GAMEPAD_JogCon2(d, jogValue, 0x100);
		return;
	}

	if (d->jump_LandingBoost < 0x80)
	{
		int jogValue = 0x12;

		if ((d->simpTurnState < 0) || ((jogValue = 0x22), (d->simpTurnState > 0)))
			DECOMP_GAMEPAD_JogCon1(d, jogValue, 0x20);
	}

	DECOMP_GAMEPAD_JogCon2(d, 0, 0);
}

void DECOMP_VehEmitter_DriverMain(struct Thread *thread, struct Driver *d)
{
	VehEmitter_DriverMain(thread, d);
}
