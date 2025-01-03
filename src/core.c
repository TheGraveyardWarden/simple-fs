#include "core.h"
#include "fs.h"

void bitmap_print(u8 *bm)
{
	LOG("--------- [ Bitmap View ] ---------\n");
	for (int _i = 0, _z = 1; _i < BLOCK_SIZE; _i++, _z++)
	{
		LOG("0b");
		for (int _j = 7; _j > -1; _j--)
		{
			LOG("%d", (bm[_i] & (1 << _j)) == 0 ? 0 : 1);
		}
	if (_z % 4 == 0) LOG("\n");
		else LOG("\t");
	}
	LOG("----------------------------------\n");
}
