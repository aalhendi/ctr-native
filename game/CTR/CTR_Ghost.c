#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80022234-0x800222e0.
void CTR_ScrambleGhostString(char *dst, const char *src)
{
	u8 inputByte = *src;

	while (inputByte != '\0')
	{
		u32 key = 0;

		if (inputByte < 4)
		{
			inputByte = *src++;
			key = (u32)inputByte << 8;
		}

		inputByte = *src++;
		key |= inputByte;

		u16 *entry = &data.ghostScrambleData[0];
		while (entry[1] != 0xffff)
		{
			if (entry[1] == (key & 0xffff))
			{
				u16 encoded = entry[0];
				if ((encoded & 0xff00) != 0)
				{
					*dst++ = encoded >> 8;
					*dst++ = encoded;
					break;
				}
			}

			entry += 2;
		}

		inputByte = *src;
	}

	*dst = '\0';
}
