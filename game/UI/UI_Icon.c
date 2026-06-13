#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004e0e0-0x8004e37c.
void UI_WeaponBG_AnimateShine(void)
{
	int sine;
	u32 local_18;
	u32 local_14;
	u32 local_10;

	sine = MATH_Sin((int)sdata->wumpaShineTheta);
	sine = (sine < 0) ? -sine : sine;

	// Calculate wumpaShineColor1
	local_18 = ((sine * 0x7f) >> 0xc) + 0x7f;
	sdata->wumpaShineColor1[0][0] = local_18;
	sdata->wumpaShineColor1[0][1] = local_18;
	*(s16 *)&sdata->wumpaShineColor1[0][2] = 0;

	local_14 = ((sine * 0x32) >> 0xc) + 0x32;
	sdata->wumpaShineColor1[1][0] = local_18;
	sdata->wumpaShineColor1[1][1] = local_14;
	*(s16 *)&sdata->wumpaShineColor1[1][2] = 0;

	sdata->wumpaShineColor1[2][0] = ((sine * 0x21) >> 0xc) + 0x21;
	sdata->wumpaShineColor1[2][1] = ((sine * 0x10) >> 0xc) + 0x10;
	*(s16 *)&sdata->wumpaShineColor1[2][2] = 0;

	// Calculate wumpaShineColor2
	local_10 = ((sine * 0x5f) >> 0xc) + 0x5f;
	sdata->wumpaShineColor2[0][0] = local_10;
	sdata->wumpaShineColor2[0][1] = local_10;
	*(s16 *)&sdata->wumpaShineColor2[0][2] = local_10;

	local_10 = *(int *)&sdata->wumpaShineColor2[0][0];
	*(int *)&sdata->wumpaShineColor2[1][0] = local_10;
	*(int *)&sdata->wumpaShineColor2[2][0] = local_10;

	sdata->wumpaShineResult = (sine * 0xff >> 0xd) + 0x80;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004e37c-0x8004e660.
void UI_WeaponBG_DrawShine(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 angleX, s16 angleY,
                           int unusedColor)
{
	s16 sVar1;
	s16 sVar2;
	s16 sVar3;
	s16 sVar4;
	s16 sVar5;
	s16 sVar6;
	POLY_GT4 *p;
	int i;
	s16 sVar11;
	s16 sVar12;

	(void)unusedColor;

	u32 *wumpaShine = (u32 *)&sdata->wumpaShineColor1[0][0];

	if (transparency == 3)
	{
		wumpaShine = (u32 *)&sdata->wumpaShineColor2[0][0];
	}

	sVar3 = (s16)(((icon->texLayout.u1 - icon->texLayout.u0) * (int)angleX) >> 0xc);
	sVar1 = posX + sVar3;
	angleX = angleX >> 0xc;
	sVar12 = sVar1 - angleX;

	sVar4 = (s16)(((icon->texLayout.v2 - icon->texLayout.v0) * (int)angleY) >> 0xc);
	sVar2 = posY + sVar4;
	angleY = angleY >> 0xc;
	sVar11 = sVar2 - angleY;

	// loop 4 times
	for (i = 0; i < 4; i++)
	{
		p = primMem->curr;
		*(int *)&p->u0 = *(int *)&icon->texLayout.u0;
		*(int *)&p->u1 = *(int *)&icon->texLayout.u1;
		*(int *)&p->u2 = *(int *)&icon->texLayout.u2;
		*(s16 *)&p->u3 = *(s16 *)&icon->texLayout.u3;

		switch (i)
		{
		// top left
		case 0:
			// xy0
			p->x0 = posX;
			p->y0 = posY;

			p->x1 = sVar1;
			p->y1 = posY;
			p->x2 = posX;
			p->y2 = sVar2;
			p->x3 = sVar1;
			p->y3 = sVar2;
			break;

		// top right
		case 1:
			// xy0
			sVar5 = (posX + sVar3 * 2) - angleX;
			p->x0 = sVar5;
			p->y0 = posY;

			p->x1 = sVar12;
			p->y1 = posY;
			p->x2 = sVar5;
			p->y2 = sVar2;
			p->x3 = sVar12;
			p->y3 = sVar2;

			break;

		case 2:
			// xy0
			sVar5 = (posY + sVar4 * 2) - angleY;
			p->x0 = posX;
			p->y0 = sVar5;

			p->x1 = sVar1;
			p->y1 = sVar5;
			p->x2 = posX;
			p->y2 = sVar11;
			p->x3 = sVar1;
			p->y3 = sVar11;

			break;

		case 3:
			// xy0
			sVar5 = (posX + sVar3 * 2) - angleX;
			sVar6 = (posY + sVar4 * 2) - angleY;
			p->x0 = sVar5;
			p->y0 = sVar6;

			p->x1 = sVar12;
			p->y1 = sVar6;
			p->x2 = sVar5;
			p->y2 = sVar11;
			p->x3 = sVar12;
			p->y3 = sVar11;

			break;
		}

		// color RGB
		*(u32 *)&p->r0 = wumpaShine[2];
		*(u32 *)&p->r1 = wumpaShine[1];
		*(u32 *)&p->r2 = wumpaShine[1];
		*(u32 *)&p->r3 = wumpaShine[0];

		setPolyGT4(p);

		if (transparency != 0)
		{
			p->tpage = (p->tpage & ~(0x60)) | (((u16)transparency - 1) * 0x20);
			p->code |= 2;
		}

		AddPrim(ot, p);

		// increment primMem
		primMem->curr = p + 1;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004e660-0x8004e8d8.
void UI_TrackerBG(struct Icon *targetIcon, s16 centerX, s16 centerY, struct PrimMem *primMem, u_long *ot, char transparency, s16 angleX, s16 angleY, int color)
{
	s16 rightX;
	s16 bottomY;
	s16 offsY;
	s16 tmpX;
	s16 tmpY;
	POLY_FT4 *p;
	int offsX;
	int quadIndex;
	s16 topY;
	s16 leftX;

	// wumpaShineTheta (given to sine)
	sdata->wumpaShineTheta += 0x100;

	offsX = ((targetIcon->texLayout.u1 - targetIcon->texLayout.u0) * angleX) >> 0xc;
	offsY = ((targetIcon->texLayout.v2 - targetIcon->texLayout.v0) * angleY) >> 0xc;

	rightX = centerX + offsX;
	angleX >>= 0xc;
	leftX = rightX - angleX;

	bottomY = centerY + offsY;
	angleY >>= 0xc;
	topY = bottomY - angleY;

	int altX0 = (centerX + (offsX * 2)) - angleX;
	int altY0 = (centerY + (offsY * 2)) - angleY;

	// loop 4 times
	for (quadIndex = 0; quadIndex < 4; quadIndex++)
	{
		p = primMem->curr;
		primMem->curr = (p + 1);

		*(int *)&p->r0 = *(int *)&color;
		*(int *)&p->u0 = *(int *)&targetIcon->texLayout.u0;
		*(int *)&p->u1 = *(int *)&targetIcon->texLayout.u1;
		*(s16 *)&p->u2 = *(s16 *)&targetIcon->texLayout.u2;
		*(s16 *)&p->u3 = *(s16 *)&targetIcon->texLayout.u3;

		setPolyFT4(p);

		if (transparency != 0)
		{
			p->tpage = (p->tpage & 0xff9f) | (((u16)transparency - 1) * 0x20);
			p->code |= 2;
		}

		// compiler optimization will remove this,
		// if not using widescreen hacks
		int len = 0;

		// quadIndex(0)
		p->x0 = centerX + len;
		p->x1 = rightX;
		p->y0 = centerY;
		p->y2 = bottomY;

		switch (quadIndex)
		{
		case 1:
			p->x0 = altX0 - len;
			p->x1 = leftX;
			break;

		case 2:
			p->y0 = altY0;
			p->y2 = topY;
			break;

		case 3:
			p->x0 = altX0 - len;
			p->y0 = altY0;

			p->x1 = leftX;
			p->y2 = topY;
		}

		p->x2 = p->x0;
		p->y1 = p->y0;

		p->x3 = p->x1;
		p->y3 = p->y2;

		AddPrim(ot, p);
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004e8d8-0x8004eaa8.
void UI_DrawDriverIcon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, char transparency, s16 scale, u32 color)
{
	PolyFT4 *p = primMem->curr;
	const PrimCode primCode = {.poly = {.renderCode = RenderCode_Polygon, .quad = 1, .textured = 1}};
	p->colorCode.self = color;
	p->colorCode.code = primCode;

	int width = icon->texLayout.u1 - icon->texLayout.u0;
	int height = icon->texLayout.v2 - icon->texLayout.v0;
	int topX = posX;
	int bottomX = topX + FP_Mult(width, scale);
#if BUILD != EurRetail
	int topY = (posY < 166) ? posY : 165;
	int bottomY = ((posY + FP_Mult(height, scale)) < 166) ? (posY + FP_Mult(height, scale)) : 165;
#else
	int topY = (posY < 176) ? posY : 175;
	int bottomY = ((posY + FP_Mult(height, scale)) < 176) ? (posY + FP_Mult(height, scale)) : 175;
#endif

	p->tag.size = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
	p->colorCode.code.code = 0x2c;

	p->v[0].pos.x = topX;
	p->v[0].pos.y = topY;
	p->v[1].pos.x = bottomX;
	p->v[1].pos.y = topY;
	p->v[2].pos.x = topX;
	p->v[2].pos.y = bottomY;
	p->v[3].pos.x = bottomX;
	p->v[3].pos.y = bottomY;

	p->polyClut.self = icon->texLayout.clut;
	p->polyTpage.self = icon->texLayout.tpage;
	p->v[2].clut.self = (icon->texLayout.v3 << 8) | icon->texLayout.u3;

	if (transparency)
	{
		p->polyTpage.semiTransparency = transparency - 1;
		p->colorCode.code.poly.semiTransparency = 1;
	}

	u32 bottomV = (icon->texLayout.v0 + bottomY) - posY;
	p->v[0].texCoords.u = icon->texLayout.u0;
	p->v[0].texCoords.v = icon->texLayout.v0;
	p->v[1].texCoords.u = icon->texLayout.u1;
	p->v[1].texCoords.v = icon->texLayout.v1;
	p->v[2].texCoords.u = icon->texLayout.u2;
	p->v[2].texCoords.v = bottomV;
	p->v[3].texCoords.u = icon->texLayout.u3;
	p->v[3].texCoords.v = bottomV;

	AddPrimitive(p, ot);
	primMem->curr = p + 1;
}
