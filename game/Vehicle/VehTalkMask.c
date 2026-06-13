#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068f90-0x80069178.
void VehTalkMask_ThTick(struct Thread *t)
{
	struct GameTracker *gGT = sdata->gGT;

	struct MaskHint *mhObj = t->object;
	struct Instance *mhInst = t->inst;

	int scale = 0x2000;

	if (sdata->modelMaskHints3D != 0)
	{
		mhInst->model = sdata->modelMaskHints3D;
	}
	else
	{
		scale = 0x1000;

		if (gGT->drivers[0] != 0)
		{
			int boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]);

			// 0x3A for Uka, 0x39 for Aku
			int modelID = STATIC_UKAUKA - boolGoodGuy;
			mhInst->model = gGT->modelPtr[modelID];
		}
	}

	scale = (mhObj->scale * scale) >> 0xc;
	mhInst->scale[0] = scale;
	mhInst->scale[1] = scale;
	mhInst->scale[2] = scale;

	u32 lastFrame = VehFrameInst_GetNumAnimFrames(mhInst, 0) - 1;

	sdata->unk_8008d9f4[1] = sdata->XA_MaxSampleValInArr;

	int iVar4 = sdata->unk_8008d9f4[1] * 7;

	if (iVar4 < 0)
	{
		iVar4 = iVar4 + 0x3fff;
	}

	iVar4 = iVar4 >> 0xe;

	if (sdata->unk_8008d9f4[2] < iVar4)
		sdata->unk_8008d9f4[2] = iVar4;

	int iVar6 = iVar4;
	if (iVar4 < 2)
	{
		iVar6 = 0;
	}

	int iVar5 = mhInst->animFrame;

	if (iVar4 > 3)
	{
		iVar4 = iVar5 - iVar6;

		if (iVar4 < 0)
		{
			iVar4 = -iVar4;
		}

		if (iVar4 > 3)
		{
			mhInst->animFrame = (s16)iVar6;

			goto SkipLerp;
		}
	}

	mhInst->animFrame = EngineSound_VolumeAdjust(iVar6, iVar5, 1);

SkipLerp:

	iVar5 = mhInst->animFrame;

	// animFrame
	iVar4 = iVar5 - iVar6;

	if (iVar4 < 0)
	{
		iVar4 = -iVar4;
	}

	if (iVar4 < 6)
	{
		mhInst->animFrame = EngineSound_VolumeAdjust(iVar6, iVar5, 1);
	}
	else
	{
		mhInst->animFrame = (s16)iVar6;
	}

	// animation frame goes back and forth
	// 0x00: mouth close
	// 0x0C: mouth open

	if (mhInst->animFrame < 0)
		mhInst->animFrame = 0;

	else if (lastFrame < (u32)mhInst->animFrame)
		mhInst->animFrame = (s16)lastFrame;

	if (sdata->talkMask_boolDead != 0)
	{
		sdata->talkMask_boolDead = 0;

		// dead thread
		t->flags |= 0x800;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069178-0x800691e4.
struct Instance *VehTalkMask_Init()
{
	sdata->boolIsMaskThreadAlive = 1;
	sdata->talkMask_boolDead = 0;

	struct Instance *mhInst = INSTANCE_BirthWithThread(0x39, sdata->s_head, SMALL, AKUAKU, VehTalkMask_ThTick, 6, 0);

	struct Thread *mhTh = mhInst->thread;
	mhTh->funcThDestroy = PROC_DestroyInstance;

	((struct MaskHint *)mhTh->object)->scale = 0;

	return mhInst;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800691e4-0x8006924c.
void VehTalkMask_PlayXA(struct Instance *i, int id)
{
	struct Driver *d = sdata->gGT->drivers[0];

	if (d != 0)
	{
		int boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(d);

		if (boolGoodGuy == 0)
			id += 0x1f;
	}

	CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, id);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006924c-0x8006925c.
int VehTalkMask_boolNoXA()
{
	return sdata->XA_State == 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006925c-0x80069284.
void VehTalkMask_End()
{
	CDSYS_XAPauseRequest();

	sdata->boolIsMaskThreadAlive = 0;
	sdata->talkMask_boolDead = 1;
}
