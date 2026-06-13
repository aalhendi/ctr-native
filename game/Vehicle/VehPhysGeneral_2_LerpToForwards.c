#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060488-0x800605a0.
int VehPhysGeneral_LerpToForwards(struct Driver *d, int param_2, int param_3, int param_4)
{
	bool bVar1;
	u32 uVar2;
	int iVar3;

	bVar1 = false;
	d->unk3CA = 0;
	if ((param_4 < 0) || ((param_4 == 0 && (param_2 < 0))))
	{
		bVar1 = true;
		param_2 = CTR_MipsNegLo(param_2);
		param_3 = CTR_MipsNegLo(param_3);
		param_4 = CTR_MipsNegLo(param_4);
	}
	iVar3 = 0;

	if (d->set_0xF0_OnWallRub != 0xf0)
	{
		if (param_4 < param_2)
		{
			if (d->const_modelRotVelMax < param_2)
			{
				uVar2 = CTR_MipsSubLo(CTR_MipsSll((u8)d->unk458, 4), (u8)d->unk458);
			}
			else
			{
				uVar2 = (u8)d->unk458;
			}
			iVar3 = VehPhysGeneral_LerpQuarterStrength(uVar2, CTR_MipsSubLo(param_2, param_4));
			iVar3 = CTR_MipsNegLo(iVar3);
		}
		else
		{
			if (param_2 < param_4)
			{
				if (param_2 < 0)
				{
					iVar3 = VehPhysGeneral_LerpQuarterStrength((u8)d->unk459, CTR_MipsSubLo(param_4, param_2));
				}
				else
				{
					iVar3 = VehPhysGeneral_LerpQuarterStrength((u8)d->angleMaxCounterSteer, CTR_MipsSubLo(param_4, param_2));
					d->unk3CA = (s16)param_4;
				}
			}
		}
	}

	// Interpolate rotation by speed
	iVar3 = VehCalc_InterpBySpeed(param_3, d->unk45a, iVar3);
	if (bVar1)
	{
		iVar3 = CTR_MipsNegLo(iVar3);
	}
	return iVar3;
}
