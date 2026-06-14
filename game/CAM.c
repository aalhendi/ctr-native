#include <common.h>

struct CAMSkyboxGlowGradient
{
	s16 pointFrom;
	s16 pointTo;
	u32 colorFrom;
	u32 colorTo;
};

static u32 CAM_SkyboxGlow_PrimAddr(const void *prim)
{
	return CtrGpu_PrimToOTLink24(prim);
}

static u32 CAM_SkyboxGlow_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static u32 CAM_SkyboxGlow_LerpColor(u32 from, u32 to, int ratio)
{
	int r = (u8)from + ((((int)(u8)to - (int)(u8)from) * ratio) >> 12);
	int g = (u8)(from >> 8) + ((((int)(u8)(to >> 8) - (int)(u8)(from >> 8)) * ratio) >> 12);
	int b = (u8)(from >> 16) + ((((int)(u8)(to >> 16) - (int)(u8)(from >> 16)) * ratio) >> 12);

	return ((u32)(u8)r) | ((u32)(u8)g << 8) | ((u32)(u8)b << 16);
}

static int CAM_SkyboxGlow_FixedRatio(int numerator, int denominator)
{
	return (numerator << 12) / denominator;
}

static int CAM_SkyboxGlow_ScreenX(int screenWidth, int ratio)
{
	return (screenWidth * ratio) >> 12;
}

static int CAM_SkyboxGlow_Div2TowardZero(int value)
{
	return (value + ((u32)value >> 31)) >> 1;
}

static int CAM_SkyboxGlow_CalcCenterY(struct PushBuffer *pb)
{
	int pitch = (pb->rot[0] - 0x800) * 0x78;
	int height = (s16)pb->rect.h;

	if (pitch < 0)
		pitch += 0x3ff;

	return (pitch >> 10) + CAM_SkyboxGlow_Div2TowardZero(height);
}

static int CAM_SkyboxGlow_CalcTilt(struct PushBuffer *pb)
{
	int sine = MATH_Sin(pb->rot[2]);
	int cosine = MATH_Cos(pb->rot[2]);
	int ratio;
	int shifted;

	if (cosine == 0)
		cosine = 1;

	ratio = (sine << 12) / cosine;
	shifted = (s32)((u32)ratio << 8);
	shifted >>= 12;

	return CAM_SkyboxGlow_Div2TowardZero(-shifted);
}

static void CAM_SkyboxGlow_EmitG3(struct PrimMem *primMem, u_long *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2)
{
	u32 *prim = primMem->curr;

	prim[1] = (color0 & 0xffffff) | 0x30000000;
	prim[2] = xy0;
	prim[3] = color1 & 0xffffff;
	prim[4] = xy1;
	prim[5] = color2 & 0xffffff;
	prim[6] = xy2;
	prim[0] = *ot | 0x06000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 7;
}

static void CAM_SkyboxGlow_EmitG4(struct PrimMem *primMem, u_long *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2, u32 color3, u32 xy3)
{
	u32 *prim = primMem->curr;

	prim[1] = (color0 & 0xffffff) | 0x38000000;
	prim[2] = xy0;
	prim[3] = color1 & 0xffffff;
	prim[4] = xy1;
	prim[5] = color2 & 0xffffff;
	prim[6] = xy2;
	prim[7] = color3 & 0xffffff;
	prim[8] = xy3;
	prim[0] = *ot | 0x08000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 9;
}

static void CAM_SkyboxGlow_EmitF3(struct PrimMem *primMem, u_long *ot, u32 color, u32 xy0, u32 xy1, u32 xy2)
{
	u32 *prim = primMem->curr;

	prim[1] = (color & 0xffffff) | 0x20000000;
	prim[2] = xy0;
	prim[3] = xy1;
	prim[4] = xy2;
	prim[0] = *ot | 0x04000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 5;
}

static void CAM_SkyboxGlow_EmitF4(struct PrimMem *primMem, u_long *ot, u32 color, u32 xy0, u32 xy1, u32 xy2, u32 xy3)
{
	u32 *prim = primMem->curr;

	prim[1] = (color & 0xffffff) | 0x28000000;
	prim[2] = xy0;
	prim[3] = xy1;
	prim[4] = xy2;
	prim[5] = xy3;
	prim[0] = *ot | 0x05000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 6;
}

static int CAM_SkyboxGlow_HasClearGradient(int gradientIndex)
{
	return gradientIndex == 0 && sdata->gGT->level1->clearColor[2].enable != 0;
}

static u32 CAM_SkyboxGlow_ClearGradientColor(void)
{
	return *(u32 *)&sdata->gGT->level1->clearColor[2].rgb[0] & 0xffffff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800175cc-0x8001861c
void CAM_SkyboxGlow(s16 *param_1, struct PushBuffer *pb, struct PrimMem *primMem, u_long *ptrOT)
{
	struct CAMSkyboxGlowGradient *grad = (struct CAMSkyboxGlowGradient *)param_1;
	int tilt = CAM_SkyboxGlow_CalcTilt(pb);
	int centerY1 = CAM_SkyboxGlow_CalcCenterY(pb);
	int centerY2 = CAM_SkyboxGlow_CalcCenterY(pb);
	int screenWidth = (s16)pb->rect.w;
	u32 screenWidthXY = (u16)screenWidth;

	for (int i = 0; i < 3; i++, grad++)
	{
		int toLeft = centerY2 - grad->pointTo - tilt;
		int toRight = centerY2 - grad->pointTo + tilt;
		int fromLeft = centerY1 - grad->pointFrom - tilt;
		int fromRight = centerY1 - grad->pointFrom + tilt;
		u32 colorFrom = grad->colorFrom & 0xffffff;
		u32 colorTo = grad->colorTo & 0xffffff;
		u32 mask = 0;

		if (toLeft >= 0)
			mask |= 1;
		if (toRight >= 0)
			mask |= 2;
		if (fromLeft >= 0)
			mask |= 4;
		if (fromRight >= 0)
			mask |= 8;

		switch (mask)
		{
		case 1:
		{
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, rightX);
			break;
		}
		case 2:
		{
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			int leftRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);
			u32 leftX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, leftRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorTo, leftX, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 3:
		{
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 5:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft),
			                      colorTo, toX);

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			break;
		}
		case 7:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, rightColor, screenWidthXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), rightColor, screenWidthXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			break;
		}
		case 10:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, fromX, colorFrom, fromRightXY, colorTo, toX, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			break;
		}
		case 11:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorFrom, fromX, colorFrom, fromRightXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, colorFrom, fromRightXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			break;
		}
		case 15:
		{
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromRightXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF4(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), 0, screenWidthXY, CAM_SkyboxGlow_PackXY(0, fromLeft), fromRightXY);
			break;
		}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001861c-0x80018818
void CAM_ClearScreen(struct GameTracker *gGT)
{
	char numPlyr;
	char swap;
	s16 x;
	s16 y;
	s16 w;
	s16 h;
	struct DB *backDB;
	int iVar5;
	int iVar7;
	struct PushBuffer *pb;
	TILE *tile;
	int loop;
	struct Level *level1;
	u_long *endOT;

	backDB = gGT->backBuffer;
	tile = backDB->primMem.curr;
	numPlyr = gGT->numPlyrCurrGame;
	swap = gGT->swapchainIndex;
	level1 = gGT->level1;

	for (loop = 0; loop < numPlyr; loop++)
	{
		// pointer to pushBuffer struct
		pb = &gGT->pushBuffer[loop];
		endOT = &pb->ptrOT[0x3FF];

		x = pb->rect.x;
		y = pb->rect.y + swap * 0x128;
		w = pb->rect.w;
		h = pb->rect.h;

		// pushBuffer rotation
		// cam up/down will change where the line splits.
		// At 0x800, camera looks straight, and line is perfectly midpoint
		iVar5 = ((int)pb->rot[0] - 0x800 >> 3) + (h >> 1);

		// if splitline is above top of screen,
		// camera looks far down and only sees bottom half
		if (iVar5 < 0)
		{
			// top half has zero height
			iVar5 = 0;
		}

		// if splitline is below bottom of the screen,
		// top quad height is window sizeY, bottom quad is zero height
		iVar7 = h;

		// if splitline is below top of screen
		if (iVar5 < h)
		{
			// set midpoint accordingly
			iVar7 = iVar5;
		}

		// if top-half clear color exists,
		// and if splitline is below top of screen (so top quad exists)
		if ((level1->clearColor[0].enable != 0) && (0 < iVar7))
		{
			tile->x0 = x;
			tile->y0 = y;
			tile->w = w;
			tile->h = iVar7;

			*(int *)&tile->r0 = *(int *)&level1->clearColor[0].rgb[0];
			tile->code = 0x2;

			tile->tag = *(u32 *)endOT | 0x3000000;
			*(u32 *)endOT = CtrGpu_PrimToOTLink24(tile);

			tile++;
		}

		// if bottom-half clear color exists,
		// and if splitline is above bottom of screen (so bottom quad exists)
		if ((level1->clearColor[1].enable != 0) && (iVar7 < h))
		{
			tile->x0 = x;
			tile->y0 = y + iVar7;
			tile->w = w;
			tile->h = h - iVar7;

			*(int *)&tile->r0 = *(int *)&level1->clearColor[1].rgb[0];
			tile->code = 0x2;

			tile->tag = *(u32 *)endOT | 0x3000000;
			*(u32 *)endOT = CtrGpu_PrimToOTLink24(tile);

			tile++;
		}
	}

	backDB->primMem.curr = tile;

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018818-0x800188a8
void CAM_Init(struct CameraDC *cDC, int cameraID, struct Driver *d, struct PushBuffer *pb)
{
// Naughty Dog debug printf
#if BUILD == SepReview
	printf("camera init\n");
#endif

	PROC_BirthWithObject(0x30f, CAM_ThTick, sdata->s_camera, NULL)->inst = (struct Instance *)cDC;

	memset(cDC, 0, sizeof(struct CameraDC));

	// needed or L2 breaks
	cDC->cameraID = cameraID;

	cDC->driverToFollow = d;
	cDC->pushBuffer = pb;

	// dont set cameraMode to zero,
	// memset makes it already zero

	cDC->flags |= 8;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018b18-0x80018ba0
int CAM_Path_GetNumPoints(void)
{
	struct GameTracker *gGT;
	struct Level *level1;
	struct SpawnType1 *ptrSpawnType1;
	s16 *introCam;
	u16 uVar4;

	uVar4 = 0;

	gGT = sdata->gGT;
	level1 = gGT->level1;
	if (level1 == NULL)
		return 0;

	ptrSpawnType1 = level1->ptrSpawnType1;
	if (ptrSpawnType1->count < 3)
		return 0;

	void **ptrs = ST1_GETPOINTERS(ptrSpawnType1);
	introCam = ptrs[ST1_CAMERA_PATH];
	if (introCam == NULL)
		return 0;

	while (1)
	{
		if (introCam[0] == 0)
			break;
		uVar4 += introCam[0];
		introCam += introCam[0] * 6 + 2;
	}

	return (s16)uVar4;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ba0-0x80018d20
u8 CAM_Path_Move(int frameIndex, s16 *position, s16 *rotation, s16 *getPath)
{
	s16 frame;
	s16 numPos;
	u16 pathNumNode;
	u16 pathID;
	s16 *ptrCam;
	s16 *move;

	frame = (s16)frameIndex;

	// get number of position on track
	numPos = CAM_Path_GetNumPoints();

	if (frame < 0)
		return 0;
	if (frame >= numPos)
		return 0;

	void **ptrs = ST1_GETPOINTERS(sdata->gGT->level1->ptrSpawnType1);
	ptrCam = ptrs[ST1_CAMERA_PATH];

	pathNumNode = (u16)ptrCam[0];
	pathID = (u16)ptrCam[1];
	move = ptrCam + 2;

	while ((s16)pathNumNode <= frame)
	{
		frame = (s16)(frame - (s16)pathNumNode);
		move = move + (s16)pathNumNode * 6;
		pathNumNode = (u16)move[0];
		pathID = (u16)move[1];
		move = move + 2;
	}

	// advance pointer to pos+rot
	move += (int)frame * 6;

	*getPath = pathID;

	// position of frame
	position[0] = move[0];
	position[1] = move[1];
	position[2] = move[2];

	// rotation of frame
	rotation[0] = ((s16)move[3] >> 4) + 0x800U & 0xfff;
	rotation[1] = (u16)move[4] >> 4;
	rotation[2] = (u16)move[5] >> 4;
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018d20-0x80018d9c
void CAM_StartOfRace(struct CameraDC *cDC)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level1 = gGT->level1;

	// if fly-in camera data exists and there is only one screen
	int hasFlyInCamera = (2 < level1->cnt_restart_points);

	if (hasFlyInCamera)
	{
		int flyInData = (int)level1->ptr_restart_points;
		cDC->unk94 = 0;
		cDC->unk8C = 0;

		// make transition to driver last one second
		cDC->frameCounterTransition = 0x1E;
		cDC->desiredRot[6] = 0;

		// when camera reaches player, be zoomed in
		cDC->cameraMode = 0;
		cDC->unk88 = (void *)(flyInData + 0x18);

		// if 1 or less screens
		// set fly-in to last 165 frames, (5.25 seconds)
		cDC->unk8E = 0xA5;
		if (gGT->numPlyrCurrGame > 1)
		{
			// set animation to last one frame
			cDC->unk8E = 1;
		}
	}

	cDC->cameraMode = 0;

	// remove bit & 0x4 (battle end of race) and 0x1000 (idk)
	cDC->flags &= 0xFFFFEFFB;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018d9c-0x80018e38.
void CAM_EndOfRace_Battle(struct CameraDC *cDC, struct Driver *d)
{
	int dx;
	int dz;
	int height;
	struct PushBuffer *pb;

	// This is NOT TransitionTo,
	// spin360's spinSpeed (missing union)
	// and spinHeight, spinDistance, etc
	height = data.Spin360_heightOffset_cameraPos[sdata->gGT->numPlyrCurrGame];
	cDC->transitionTo.pos[0] = 0xffe5;
	cDC->transitionTo.pos[1] = height;
	cDC->transitionTo.pos[2] = 0xc0;

	// use camera that spins around player
	cDC->flags |= 4;

	// transition should take two seconds
	cDC->unk8C = 0;
	cDC->unk8E = 60;
	cDC->frameCounterTransition = 60;

	// direction to face
	pb = cDC->pushBuffer;
	dx = pb->pos[0] - (d->posCurr.x >> 8);
	dz = pb->pos[2] - (d->posCurr.z >> 8);
	cDC->unk90 = ratan2(dx, dz);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018e38-0x80018ec0.
void CAM_EndOfRace(struct CameraDC *cDC, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

#if BUILD > SepReview

	// If not in Battle Mode and track path points exist and game is on 1P or 2P mode
	if (((gGT->gameMode1 & BATTLE_MODE) == 0) && (1 < gGT->level1->ptrSpawnType1->count) && (gGT->numPlyrCurrGame < 3))
	{
		// Activate end-of-race cDC flag in CameraDC struct
		cDC->flags |= 0x20;
	}
	else
	{
		// Call function to initialize end-of-race cDC for Battle Mode
		CAM_EndOfRace_Battle(cDC, d);
	}
	return;

#else
	// this is SepReview's more error-prone version of the function

	if (gGT->level1->ptrSpawnType1->count < 2 || gGT->numPlyrCurrGame > 2)
		CAM_EndOfRace_Battle(cDC, d);
	else
		cDC->flags |= 0x20;

#endif
}

static s32 CAM_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018fec-0x80019128
void CAM_ProcessTransition(s16 *currPos, s16 *currRot, s16 *startPos, s16 *startRot, s16 *endPos, s16 *endRot, int frame)
{
	int deltaRot;

	currPos[0] = startPos[0] + (s16)(CAM_MulLo((int)endPos[0] - (int)startPos[0], frame) >> 0xc);
	currPos[1] = startPos[1] + (s16)(CAM_MulLo((int)endPos[1] - (int)startPos[1], frame) >> 0xc);
	currPos[2] = startPos[2] + (s16)(CAM_MulLo((int)endPos[2] - (int)startPos[2], frame) >> 0xc);

	deltaRot = (int)endRot[0] - (int)startRot[0] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}

	currRot[0] = startRot[0] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	deltaRot = (int)endRot[1] - (int)startRot[1] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot[1] = startRot[1] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	deltaRot = (int)endRot[2] - (int)startRot[2] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot[2] = startRot[2] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	return;
}

_Static_assert(sizeof(struct QuadBlock) == 0x5c);

static void CAM_FindClosestQuadblock_SetScratchpadWord(s16 *scratchpad, int halfwordIndex, int value)
{
	*(int *)(scratchpad + halfwordIndex) = value;
}

static int CAM_FindClosestQuadblock_GetScratchpadWord(s16 *scratchpad, int halfwordIndex)
{
	return *(int *)(scratchpad + halfwordIndex);
}

static void CAM_FindClosestQuadblock_SetCameraHitFlag(struct CameraDC *cDC, int value)
{
	*(int *)((u8 *)cDC + 0x3c) = value;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800188a8-0x80018b18
void CAM_FindClosestQuadblock(s16 *scratchpad, struct CameraDC *cDC, struct Driver *d, s16 *pos)
{
	struct GameTracker *gGT;
	struct mesh_info *meshInfo;
	struct QuadBlock *quad;

	(void)d;

	scratchpad[8] = pos[0];
	scratchpad[9] = pos[2];
	scratchpad[10] = pos[4];

	scratchpad[0] = pos[0];
	scratchpad[1] = (s16)((u16)pos[2] - 0x800);
	scratchpad[14] = scratchpad[0];
	scratchpad[2] = pos[4];
	scratchpad[9] = (s16)((u16)scratchpad[9] + 0x100);
	scratchpad[15] = scratchpad[1];
	scratchpad[16] = pos[4];

	scratchpad[0x18] = scratchpad[0] < scratchpad[8] ? scratchpad[0] : scratchpad[8];
	scratchpad[0x19] = scratchpad[1] < scratchpad[9] ? scratchpad[1] : scratchpad[9];
	scratchpad[0x1a] = scratchpad[2] < scratchpad[10] ? scratchpad[2] : scratchpad[10];
	scratchpad[0x1b] = scratchpad[8] < scratchpad[0] ? scratchpad[0] : scratchpad[8];
	scratchpad[0x1c] = scratchpad[9] < scratchpad[1] ? scratchpad[1] : scratchpad[9];
	scratchpad[0x1d] = scratchpad[10] < scratchpad[2] ? scratchpad[2] : scratchpad[10];

	CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x12, 0x800);
	CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x14, 0);
	scratchpad[0x1f] = 0;
	scratchpad[0x1e] = 0;
	scratchpad[0x11] = 0;
	CAM_FindClosestQuadblock_SetCameraHitFlag(cDC, 0);

	gGT = sdata->gGT;

	if ((gGT->level1 == NULL) || (gGT->level1->ptr_mesh_info == NULL) || (gGT->level1->ptr_mesh_info->bspRoot == NULL))
	{
		CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x16, 0);
		return;
	}

	meshInfo = gGT->level1->ptr_mesh_info;
	CAM_FindClosestQuadblock_SetScratchpadWord(scratchpad, 0x16, (int)meshInfo);

	if (cDC->ptrQuadBlock != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(cDC->ptrQuadBlock, (struct ScratchpadStruct *)scratchpad);
	}

	if (scratchpad[0x1f] == 0)
	{
		COLL_SearchBSP_CallbackPARAM(meshInfo->bspRoot, (struct BoundingBox *)(scratchpad + 0x18), COLL_FIXED_BSPLEAF_TestQuadblocks,
		                             (struct ScratchpadStruct *)scratchpad);
	}

	if (scratchpad[0x1f] == 0)
	{
		gGT->unk1cac[0] = -1;
		return;
	}

	CAM_FindClosestQuadblock_SetCameraHitFlag(cDC, 1);

	quad = (struct QuadBlock *)CAM_FindClosestQuadblock_GetScratchpadWord(scratchpad, 0x40);
	cDC->ptrQuadBlock = quad;
	gGT->unk1cac[0] = quad - meshInfo->ptrQuadBlockArray;
}

static void CAM_StartLine_FlyIn_FixY_SetPoint(u32 point[2], s16 x, s16 y, s16 z)
{
	point[0] = (u16)x | ((u32)(u16)y << 0x10);
	point[1] = (u16)z;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018ec0-0x80018fec.
void CAM_StartLine_FlyIn_FixY(s16 *posRot)
{
	struct ScratchpadStruct *sps = &sdata->scratchpadStruct;
	s16 pos[3];
	u32 posTop[2];
	u32 posBottom[2];
	int i;

	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	pos[0] = posRot[0];
	pos[1] = posRot[1];
	pos[2] = posRot[2];

	for (i = 0; i < 8; i++)
	{
		s16 probeOffset = i * 0x400;

		CAM_StartLine_FlyIn_FixY_SetPoint(posTop, pos[0], pos[1] - (probeOffset + 0x400), pos[2]);
		CAM_StartLine_FlyIn_FixY_SetPoint(posBottom, pos[0], pos[1] - (probeOffset - 0x100), pos[2]);

		COLL_SearchBSP_CallbackQUADBLK(posTop, posBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			pos[0] = sps->Union.QuadBlockColl.hitPos[0];
			pos[1] = sps->Union.QuadBlockColl.hitPos[1];
			pos[2] = sps->Union.QuadBlockColl.hitPos[2];
			break;
		}
	}

	posRot[1] = pos[1];
}

static s32 CAM_FollowDriver_AngleAxis_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static s32 CAM_FollowDriver_AngleAxis_Lerp256(s32 current, s32 previous, s32 ratio)
{
	return (CAM_FollowDriver_AngleAxis_MulLo(0x100 - ratio, current) + CAM_FollowDriver_AngleAxis_MulLo(ratio, previous)) >> 8;
}

static void CAM_FollowDriver_AngleAxis_LoadGteMatrix(MATRIX *axisMatrix, struct Driver *d)
{
	gte_SetRotMatrix(axisMatrix);
	gte_SetTransVector(d->instSelf->matrix.t);
}

static void CAM_FollowDriver_AngleAxis_TransformOffset(const s16 *offset, VECTOR *out)
{
	gte_ldv0((SVECTOR *)offset);
	gte_rtv0tr();
	gte_stlvnl(out);
}

void CAM_FollowDriver_AngleAxis(struct CameraDC *cDC, struct Driver *d, int scratchpad, s16 *pushBufferPos, s16 *pushBufferRot)
{
	MATRIX *axisMatrix = (MATRIX *)(scratchpad + 0x220);
	VECTOR *eye = (VECTOR *)(scratchpad + 0x240);
	VECTOR lookAt;
	int ratio;
	int dx;
	int dy;
	int dz;
	int distanceXZ;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019128-0x800194c8.
	if (cDC->cameraMode == 0xe)
		VehPhysForce_RotAxisAngle(axisMatrix, d->AxisAngle2_normalVec, d->angle);
	else
		VehPhysForce_RotAxisAngle(axisMatrix, d->AxisAngle2_normalVec, d->rotCurr.y);

	CAM_FollowDriver_AngleAxis_LoadGteMatrix(axisMatrix, d);
	CAM_FollowDriver_AngleAxis_TransformOffset(cDC->driverOffset_CamEyePos, eye);
	CAM_FollowDriver_AngleAxis_TransformOffset(cDC->driverOffset_CamLookAtPos, &lookAt);

	if ((cDC->flags & 8) != 0)
	{
		cDC->unkTriplet3[0] = lookAt.vx;
		cDC->unkTriplet3[1] = lookAt.vy;
		cDC->unkTriplet3[2] = lookAt.vz;
	}
	else
	{
		ratio = cDC->unk7A;

		eye->vx = CAM_FollowDriver_AngleAxis_Lerp256(eye->vx, pushBufferPos[0], ratio);
		eye->vy = CAM_FollowDriver_AngleAxis_Lerp256(eye->vy, pushBufferPos[1], ratio);
		eye->vz = CAM_FollowDriver_AngleAxis_Lerp256(eye->vz, pushBufferPos[2], ratio);

		cDC->unkTriplet3[0] = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vx, cDC->unkTriplet3[0], ratio);
		cDC->unkTriplet3[1] = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vy, cDC->unkTriplet3[1], ratio);
		cDC->unkTriplet3[2] = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.vz, cDC->unkTriplet3[2], ratio);
	}

	dx = eye->vx - cDC->unkTriplet3[0];
	dy = eye->vy - cDC->unkTriplet3[1];
	dz = eye->vz - cDC->unkTriplet3[2];

	*(int *)(scratchpad + 0x24c) = dx;
	*(int *)(scratchpad + 0x250) = dy;
	*(int *)(scratchpad + 0x254) = dz;

	pushBufferRot[1] = (s16)ratan2(dx, dz);
	distanceXZ = SquareRoot0_stub(CAM_FollowDriver_AngleAxis_MulLo(dx, dx) + CAM_FollowDriver_AngleAxis_MulLo(dz, dz));
	pushBufferRot[0] = 0x800 - (s16)ratan2(dy, distanceXZ);
	pushBufferRot[2] = 0;

	pushBufferPos[0] = (s16)eye->vx;
	pushBufferPos[1] = (s16)eye->vy;
	pushBufferPos[2] = (s16)eye->vz;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800194c8-0x800198f8.
void CAM_StartLine_FlyIn(struct FlyInData *flyInData, s16 maxFrames, int frame, s16 *desiredPos, s16 *desiredRot)
{
	struct Level *lev = sdata->gGT->level1;
	int frameIndex = (frame << 0x10) >> 4;
	int frameRatio = frameIndex / maxFrames;
	int countEnd = flyInData->frameCount1;
	s16 count = flyInData->frameCount2;
	SVECTOR local_78;
	SVECTOR local_70;
	SVECTOR rot;
	MATRIX matrix;
	VECTOR transformed;
	s32 flags[2];
	s16 *pathEnd;
	s16 *pathStart;
	int pathRatioEnd;

	if (count < countEnd)
		count = countEnd;

	s16 pathIndex = (s16)(count * frameRatio >> 0xc);
	if (pathIndex < countEnd - 1)
	{
		pathEnd = (s16 *)(flyInData->ptrEnd + pathIndex * 6);
		pathRatioEnd = frameRatio;
	}
	else
	{
		pathEnd = (s16 *)(flyInData->ptrEnd + countEnd * 6 - 0xc);
		pathRatioEnd = 0;
	}

	if (pathIndex < flyInData->frameCount2 - 1)
	{
		pathStart = (s16 *)(flyInData->ptrStart + pathIndex * 6);
	}
	else
	{
		pathStart = (s16 *)(flyInData->ptrStart + flyInData->frameCount2 * 6 - 0xc);
		frameRatio = 0;
	}

	int ratio = count * pathRatioEnd & 0xfff;
	local_78.vx = pathEnd[0] + (s16)(((pathEnd[3] - pathEnd[0]) * ratio) >> 0xc);
	local_78.vy = pathEnd[1] + (s16)(((pathEnd[4] - pathEnd[1]) * ratio) >> 0xc);
	local_78.vz = pathEnd[2] + (s16)(((pathEnd[5] - pathEnd[2]) * ratio) >> 0xc);

	ratio = count * frameRatio & 0xfff;
	local_70.vx = pathStart[0] + (s16)(((pathStart[3] - pathStart[0]) * ratio) >> 0xc);
	local_70.vy = pathStart[1] + (s16)(((pathStart[4] - pathStart[1]) * ratio) >> 0xc) - 0x60;
	local_70.vz = pathStart[2] + (s16)(((pathStart[5] - pathStart[2]) * ratio) >> 0xc);

	rot.vx = lev->DriverSpawn[0].rot[0];
	rot.vy = lev->DriverSpawn[0].rot[1] + 0x400;
	rot.vz = lev->DriverSpawn[0].rot[2];

	ConvertRotToMatrix(&matrix, (s16 *)&rot);

	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[1].pos[0]);
	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[2].pos[0]);
	CAM_StartLine_FlyIn_FixY(&lev->DriverSpawn[5].pos[0]);

	matrix.t[0] = lev->DriverSpawn[1].pos[0] + (lev->DriverSpawn[2].pos[0] - lev->DriverSpawn[1].pos[0]) / 2;
	matrix.t[1] = lev->DriverSpawn[1].pos[1] + (lev->DriverSpawn[2].pos[1] - lev->DriverSpawn[1].pos[1]) / 2 + 0x40;
	matrix.t[2] = lev->DriverSpawn[1].pos[2] + (lev->DriverSpawn[2].pos[2] - lev->DriverSpawn[1].pos[2]) / 2;

	SetRotMatrix(&matrix);
	SetTransMatrix(&matrix);

	RotTrans(&local_78, &transformed, (long *)flags);
	desiredPos[0] = (s16)transformed.vx;
	desiredPos[1] = (s16)transformed.vy;
	desiredPos[2] = (s16)transformed.vz;

	RotTrans(&local_70, &transformed, (long *)flags);

	s16 deltaX = desiredPos[0] - (s16)transformed.vx;
	s16 deltaY = desiredPos[1] - (s16)transformed.vy;
	s16 deltaZ = desiredPos[2] - (s16)transformed.vz;

	desiredRot[1] = (s16)ratan2(deltaX, deltaZ);
	desiredRot[0] = 0x800 - (s16)ratan2(deltaY, SquareRoot0(deltaX * deltaX + deltaZ * deltaZ));
	desiredRot[2] = 0;
}

static s32 CAM_FollowDriver_TrackPath_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static struct CheckpointNode *CAM_FollowDriver_TrackPath_GetNode(struct CameraDC *cDC, struct CheckpointNode *node, int speed)
{
	struct CheckpointNode *nodes = sdata->gGT->level1->ptr_restart_points;
	u8 nodeIndex;

	if (speed > 0)
	{
		nodeIndex = node->nextIndex_backward;
		if ((cDC->flags & 0x100) != 0 && node->nextIndex_right != 0xff)
			nodeIndex = node->nextIndex_right;
	}
	else
	{
		nodeIndex = node->nextIndex_forward;
		if ((cDC->flags & 0x100) != 0 && node->nextIndex_left != 0xff)
			nodeIndex = node->nextIndex_left;
	}

	return &nodes[nodeIndex];
}

static int CAM_FollowDriver_TrackPath_Length(struct CheckpointNode *from, struct CheckpointNode *to, int *dx, int *dy, int *dz)
{
	*dx = to->pos[0] - from->pos[0];
	*dy = to->pos[1] - from->pos[1];
	*dz = to->pos[2] - from->pos[2];

	return SquareRoot0_stub(CAM_FollowDriver_TrackPath_MulLo(*dx, *dx) + CAM_FollowDriver_TrackPath_MulLo(*dy, *dy) +
	                        CAM_FollowDriver_TrackPath_MulLo(*dz, *dz));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800198f8-0x80019e7c.
u32 CAM_FollowDriver_TrackPath(struct CameraDC *cDC, s16 *pos, int speed, int update)
{
	struct CheckpointNode *curr = (struct CheckpointNode *)cDC->unk88;
	struct CheckpointNode *next = CAM_FollowDriver_TrackPath_GetNode(cDC, curr, speed);
	struct CheckpointNode *angleNext;
	int pathProgress;
	int segmentLength;
	int ratio;
	int dx;
	int dy;
	int dz;
	int yaw;
	int nextYaw;
	int yawDelta;

	segmentLength = CAM_FollowDriver_TrackPath_Length(curr, next, &dx, &dy, &dz);

	if ((sdata->gGT->gameMode1 & 0xf) != 0)
		pathProgress = 0;
	else if (speed > 0)
		pathProgress = cDC->unk94 + speed;
	else
		pathProgress = cDC->unk94 - speed;

	while (pathProgress >= segmentLength)
	{
		pathProgress -= segmentLength;
		curr = next;
		next = CAM_FollowDriver_TrackPath_GetNode(cDC, curr, speed);
		segmentLength = CAM_FollowDriver_TrackPath_Length(curr, next, &dx, &dy, &dz);
	}

	if (update)
	{
		cDC->unk94 = pathProgress;
		cDC->unk88 = curr;
	}

	if (segmentLength != 0)
		ratio = (pathProgress << 12) / segmentLength;
	else
		ratio = 0;

	pos[0] = curr->pos[0] + (s16)((CAM_FollowDriver_TrackPath_MulLo(dx, ratio)) >> 12);
	pos[1] = curr->pos[1] + (s16)((CAM_FollowDriver_TrackPath_MulLo(dy, ratio)) >> 12) + 0x80;
	pos[2] = curr->pos[2] + (s16)((CAM_FollowDriver_TrackPath_MulLo(dz, ratio)) >> 12);

	yaw = (s16)ratan2(dx, dz) + 0x800;
	angleNext = CAM_FollowDriver_TrackPath_GetNode(cDC, next, speed);
	nextYaw = (s16)ratan2(angleNext->pos[0] - next->pos[0], angleNext->pos[2] - next->pos[2]) + 0x800;

	yawDelta = (nextYaw - yaw) & 0xfff;
	if (yawDelta >= 0x800)
		yawDelta -= 0x1000;

	return (yaw + (CAM_FollowDriver_TrackPath_MulLo(yawDelta, ratio) >> 12)) & 0xfff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019e7c-0x80019f58
void CAM_LookAtPosition(int scratchpad, int *positions, s16 *desiredPos, s16 *desiredRot)
{
	int dirX = desiredPos[0] - (positions[0] >> 8);
	int dirY = desiredPos[1] - ((positions[1] >> 8) + data.Spin360_heightOffset_driverPos[sdata->gGT->numPlyrCurrGame]);
	int dirZ = desiredPos[2] - (positions[2] >> 8);

	// Store dirX, dirY, dirZ in scratchpad
	*(int *)(scratchpad + 0x24c) = dirX;
	*(int *)(scratchpad + 0x250) = dirY;
	*(int *)(scratchpad + 0x254) = dirZ;

	int distance = SquareRoot0_stub(CAM_MulLo(dirX, dirX) + CAM_MulLo(dirZ, dirZ));

	// rotations
	desiredRot[0] = 0x800 - (s16)ratan2(dirY, distance);
	desiredRot[1] = (s16)ratan2(dirX, dirZ);
	desiredRot[2] = 0;

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019f58-0x8001a054
void CAM_FollowDriver_Spin360(struct CameraDC *cDC, int param_2, struct Driver *d, s16 *desiredPos, s16 *desiredRot)
{
	int ratio;

	// === Union Missing ===
	// Not really "transitionTo" but the variables
	// are shared with other camera modes, therefore
	// need a union with proper names for each mode

	// rotate other way for odd number
	if ((d->driverID & 1) != 0)
	{
		cDC->unk90 -= cDC->transitionTo.pos[0];
	}
	else
	{
		// rotate one way
		cDC->unk90 += cDC->transitionTo.pos[0];
	}

	int angle = cDC->unk90;
	ratio = MATH_Sin(angle);
	desiredPos[0] = (s16)(d->posCurr.x >> 8) + (s16)(CAM_MulLo(ratio, cDC->transitionTo.pos[2]) >> 0xc);

	ratio = MATH_Cos(angle);
	desiredPos[2] = (s16)(d->posCurr.z >> 8) + (s16)(CAM_MulLo(ratio, cDC->transitionTo.pos[2]) >> 0xc);

	desiredPos[1] = (s16)(d->posCurr.y >> 8) + cDC->transitionTo.pos[1];

	CAM_LookAtPosition(param_2, (int *)&d->posCurr.x, desiredPos, desiredRot);
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001a054-0x8001a0bc
void CAM_SetDesiredPosRot(struct CameraDC *cDC, s16 *pos, s16 *rot)
{
	int i;

	// save desired pos and rot
	for (i = 0; i < 3; i++)
	{
		cDC->transitionTo.pos[i] = pos[i];
		cDC->transitionTo.rot[i] = rot[i];
	}

	// 1 second, 30 frames
	cDC->frameCounterTransition = 0x1e;

	cDC->unk8E = 0;
	cDC->unk8C = 0x1000;

	cDC->flags |= 0x200;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001a0bc-0x8001b254.
void CAM_FollowDriver_Normal(struct CameraDC *cDC, struct Driver *d, s16 *pushBuffer, int scratchpad, struct ZoomData *zoom)
{
	struct PushBuffer *pb = (struct PushBuffer *)pushBuffer;
	struct GameTracker *gGT = sdata->gGT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[d->driverID];
	char state;
	s16 uVar8;
	u16 uVar9;
	s16 sVar10;
	u32 backupFlags;
	int x;
	s32 x_00;
	u32 uVar11;
	int iVar12;
	SVECTOR *psVar12;
	u32 uVar13;
	int iVar14;
	s16 local_40[3];
	s16 local_38[3];
	struct FlyInData flyInData;

	// backup flags
	backupFlags = cDC->flags;

	// disable Reverse Cam flag,
	// assuming you dont hold R2
	cDC->flags &= 0xfffeffff;

	if (
	    // If this is human and not AI
	    ((d->actionsFlagSet & 0x100000) == 0) &&

	    // If not drawing intro-race cutscene
	    ((gGT->gameMode1 & START_OF_RACE) == 0) &&

	    // If you are holding R2
	    ((pad->buttonsHeldCurrFrame & 0x200) != 0))
	{
		// Reverse the camera
		cDC->flags |= 0x10000;
	}

	// if camera just changed direction
	// (either forward to reverse, or reverse to forward)
	if (backupFlags != cDC->flags)
	{
		// set flag that cam just changed this frame
		cDC->flags |= 8;
	}

	// 0 = forwards
	// 0x800 = backwards
	sVar10 = ((cDC->flags & 0x10000) != 0) * 0x800;

	// if camera angle was not just changed
	if ((cDC->flags & 8) == 0)
	{
		// absolute value driver speed
		x = (int)d->speedApprox;

		if (x < 0)
		{
			x = -x;
		}

		// driver speed slower than camera speed
		if (x < cDC->cameraMoveSpeed)
		{
			// transition inward
			uVar13 = (u32)zoom->percentage2;
			uVar11 = (u32)zoom->percentage1;
		}

		// driver speed faster than camera
		else
		{
			// transition outward
			uVar13 = (u32)zoom->percentage1;
			uVar11 = (u32)zoom->percentage1;
		}
		cDC->cameraMoveSpeed = (int)(uVar11 * cDC->cameraMoveSpeed + (0x100 - uVar13) * x) >> 8;
	}

	// if camera angle changed
	else
	{
		// absolute value driver speed
		x = (int)d->speedApprox;
		if (x < 0)
			x = -x;

		cDC->cameraMoveSpeed = x;
	}

	uVar8 = 0;
	if (gGT->numPlyrCurrGame != 2)
	{
		uVar8 = 0xff9c;
	}

	*(s16 *)(scratchpad + 0x20c) = uVar8;

	// 0x20e
	// camera RotY
	*(u16 *)(scratchpad + 0x20e) = (d->rotCurr.w + d->angle + 0x800 + sVar10) & 0xfff;

	*(s16 *)(scratchpad + 0x210) = cDC->desiredRot[0] * -2;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(scratchpad + 0x220, scratchpad + 0x20c);

	if (((cDC->flags & 0x80) != 0) && (x = (int)((u32)d->fireSpeedCap << 0x10) >> 0x14, *(int *)&cDC->unk_b8[4] < x))
	{
		*(int *)&cDC->unk_b8[4] = x;
	}
	*(s16 *)(scratchpad + 0x20c) = 0;
	*(s16 *)(scratchpad + 0x20e) = 0;

	uVar8 = VehCalc_MapToRange(cDC->cameraMoveSpeed, (int)zoom->speedMin, (int)zoom->speedMax, (int)zoom->distMin, (int)zoom->distMax);

	*(s16 *)(scratchpad + 0x210) = uVar8;

	if (*(int *)&cDC->unk_b8[4] == 0)
	{
		if (*(int *)&cDC->unk_b8[0] != 0)
		{
			*(int *)&cDC->unk_b8[0] -= gGT->elapsedTimeMS * 0x10;

			if (*(int *)&cDC->unk_b8[0] < 0)
				*(int *)&cDC->unk_b8[0] = 0;
		}
	}
	else
	{
		*(int *)&cDC->unk_b8[0] += gGT->elapsedTimeMS * 0x40;
		if (*(int *)&cDC->unk_b8[0] > 0x6000)
			*(int *)&cDC->unk_b8[0] = 0x6000;

		*(int *)&cDC->unk_b8[4] -= gGT->elapsedTimeMS;
		if (*(int *)&cDC->unk_b8[4] < 0)
			*(int *)&cDC->unk_b8[4] = 0;
	}
	*(s16 *)(scratchpad + 0x210) += *(int *)&cDC->unk_b8[0] >> 8;

	gte_SetRotMatrix((MATRIX *)(scratchpad + 0x220));
	psVar12 = (SVECTOR *)(scratchpad + 0x20c);
	gte_ldv0(psVar12);
	gte_rtv0();
	gte_stlvnl((VECTOR *)(scratchpad + 0x240));

	*(s16 *)(scratchpad + 0x20c) = 0;
	*(s16 *)(scratchpad + 0x20e) = 0x40;
	*(s16 *)(scratchpad + 0x210) = 0;

	gte_ldv0(psVar12);
	gte_rtv0();
	gte_stlvnl((VECTOR *)(scratchpad + 0x27c));

	*(int *)(scratchpad + 600) = d->posCurr.x >> 8;
	*(int *)(scratchpad + 0x25c) = d->posCurr.y >> 8;
	*(int *)(scratchpad + 0x260) = d->posCurr.z >> 8;

	*(int *)(scratchpad + 600) += *(int *)(scratchpad + 0x27c);
	*(int *)(scratchpad + 0x260) += *(int *)(scratchpad + 0x284);
	*(int *)(scratchpad + 0x240) += *(int *)(scratchpad + 600);


	// mask-grab
	if ((cDC->flags & 0x10) != 0)
	{
		*(int *)(scratchpad + 0x244) = (d->quadBlockHeight >> 8) + (int)cDC->unk98 + (int)zoom->vertDistance;
	}

	else
	{
		*(int *)(scratchpad + 0x244) += *(int *)(scratchpad + 0x25c) + (int)zoom->vertDistance;
	}

	*(int *)(scratchpad + 0x248) += *(int *)(scratchpad + 0x260);

	uVar8 = 0;
	if (gGT->numPlyrCurrGame != 2)
		uVar8 = 0xff9c;

	// rotX
	*(s16 *)(scratchpad + 0x20c) = uVar8;

	// rotZ
	*(s16 *)(scratchpad + 0x210) = 0;

	// rotY
	*(u16 *)(scratchpad + 0x20e) = (d->rotCurr.w + d->angle + d->turnAngleCurr + 0x800 + sVar10) & 0xfff;

	// convert 3 rotation shorts into rotation matrix
	ConvertRotToMatrix(scratchpad + 0x220, scratchpad + 0x20c);

	// if racer is not damaged,
	// slight-down view angle
	if ((d->actionsFlagSet & 0x4000) == 0)
	{
		cDC->unk1A -= 8;
		if (cDC->unk1A < -0x20)
			cDC->unk1A = -0x20;
	}

	// if racer is damaged,
	// straight-forward angle
	else
	{
		cDC->unk1A += 8;
		if (cDC->unk1A > 0)
			cDC->unk1A = 0;
	}

	// Z, Y, X
	*(s16 *)(scratchpad + 0x210) = cDC->unk1A;
	*(s16 *)(scratchpad + 0x20e) = 0;
	*(s16 *)(scratchpad + 0x20c) = 0;

	gte_SetRotMatrix((MATRIX *)(scratchpad + 0x220));

	psVar12 = (SVECTOR *)(scratchpad + 0x20c);

	gte_ldv0(psVar12);
	gte_rtv0();
	gte_stsv(psVar12);

	*(int *)(scratchpad + 600) += (int)*(s16 *)(scratchpad + 0x20c);
	*(int *)(scratchpad + 0x260) += (int)*(s16 *)(scratchpad + 0x210);
	*(int *)(scratchpad + 0x25c) += (int)*(s16 *)(scratchpad + 0x20e) + (int)zoom->angle[2];

	cDC->desiredRot[0] = ((zoom->angle[1] * (int)cDC->desiredRot[0]) + ((0x100 - (int)zoom->angle[1]) * (int)d->rotCurr.z)) >> 8;


	state = d->kartState;

	if (state == KS_MASK_GRABBED)
	{
		// pushBuffer position
		*(int *)(scratchpad + 0x240) = (int)pb->pos[0];
		*(int *)(scratchpad + 0x244) = (int)pb->pos[1];
		*(int *)(scratchpad + 0x248) = (int)pb->pos[2];

		// reset camera interpolation
		*(s16 *)((int)cDC + 0xc0) = 0;
		*(s16 *)((int)cDC + 0xc2) = 0;
		cDC->framesZoomingOut = 0;
	}

	if (state == KS_ENGINE_REVVING)
	{
		// reset camera interpolation
		*(s16 *)((int)cDC + 0xc0) = 0;
		*(s16 *)((int)cDC + 0xc2) = 0;
		cDC->framesZoomingOut = 0;
	}

	if ((d->kartState != KS_BLASTED) && ((d->actionsFlagSet & 1) != 0) && (cDC->BlastedLerp.boolLerpPending != 0))
	{
		cDC->BlastedLerp.boolLerpPending = 0;

		cDC->BlastedLerp.desiredRot[0] = cDC->unkTriplet3[0] - *(s16 *)(scratchpad + 600);
		cDC->BlastedLerp.desiredRot[1] = cDC->unkTriplet3[1] - *(s16 *)(scratchpad + 0x25c);
		cDC->BlastedLerp.desiredRot[2] = cDC->unkTriplet3[2] - *(s16 *)(scratchpad + 0x260);

		cDC->BlastedLerp.desiredPos[0] = cDC->unkTriplet2[0] - *(s16 *)(scratchpad + 0x240);
		cDC->BlastedLerp.desiredPos[1] = cDC->unkTriplet2[1] - *(s16 *)(scratchpad + 0x244);
		cDC->BlastedLerp.desiredPos[2] = cDC->unkTriplet2[2] - *(s16 *)(scratchpad + 0x248);

		cDC->BlastedLerp.framesRemaining = 8;
	}

	// if not arcade end-of-race
	if (((cDC->flags & 0x20) == 0) && (cDC->cameraMode == 0))
	{
		if ((d->kartState != KS_BLASTED) && (cDC->BlastedLerp.boolLerpPending == 0))
			goto LAB_8001a8c0;

		if (cDC->BlastedLerp.boolLerpPending == 0)
		{
			*(s16 *)((int)cDC + 0xc8) = cDC->unkTriplet3[1] - cDC->unkTriplet2[1];
			*(s16 *)((int)cDC + 0xca) = cDC->unkTriplet2[1] - (d->quadBlockHeight >> 8);
		}

		cDC->BlastedLerp.boolLerpPending = 1;

		if (((int)cDC->unkTriplet2[1] < *(int *)(scratchpad + 0x244)) &&
		    (x = (int)*(s16 *)((int)cDC + 0xca) + (d->quadBlockHeight >> 8), x < *(int *)(scratchpad + 0x244)))
		{
			*(int *)(scratchpad + 0x244) = x;
		}

	LAB_8001a8b0:

		if (cDC->BlastedLerp.boolLerpPending == 0)
			goto LAB_8001a8c0;
	}

	// if this is arcade end-of-race
	else
	{
		if (cDC->BlastedLerp.boolLerpPending != 0)
		{
			cDC->BlastedLerp.boolLerpPending = 0;

			cDC->BlastedLerp.desiredRot[0] = cDC->unkTriplet3[0] - *(s16 *)(scratchpad + 0x258);
			cDC->BlastedLerp.desiredRot[1] = cDC->unkTriplet3[1] - *(s16 *)(scratchpad + 0x25c);
			cDC->BlastedLerp.desiredRot[2] = cDC->unkTriplet3[2] - *(s16 *)(scratchpad + 0x260);

			cDC->BlastedLerp.desiredPos[0] = cDC->unkTriplet2[0] - *(s16 *)(scratchpad + 0x240);
			cDC->BlastedLerp.desiredPos[1] = cDC->unkTriplet2[1] - *(s16 *)(scratchpad + 0x244);
			cDC->BlastedLerp.desiredPos[2] = cDC->unkTriplet2[2] - *(s16 *)(scratchpad + 0x248);

			cDC->BlastedLerp.framesRemaining = 8;

			goto LAB_8001a8b0;
		}

	LAB_8001a8c0:

		// if frame countdown is not finished
		if (cDC->BlastedLerp.framesRemaining != 0)
		{
			*(int *)(scratchpad + 0x240) += (cDC->BlastedLerp.desiredPos[0] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x244) += (cDC->BlastedLerp.desiredPos[1] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x248) += (cDC->BlastedLerp.desiredPos[2] * cDC->BlastedLerp.framesRemaining) >> 3;

			*(int *)(scratchpad + 0x258) += (cDC->BlastedLerp.desiredRot[0] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x25c) += (cDC->BlastedLerp.desiredRot[1] * cDC->BlastedLerp.framesRemaining) >> 3;
			*(int *)(scratchpad + 0x260) += (cDC->BlastedLerp.desiredRot[2] * cDC->BlastedLerp.framesRemaining) >> 3;

			// decrease frame countdown
			cDC->BlastedLerp.framesRemaining--;
		}
	}

	CAM_FindClosestQuadblock(scratchpad, cDC, d, scratchpad + 0x240);

	if ((*(s16 *)(scratchpad + 0x3e) == 0) ||

	    // quadblock->quadFlags & 0x4100
	    ((*(u16 *)(*(int *)(scratchpad + 0x80) + 0x12) & 0x4100) != 0))
	{
		if (*(int *)(scratchpad + 0x244) < (int)cDC->framesZoomingOut + (d->posCurr.y >> 8))
		{
			*(s16 *)((int)cDC + 0xc2) = 8;
			*(s16 *)((int)cDC + 0xc0) = cDC->framesZoomingOut;
			*(int *)(scratchpad + 0x244) = (int)cDC->framesZoomingOut + (d->posCurr.y >> 8);

			goto LAB_8001ab04;
		}

		*(s16 *)((int)cDC + 0xc2) = 8;
		*(s16 *)((int)cDC + 0xc0) = *(s16 *)(scratchpad + 0x244) - (s16)(d->posCurr.y >> 8);
	}

	else
	{
		// quadblock->terrainFlags
		state = *(char *)(*(int *)(scratchpad + 0x80) + 0x38);

		// Mud, Water, or FastWater
		if (((state == 0xe) || (state == 4)) || (state == 0xd))
		{
			*(s16 *)(scratchpad + 0x1e) = 0;
		}

		x = (int)*(s16 *)(scratchpad + 0x1e) + (int)zoom->vertDistance;
		if (*(int *)(scratchpad + 0x244) < x)
		{
			*(int *)(scratchpad + 0x244) = x;
		}

		x = (int)*(s16 *)((int)cDC + 0xc2);
		if (x != 0)
		{
			*(int *)(scratchpad + 0x244) =

			    (8 - x) * *(int *)(scratchpad + 0x244) + x * ((int)*(s16 *)((int)cDC + 0xc0) + (d->posCurr.y >> 8)) >> 3;

			*(s16 *)((int)cDC + 0xc2) += -1;
		}
	}
	cDC->framesZoomingOut = *(s16 *)(scratchpad + 0x244) - (d->posCurr.y >> 8);
LAB_8001ab04:

	// if mask grabs you when you're underwater
	if (((gGT->level1->configFlags & 2) != 0) && (*(int *)(scratchpad + 0x244) < zoom->vertDistance))
	{
		*(int *)(scratchpad + 0x244) = zoom->vertDistance;
	}

	if (cDC->BlastedLerp.boolLerpPending != 0)
	{
		*(int *)(scratchpad + 0x25c) = *(int *)(scratchpad + 0x244) + (int)*(s16 *)((int)cDC + 0xc8);
	}

	if (d->kartState == KS_MASK_GRABBED)
	{
		pb->rot[2] -= (pb->rot[2] >> 3);

		// camera dirX, cameraPosX minus driverPosX
		*(int *)(scratchpad + 0x24c) = (int)pb->pos[0] - (d->posCurr.x >> 8);

		// camera dirY, cameraPosY minus driverPosY, plus something else
		*(int *)(scratchpad + 0x250) = (int)pb->pos[1] - ((d->posCurr.y >> 8) + (int)zoom->angle[2]);

		// camera dirZ, cameraPosZ minus driverPosZ
		*(int *)(scratchpad + 0x254) = (int)pb->pos[2] - (d->posCurr.z >> 8);

		if (pb->rot[0] < 0x800)
		{
			pb->rot[0] += 0x10;
			if (pb->rot[0] > 0x800)
				pb->rot[0] = 0x800;
		}
	}

	// if not mask grab
	else
	{
		x = *(int *)(scratchpad + 0x248) - *(int *)(scratchpad + 0x260);

		// camera direction
		*(int *)(scratchpad + 0x254) = x;
		*(int *)(scratchpad + 0x24c) = *(int *)(scratchpad + 0x240) - *(int *)(scratchpad + 600);
		*(int *)(scratchpad + 0x250) = *(int *)(scratchpad + 0x244) - *(int *)(scratchpad + 0x25c);

		// camera rotation
		x_00 = ratan2(*(s32 *)(scratchpad + 0x24c), x);
		pb->rot[1] = (s16)x_00;

		x_00 = SquareRoot0_stub(*(int *)(scratchpad + 0x24c) * *(int *)(scratchpad + 0x24c) + *(int *)(scratchpad + 0x254) * *(int *)(scratchpad + 0x254));

		x_00 = ratan2(*(s32 *)(scratchpad + 0x250), x_00);
		pb->rot[0] = 0x800 - (s16)x_00;

		pb->rot[2] = (s16)((u32)((int)zoom->angle[0] * (int)cDC->desiredRot[0]) >> 8);
	}

	// something with pushBuffer position
	*(int *)(scratchpad + 0x214) = *(int *)(scratchpad + 0x240) - (int)pb->pos[0];
	*(int *)(scratchpad + 0x218) = *(int *)(scratchpad + 0x244) - (int)pb->pos[1];
	*(int *)(scratchpad + 0x21c) = *(int *)(scratchpad + 0x248) - (int)pb->pos[2];

	cDC->unkTriplet1[0] -= (*(int *)(scratchpad + 0x240) - *(int *)((int)cDC + 0x58));
	cDC->unkTriplet1[1] -= (*(int *)(scratchpad + 0x244) - *(int *)((int)cDC + 0x5c));
	cDC->unkTriplet1[2] -= (*(int *)(scratchpad + 0x248) - *(int *)((int)cDC + 0x60));

	if (cDC->unkTriplet1[0] > 2)
		cDC->unkTriplet1[0] = 2;
	if (cDC->unkTriplet1[1] > 2)
		cDC->unkTriplet1[1] = 2;
	if (cDC->unkTriplet1[2] > 2)
		cDC->unkTriplet1[2] = 2;

	if (cDC->unkTriplet1[0] < -2)
		cDC->unkTriplet1[0] = -2;
	if (cDC->unkTriplet1[1] < -2)
		cDC->unkTriplet1[1] = -2;
	if (cDC->unkTriplet1[2] < -2)
		cDC->unkTriplet1[2] = -2;

	if (d->kartState != KS_MASK_GRABBED)
	{
		// pushBuffer position
		pb->pos[0] += *(s16 *)(scratchpad + 0x214) + cDC->unkTriplet1[0];
		pb->pos[1] += *(s16 *)(scratchpad + 0x218) + cDC->unkTriplet1[1];
		pb->pos[2] += *(s16 *)(scratchpad + 0x21c) + cDC->unkTriplet1[2];
	}

	cDC->unkTriplet2[0] = *(int *)(scratchpad + 0x240);
	cDC->unkTriplet2[1] = *(int *)(scratchpad + 0x244);
	cDC->unkTriplet2[2] = *(int *)(scratchpad + 0x248);
	cDC->unkTriplet3[0] = *(int *)(scratchpad + 0x258);
	cDC->unkTriplet3[1] = *(int *)(scratchpad + 0x25c);
	cDC->unkTriplet3[2] = *(int *)(scratchpad + 0x260);

	// backup flags (again)
	backupFlags = cDC->flags;

	cDC->flags &= 0xffffffef;

	if (
	    // transitioning, end-race battle, intro-race
	    ((backupFlags & 0x204) == 0) && ((gGT->gameMode1 & START_OF_RACE) == 0))
	{
		return;
	}

	// === Transition, end-race battle, intro-race ===

	// if end-of-race battle
	if ((backupFlags & 4) != 0)
	{
		CAM_FollowDriver_Spin360(cDC, scratchpad, d, &local_40[0], &local_38[0]);

		// reverse interpolation of fly-in [0x1000 to 0]
		x = 0x1000 - cDC->unk8C;
	}

	// if not end-of-race battle
	else
	{
		// if transitioning round-trip
		if ((backupFlags & 0x200) != 0)
		{
			// cameraDC TransitionTo pos and rot
			local_40[0] = cDC->transitionTo.pos[0];
			local_40[1] = cDC->transitionTo.pos[1];
			local_40[2] = cDC->transitionTo.pos[2];

			local_38[0] = cDC->transitionTo.rot[0];
			local_38[1] = cDC->transitionTo.rot[1];
			local_38[2] = cDC->transitionTo.rot[2];

			// interpolate fly-in [0 to 0x1000]
			x = cDC->unk8C;
		}

		// if startline camera
		else
		{
			// get camera path from level
			struct SpawnType1 *st1 = gGT->level1->ptrSpawnType1;
			void **pointers = ST1_GETPOINTERS(st1);
			x = pointers[ST1_CAMERA_PATH];
			int flyInDone = 0;

			// No camera + No ghosts (battle maps)
			if (st1->count < 4)
			{
				// startline fly-in is done
				flyInDone = 1;
				x = 0x1000;
			}

			// run fly-in animation
			else
			{
				flyInData.ptrEnd = x + 0x354;
				flyInData.ptrStart = x;
				flyInData.frameCount1 = 0x96;
				flyInData.frameCount2 = 0x8e;

				// which frame of fly-in you are in
				x = 0xa5 - (u16)cDC->unk8E;

				if ((s16)x > 0x96)
					x = 0x96;

				CAM_StartLine_FlyIn(&flyInData, 0x96, x, &local_40[0], &local_38[0]);

				// get interpolation of fly-in [0 - 0x1000]
				x = (int)cDC->unk8C;
			}

			if (cDC->unk8E < 1)
			{
				flyInDone = 1;
			}

			// Press Triangle
			if ((pad->buttonsTapped & BTN_TRIANGLE) != 0)
			{
				cDC->flags |= 9;
				flyInDone = 1;
			}

			// if startline fly-in is done
			if (flyInDone)
			{
				gGT->hudFlags |= 0x21;
				gGT->gameMode1 &= ~(START_OF_RACE);
			}
		}
	}

	// use camera pos+rot, TransitionTo pos+rot, camera pos+rot, and interpolation
	CAM_ProcessTransition(&pb->pos[0], &pb->rot[0], &local_40[0], &local_38[0], &pb->pos[0], &pb->rot[0], x);

	*(int *)(scratchpad + 0x240) = (int)pb->pos[0];
	*(int *)(scratchpad + 0x244) = (int)pb->pos[1];
	*(int *)(scratchpad + 0x248) = (int)pb->pos[2];

	CAM_FindClosestQuadblock(scratchpad, cDC, d, scratchpad + 0x240);

	x = cDC->frameCounterTransition;
	iVar14 = cDC->frameCounterTransition;

	if (iVar14 != 0)
	{
		iVar12 = (int)cDC->unk8E;

		if (iVar12 <= iVar14)
		{
			x = x >> 1;

			if (iVar12 < x)
			{
				// Sine(angle)
				x = MATH_Sin(0x400 - (iVar12 << 10) / x);

				cDC->unk8C = (s16)(x / 2) + 0x800;
			}
			else
			{
				iVar14 = (iVar12 - iVar14) * 0x400;

				// Cosine(angle)
				x = MATH_Cos(iVar14 / x);

				cDC->unk8C = 0x800 - (s16)(x / 2);
			}
		}
	}

	// backup  flags
	backupFlags = cDC->flags;

	// if transition is a round-trip,
	// like Load/Save that moves and comes back
	if ((backupFlags & 0x200) != 0)
	{
		// if not transitioning back to player
		if ((backupFlags & 0x400) == 0)
		{
			// Definitely >, not >=,
			// or else the transition is off-by-one,

			// |= 0x800, stop transitioning away from player,
			// sit stationary away from player, wait before moving back

			cDC->unk8E++;
			if (cDC->unk8E > cDC->frameCounterTransition)
			{
				cDC->unk8E = cDC->frameCounterTransition;
				cDC->flags |= 0x800;
				return;
			}
		}

		// if transitioning back to player
		else
		{
			// definitely do < 0, not == 0,
			// or else the transition is off-by-one

			// &= ~(0xE00), remove transition flags

			// optimization
			goto Countdown8E;
		}
	}

	// if not a round-trip,
	// like startline camera
	else
	{
		// If game is paused
		if ((gGT->gameMode1 & PAUSE_ALL) != 0)
			return;

	Countdown8E:

		cDC->unk8E--;
		if (cDC->unk8E < 0)
		{
			// This is normally not here,
			// but saves byte budget
			cDC->flags &= ~(0x200 | 0x400 | 0x800);

			cDC->unk8E = 0;
			return;
		}
	}

	return;
}

static u32 CAM_MapRange_PackS16Pair(s32 lo, s32 hi)
{
	return (u16)lo | ((u32)(u16)hi << 16);
}

int CAM_MapRange_PosPoints(s16 *pos1, s16 *pos2, s16 *currPos)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001b254-0x8001b334.
	SVec3 pathDelta;
	pathDelta.x = pos1[0] - pos2[0];
	pathDelta.y = pos1[1] - pos2[1];
	pathDelta.z = pos1[2] - pos2[2];

	MATH_VectorNormalize(&pathDelta);

	SVec3 currDelta;
	currDelta.x = currPos[0] - pos1[0];
	currDelta.y = currPos[1] - pos1[1];
	currDelta.z = currPos[2] - pos1[2];

	CTC2(CAM_MapRange_PackS16Pair(pathDelta.x, pathDelta.y), 0);
	CTC2((s32)pathDelta.z, 1);
	gte_ldv0(&currDelta);
	gte_mvmva(0, 0, 0, 3, 0);

	return MFC2_S(25) >> 12;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001b334-0x8001c360.
void CAM_ThTick(struct Thread *t)
{
	s16 sVar1;
	int bVar2;
	u16 uVar4;
	s16 sVar5;
	s16 sVar6;
	int iVar7;
	int iVar8;
	u32 uVar9;
	u32 uVar10;
	struct PVS *psVar11;
	int *piVar12;
	struct Instance **ppsVar13;
	struct SpawnType1 *psVar14;
	struct CheckpointNode *psVar15;
	u32 uVar16;
	int iVar17;
	int iVar18;
	s16 *psVar19;
	s16 *psVar20;
	s16 *psVar21;
	u32 uVar22;
	struct CameraDC *cDC;
	struct PushBuffer *pb;
	struct ZoomData *ptrZoomData;
	struct Driver *d;
	s16 *scratchpad;
	int iVar24;
	int iVar25;

	struct GameTracker *gGT = sdata->gGT;
	scratchpad = (s16 *)0x1f800108;
	cDC = (struct CameraDC *)t->inst;
	d = cDC->driverToFollow;
	pb = cDC->pushBuffer;

	if ((cDC->flags & 0x8000) != 0)
	{
		return;
	}

	if (((((gGT->gameMode1 & (PAUSE_ALL | START_OF_RACE | MAIN_MENU | GAME_CUTSCENE)) == 0) && (d->instSelf->thread->funcThTick == 0)) &&
	     ((d->actionsFlagSet & 0x100000) == 0)) &&
	    (((d->kartState != KS_WARP_PAD && (d->kartState != KS_FREEZE)) &&
	      (((gGT->gameMode2 & 4) == 0 && ((sdata->gGamepads->gamepad[cDC->cameraID].buttonsTapped & 0x80U) != 0))))))
	{
		uVar4 = cDC->unk92 + 1;
		cDC->unk92 = uVar4;
		if (1 < uVar4)
		{
			cDC->unk92 = 0;
		}
		if (cDC->unk92 == 2)
		{
			cDC->cameraMode = 0xf;
		}
		else
		{
			cDC->nearOrFar = cDC->unk92;
			cDC->cameraMode = 0;
		}
	}

	ptrZoomData = &data.NearCam4x3;
	if (gGT->numPlyrCurrGame == 2)
		ptrZoomData = &data.NearCam8x3;

	ptrZoomData = &ptrZoomData[cDC->nearOrFar * 2];

	if ((cDC->flags & 0x20) == 0)
		goto SkipNewCameraEOR;

	psVar14 = gGT->level1->ptrSpawnType1;

	psVar21 = 0;
	if (psVar14->count < 3)
		goto SkipNewCameraEOR;

	void **ptrs = ST1_GETPOINTERS(psVar14);
	psVar19 = ptrs[ST1_CAMERA_EOR];

	// number of EOR cameras
	sVar6 = *psVar19;

	// advance to first EOR
	psVar20 = psVar19 + 1;

	if (sVar6 != 0)
	{
		uVar22 = (u32)d->unknown_lap_related[1];

		// pointer to modeID in EOR camera
		psVar19 += 2;

		for (; sVar6 != 0; sVar6--)
		{
			// camera mode
			iVar7 = (int)*psVar19;
			if (iVar7 < 0)
			{
				iVar7 = -iVar7;
			}

			// respawnPoint
			uVar16 = (u32)*psVar20;

			// +2 to include respawnPoint and modeID
			psVar20 = (s16 *)((int)psVar19 + data.EndOfRace_Camera_Size[iVar7] + 2);

			psVar15 = &gGT->level1->ptr_restart_points[uVar16];

			if ((uVar22 == uVar16) || (uVar22 == psVar15->nextIndex_forward) || (uVar22 == psVar15->nextIndex_left) ||
			    (uVar22 == psVar15->nextIndex_backward) || (uVar22 == psVar15->nextIndex_right))
			{
				psVar21 = psVar19;
			}
			psVar19 = psVar20 + 1;
		}
	}

	// if no EOR found, or EOR is already in-use
	if ((psVar21 == 0) || (psVar21 == cDC->currEOR))
		goto SkipNewCameraEOR;

	cDC->currEOR = (void *)psVar21;

	sVar6 = *psVar21;
	psVar19 = psVar21 + 1;
	sVar5 = sVar6;
	if (sVar6 < 0)
	{
		sVar5 = -sVar6;
	}
	cDC->cameraMode = sVar5;
	uVar22 = cDC->flags & 0xffffefff;
	cDC->flags = uVar22 | 9;
	if (sVar6 < 0)
	{
		cDC->flags = uVar22 | 0x1009;
	}
	cDC->flags = cDC->flags | 0x1000;

	switch (cDC->cameraMode)
	{
	case 0:
		pb->pos[0] = *(s16 *)(&d->instSelf->matrix.t[0]);
		pb->pos[1] = *(s16 *)(&d->instSelf->matrix.t[1]);
		pb->pos[2] = *(s16 *)(&d->instSelf->matrix.t[2]);
		pb->rot[0] = d->rotCurr.x;
		pb->rot[1] = d->rotCurr.y;
		pb->rot[2] = d->rotCurr.z;
		*(u16 *)&cDC->unk_c0 = 0;
		break;
	case 3:
		pb->pos[0] = *psVar19;
		pb->pos[1] = psVar21[2];
		pb->pos[2] = psVar21[3];
		pb->rot[0] = psVar21[4];
		pb->rot[1] = psVar21[5];
		pb->rot[2] = psVar21[6];
		*(s16 *)&cDC->action = *psVar19;
		*(s16 *)((int)&cDC->action + 2) = psVar21[2];
		*(s16 *)&cDC->mode = psVar21[3];
		*(s16 *)&cDC->unk0xC = psVar21[4];
		*(s16 *)((int)&cDC->unk0xC + 2) = psVar21[5];
		cDC->desiredRot[0] = psVar21[6];
		break;
	case 4:
		pb->pos[0] = *psVar19;
		pb->pos[1] = psVar21[2];
		pb->pos[2] = psVar21[3];
		break;
	case 7:
		(cDC->transitionTo).pos[0] = *psVar19;
		sVar6 = psVar21[2];
		uVar22 = cDC->flags & 0xffffffbf;
		cDC->flags = uVar22;
		(cDC->transitionTo).pos[1] = sVar6;
		if (psVar21[3] != 0)
		{
			cDC->flags = uVar22 | 0x40;
		}
		break;
	case 8:
	case 14:
		cDC->driverOffset_CamEyePos[0] = *psVar19;
		cDC->driverOffset_CamEyePos[1] = psVar21[2];
		cDC->driverOffset_CamEyePos[2] = psVar21[3];

		cDC->driverOffset_CamLookAtPos[0] = psVar21[4];
		cDC->driverOffset_CamLookAtPos[1] = psVar21[5];
		cDC->driverOffset_CamLookAtPos[2] = psVar21[6];

		// driverOffset_CamEyePos
		sVar6 = *psVar19;
		sVar5 = psVar21[2];
		sVar1 = psVar21[3];

		iVar7 = VehCalc_MapToRange((int)sVar6 * (int)sVar6 + (int)sVar5 * (int)sVar5 + (int)sVar1 * (int)sVar1, 0x10000, 0x190000, 0x80, 0xf0);

		cDC->unk7A = (s16)iVar7;
		break;
	case 9:
	case 13:
		sVar6 = *psVar19;
		psVar15 = gGT->level1->ptr_restart_points;
		cDC->unk94 = 0;
		cDC->unk88 = psVar15 + sVar6;
		(cDC->transitionTo).pos[0] = psVar21[2];
		(cDC->transitionTo).pos[1] = psVar21[3];
		(cDC->transitionTo).pos[2] = psVar21[4];
		(cDC->transitionTo).rot[0] = psVar21[5];
		(cDC->transitionTo).rot[1] = psVar21[6];
		(cDC->transitionTo).rot[2] = psVar21[7];
		*(s16 *)&cDC->unk_b0[0] = psVar21[8];
		break;

	// Spin360
	case 10:

		// spinSpeed
		sVar6 = *psVar19;

		psVar19 = psVar21 + 2;
		goto LAB_8001b928;

	case 11:
		sVar6 = *(s16 *)&pb->distanceToScreen_CURR;
		pb->pos[0] = *psVar19;
		pb->pos[1] = psVar21[2];
		pb->pos[2] = psVar21[3];
		psVar19 = psVar21 + 5;
		sVar6 = psVar21[4] - sVar6;
	LAB_8001b928:
		(cDC->transitionTo).pos[0] = sVar6;
		(cDC->transitionTo).pos[1] = *psVar19;
		(cDC->transitionTo).pos[2] = psVar19[1];
		break;
	case 12:
		(cDC->transitionTo).pos[0] = *psVar19;
		(cDC->transitionTo).pos[1] = psVar21[2];
		(cDC->transitionTo).pos[2] = psVar21[3];
		(cDC->transitionTo).rot[0] = psVar21[4];
		(cDC->transitionTo).rot[1] = psVar21[5];
		(cDC->transitionTo).rot[2] = psVar21[6];
		*(s16 *)&cDC->unk_b0[0] = psVar21[7];
		*(s16 *)&cDC->unk_b0[2] = psVar21[8];
		*(s16 *)&cDC->unk_b0[4] = psVar21[9];
		*(s16 *)&cDC->unk_b0[6] = psVar21[10];
	}

SkipNewCameraEOR:

	pb->distanceToScreen_PREV = pb->distanceToScreen_CURR;
	sVar6 = cDC->cameraMode;

	if (sVar6 != 0)
	{
		sVar5 = cDC->cameraMode;
		cDC->BlastedLerp.framesRemaining = 0;

		if (sVar6 != 0)
		{
			if (sVar6 == 4)
			{
			LAB_8001c11c:
				CAM_LookAtPosition((int)scratchpad, (int *)&d->posCurr.x, &pb->pos[0], &pb->rot[0]);
				psVar21 = scratchpad;
			LAB_8001c128:
				scratchpad = psVar21;
			}
			else
			{
				psVar21 = scratchpad;
				if (sVar6 == 10)
				{
					CAM_FollowDriver_Spin360(cDC, (int)0x1f800108, d, (s16 *)pb->pos, (s16 *)pb->rot);
					goto LAB_8001c128;
				}
				if (sVar6 != 0xb)
				{
					if (sVar6 == 0xc)
					{
						if (cDC->cameraModePrev != 0xc)
						{
							cDC->unk8E = 0;
						}

						s16 stackMemPos[3];

						stackMemPos[0] = (s16)((u32)d->posCurr.x >> 8);
						stackMemPos[1] = (s16)((u32)d->posCurr.y >> 8);
						stackMemPos[2] = (s16)((u32)d->posCurr.z >> 8);

						iVar8 = CAM_MapRange_PosPoints((cDC->transitionTo).pos, (cDC->transitionTo).rot, &stackMemPos[0]);

						iVar17 = (int)*(s16 *)(&cDC->unk_b0[6]);

						stackMemPos[0] = *(s16 *)&cDC->unk_b0[0] - (cDC->transitionTo).rot[0];
						stackMemPos[1] = *(s16 *)&cDC->unk_b0[2] - (cDC->transitionTo).rot[1];
						stackMemPos[2] = *(s16 *)&cDC->unk_b0[4] - (cDC->transitionTo).rot[2];

						iVar7 = iVar17;
						if (iVar17 < 0)
						{
							iVar7 = -iVar17;
						}
						if (iVar17 < 0)
						{
							stackMemPos[0] = -stackMemPos[0];
							stackMemPos[1] = -stackMemPos[1];
							stackMemPos[2] = -stackMemPos[2];
						}

						iVar24 = SquareRoot0_stub((int)stackMemPos[0] * (int)stackMemPos[0] + (int)stackMemPos[1] * (int)stackMemPos[1] +
						                          (int)stackMemPos[2] * (int)stackMemPos[2]);

						iVar18 = cDC->unk94 << 0xc;
						iVar25 = iVar18 / iVar24;
						/*
						if (iVar24 == 0)
						{
						    trap(0x1c00);
						}
						if ((iVar24 == -1) && (iVar18 == -0x80000000))
						{
						    trap(0x1800);
						}
						*/
						cDC->unk94 = cDC->unk94 + (((cDC->unk8E * 0x1000) / 0x1e) * iVar7 >> 0xc);
						if (iVar8 < 1)
						{
							if (iVar25 < 0x1001)
							{
								if (cDC->unk8E < 0x1e)
								{
									cDC->unk8E = cDC->unk8E + 1;
								}
							}
							else if (cDC->unk8E != 0)
							{
								cDC->unk8E = cDC->unk8E + -1;
							}
						}
						else
						{
							cDC->unk94 = 0;
							cDC->unk8E = 0;
						}
						psVar21 = (cDC->transitionTo).rot;
						if (iVar17 < 0)
						{
							psVar21 = (s16 *)&cDC->unk_b0[0];
						}
						pb->pos[0] = psVar21[0] + (s16)((stackMemPos[0] * iVar25) >> 0xc);
						pb->pos[1] = psVar21[1] + (s16)((stackMemPos[1] * iVar25) >> 0xc);
						pb->pos[2] = psVar21[2] + (s16)((stackMemPos[2] * iVar25) >> 0xc);
						goto LAB_8001c11c;
					}
					if (sVar6 == 7)
					{
						pb->pos[0] = (s16)((u32)d->posCurr.x >> 8);
						pb->pos[1] = (cDC->transitionTo).pos[0] + (s16)((u32)d->posCurr.y >> 8);
						pb->pos[2] = (s16)((u32)d->posCurr.z >> 8);
						sVar6 = (cDC->transitionTo).pos[1];
						pb->rot[1] = 0;
						pb->rot[2] = 0;
						pb->rot[0] = sVar6 + 0x400;
						psVar21 = (s16 *)0x1f800108;
						if ((cDC->flags & 0x40) != 0)
						{
							pb->rot[1] = d->angle + 0x800;
						}
					}
					else if ((u16)(sVar5 - 0xfU) < 2)
					{
						pb->pos[0] = sdata->FirstPersonCamera.posOffset[0] + (s16)((u32)d->posCurr.x >> 8);
						pb->pos[1] = sdata->FirstPersonCamera.posOffset[1] + (s16)((u32)d->posCurr.y >> 8);
						pb->pos[2] = sdata->FirstPersonCamera.posOffset[2] + (s16)((u32)d->posCurr.z >> 8);

						pb->rot[0] = sdata->FirstPersonCamera.rotOffset[0] + (d->rotCurr).x;
						if (cDC->cameraMode == 0x10)
							pb->rot[1] = sdata->FirstPersonCamera.rotOffset[1] + d->angle;
						else
							pb->rot[1] = sdata->FirstPersonCamera.rotOffset[1] + (d->rotCurr).y;
						pb->rot[2] = sdata->FirstPersonCamera.rotOffset[2] + (d->rotCurr).z;
					}
					else
					{
						if ((sVar6 == 8) || (sVar6 == 0xe))
						{
							if ((d->botData.botFlags & 2U) == 0)
							{
								if ((cDC->driver5B0_prevFrame & 2) != 0)
								{
									cDC->flags = cDC->flags | 9;
								}
								CAM_FollowDriver_AngleAxis(cDC, d, (int)0x1f800108, pb->pos, pb->rot);
							}
							else
							{
								if ((cDC->driver5B0_prevFrame & 2) == 0)
								{
									cDC->flags = cDC->flags | 9;
								}
								CAM_FollowDriver_Normal(cDC, d, pb->pos, 0x1f800108, (s16 *)ptrZoomData);
							}
							cDC->driver5B0_prevFrame = d->botData.botFlags;
							goto LAB_8001c150;
						}
						if ((cDC->cameraMode == 9) || (psVar21 = (s16 *)0x1f800108, cDC->cameraMode == 0xd))
						{
							if ((gGT->level1->cnt_restart_points != 0) && ((gGT->gameMode1 & 0xf) == 0))
							{
								uVar9 = CAM_FollowDriver_TrackPath(cDC, (s16 *)0x1f800390, *(s16 *)&cDC->unk_b0[0], 1);

								iVar7 = -0xc0;
								if (-1 < *(s16 *)&cDC->unk_b0[0])
									iVar7 = 0xc0;

								uVar10 = CAM_FollowDriver_TrackPath(cDC, (s16 *)0x1f800398, iVar7, 0);

								// interpolate two rotations

								iVar8 = ((int)(((uVar10 - uVar9) + 0x800U & 0xfff) - 0x800) >> 1);
								*(s16 *)0x1f800314 = 0x800;
								*(s16 *)0x1f800316 = (s16)uVar9 + (s16)iVar8;
								*(s16 *)0x1f800318 = 0;

								// interpolate two positions
								*(s16 *)0x1f800390 = (s16)((int)(*(s16 *)0x1f800390) + (int)(*(s16 *)0x1f800398) >> 1);
								*(s16 *)0x1f800392 = (s16)((int)(*(s16 *)0x1f800392) + (int)(*(s16 *)0x1f80039a) >> 1);
								*(s16 *)0x1f800394 = (s16)((int)(*(s16 *)0x1f800394) + (int)(*(s16 *)0x1f80039c) >> 1);
								ConvertRotToMatrix((MATRIX *)0x1f800328, (s16 *)0x1f800314);
								gte_SetRotMatrix((MATRIX *)0x1f800328);
								gte_ldv0((SVECTOR *)&cDC->transitionTo);
								gte_rtv0();

								VECTOR pathOffset;
								gte_stlvnl(&pathOffset);
								uVar9 = pathOffset.vx;
								iVar7 = pathOffset.vy;
								iVar8 = pathOffset.vz;

								pb->pos[0] = scratchpad[0x144] + (s16)uVar9;
								pb->pos[1] = scratchpad[0x145] + (s16)iVar7;
								pb->pos[2] = scratchpad[0x146] + (s16)iVar8;
								pb->rot[0] = scratchpad[0x106] + cDC->transitionTo.rot[0];
								pb->rot[1] = scratchpad[0x107] + cDC->transitionTo.rot[1];
								pb->rot[2] = scratchpad[0x108] + cDC->transitionTo.rot[2];
							}
							psVar21 = scratchpad;
							if (cDC->cameraMode == 0xd)
								goto LAB_8001c11c;
						}
					}
					goto LAB_8001c128;
				}

				CAM_LookAtPosition((int)0x1f800108, (int *)&d->posCurr.x, &pb->pos[0], &pb->rot[0]);

				iVar7 = SquareRoot0_stub((*(int *)0x1f800354) * (*(int *)0x1f800354) + (*(int *)0x1f80035c) * (*(int *)0x1f80035c));
				iVar17 = (int)(cDC->transitionTo).pos[0];
				iVar24 = (iVar7 - (cDC->transitionTo).pos[1]) * iVar17;
				iVar8 = (int)(cDC->transitionTo).pos[2];
				iVar7 = iVar24 / iVar8;
				/*
				if (iVar8 == 0)
				{
				    trap(0x1c00);
				}
				if ((iVar8 == -1) && (iVar24 == -0x80000000))
				{
				    trap(0x1800);
				}
				*/
				bVar2 = iVar17 < iVar7;
				if (iVar7 < 0)
				{
					iVar7 = 0;
					bVar2 = iVar17 < 0;
				}
				if (bVar2)
				{
					iVar7 = iVar17;
				}
				pb->distanceToScreen_PREV = pb->distanceToScreen_CURR + iVar7;
			}

			*(int *)0x1f800348 = (int)pb->pos[0];
			*(int *)0x1f80034c = (int)pb->pos[1];
			*(int *)0x1f800350 = (int)pb->pos[2];

			CAM_FindClosestQuadblock(scratchpad, cDC, d, (s16 *)((u32)scratchpad | 0x240));
			goto LAB_8001c150;
		}
	}

	CAM_FollowDriver_Normal(cDC, d, &pb->pos[0], 0x1f800108, (s16 *)ptrZoomData);

LAB_8001c150:
	cDC->cameraModePrev = cDC->cameraMode;

	if (cDC->ptrQuadBlock != 0)
	{
		psVar11 = cDC->ptrQuadBlock->pvs;
		if ((psVar11 != 0) && (piVar12 = psVar11->visLeafSrc, piVar12 != 0))
		{
			cDC->visLeafSrc = piVar12;
			gGT->unk1cac[1] = ((int)cDC->ptrQuadBlock - (int)gGT->level1->ptr_mesh_info->ptrQuadBlockArray) * -0x1642c859 >> 2;
		}
		if (cDC->ptrQuadBlock != 0)
		{
			psVar11 = cDC->ptrQuadBlock->pvs;
			if ((psVar11 != 0) && (piVar12 = psVar11->visFaceSrc, piVar12 != 0))
			{
				cDC->visFaceSrc = piVar12;
			}
			if (cDC->ptrQuadBlock != 0)
			{
				psVar11 = cDC->ptrQuadBlock->pvs;
				if ((psVar11 != 0) && (ppsVar13 = psVar11->visInstSrc, ppsVar13 != 0))
				{
					cDC->visInstSrc = ppsVar13;
				}
				if (((cDC->ptrQuadBlock != 0) && (psVar11 = cDC->ptrQuadBlock->pvs, psVar11 != 0)) && (piVar12 = psVar11->visExtraSrc, piVar12 != 0))
				{
					if ((gGT->level1->configFlags & 4) == 0)
					{
						cDC->visOVertSrc = piVar12;
					}
					else
					{
						cDC->visSCVertSrc = piVar12;
					}
				}
			}
		}
	}

	if (*(int *)&cDC->unk30fill[8] == 0)
	{
		cDC->visLeafSrc = 0;
		cDC->visFaceSrc = 0;
	}

	if ((cDC->flags & 1) != 0)
	{
		gGT->rainBuffer[cDC->cameraID].cameraPos[0] = pb->pos[0];
		gGT->rainBuffer[cDC->cameraID].cameraPos[1] = pb->pos[1];
		gGT->rainBuffer[cDC->cameraID].cameraPos[2] = pb->pos[2];
		cDC->flags &= ~1;
	}
	cDC->flags &= ~0x88;

	return;
}
