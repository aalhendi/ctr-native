#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021500-0x80021594.
void CTR_Box_DrawWirePrims(Point p1, Point p2, Color color, void *ot)
{
	LineF2 *p;
	GetPrimMem(p);
	if (p == nullptr)
	{
		return;
	}

	const PrimCode primCode = {.line = {.renderCode = RenderCode_Line}};
	color.code = primCode;
	p->colorCode = color;
	p->v[0].pos = p1;
	p->v[1].pos = p2;

	AddPrimitive(p, ot);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021594-0x8002177c.
void CTR_Box_DrawWireBox(RECT *r, const Color *color, void *ot, struct PrimMem *primMem)
{
	LineF3 *p = primMem->curr;
	if (p > (LineF3 *)primMem->endMin100)
	{
		return;
	}
	primMem->curr = p + 1;

	const PrimCode primCode = {.line = {.renderCode = RenderCode_Line, .polyline = 1}};
	p->tag.size = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
	p->colorCode = *color;
	p->colorCode.code = primCode;

	s16 topX = r->x;
	s16 topY = r->y;
	s16 bottomX = r->x + r->w;
	s16 bottomY = r->y + r->h;
	p->v[0].pos.x = topX;
	p->v[0].pos.y = topY;
	p->v[1].pos.x = bottomX;
	p->v[1].pos.y = topY;
	p->v[2].pos.x = bottomX;
	p->v[2].pos.y = bottomY;
	p->end = 0x55555555;

	AddPrimitive(p, ot);
	p = primMem->curr;
	if (p > (LineF3 *)primMem->endMin100)
	{
		return;
	}
	primMem->curr = p + 1;

	p->tag.size = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
	p->colorCode = *color;
	p->colorCode.code = primCode;
	p->v[0].pos.x = topX;
	p->v[0].pos.y = topY;
	p->v[1].pos.x = topX;
	p->v[1].pos.y = bottomY;
	p->v[2].pos.x = bottomX;
	p->v[2].pos.y = bottomY;
	p->end = 0x55555555;

	AddPrimitive(p, ot);
}

// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x8002177c-0x80021894.
void CTR_Box_DrawClearBox(const RECT *r, const Color *color, int transparency, u_long *ot)
{
	typedef struct TPage_PolyF4
	{
		TPage t;
		PolyF4 p;
	} TPage_PolyF4;

	TPage_PolyF4 *p;
	GetPrimMem(p);
	if (p == nullptr)
	{
		return;
	}

	p->t.texpage = (Texpage){.code = 0xE1, .semiTransparency = transparency, .dither = 1};
	p->p.tag.self = 0;

	const PrimCode primCode = {.poly = {.renderCode = RenderCode_Polygon, .quad = 1, .semiTransparency = 1}};
	Color primColor = *color;
	primColor.code = primCode;
	p->p.colorCode = primColor;

	s16 topX = r->x;
	s16 topY = r->y;
	s16 bottomX = r->x + r->w;
	s16 bottomY = r->y + r->h;
	p->p.v[0].pos.x = topX;
	p->p.v[0].pos.y = topY;
	p->p.v[1].pos.x = bottomX;
	p->p.v[1].pos.y = topY;
	p->p.v[2].pos.x = topX;
	p->p.v[2].pos.y = bottomY;
	p->p.v[3].pos.x = bottomX;
	p->p.v[3].pos.y = bottomY;

// NOTE(aalhendi): CTR_NATIVE keeps PsyCross display-area drawing enabled.
#ifdef CTR_NATIVE
	((TPage *)p)->texpage.drawDisplayArea = 1;
#endif

	AddPrimitive(p, ot);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021894-0x80021984.
void CTR_Box_DrawSolidBox(RECT *r, Color color, u_long *ot)
{
	PolyF4 *p;
	GetPrimMem(p);
	if (p == nullptr)
	{
		return;
	}

	const PrimCode primCode = {.poly = {.renderCode = RenderCode_Polygon, .quad = 1}};
	color.code = primCode;
	p->colorCode = color;

	s16 topX = r->x;
	s16 topY = r->y;
	s16 bottomX = r->x + r->w;
	s16 bottomY = r->y + r->h;
	p->v[0].pos.x = topX;
	p->v[0].pos.y = topY;
	p->v[1].pos.x = bottomX;
	p->v[1].pos.y = topY;
	p->v[2].pos.x = topX;
	p->v[2].pos.y = bottomY;
	p->v[3].pos.x = bottomX;
	p->v[3].pos.y = bottomY;

	AddPrimitive(p, ot);
}
