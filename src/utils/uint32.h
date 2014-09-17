#ifndef __UINT32_H
#define __UINT32_H

void uint32_pack(unsigned char s[4], unsigned int* addr)
{
	*addr = 0;
	*addr += (s[0] << 24) & 0xf000;
	*addr += (s[1] << 16) & 0x0f00;
	*addr += (s[2] << 8)  & 0x00f0;
	*addr +=  s[3]        & 0x000f;
}

#endif

