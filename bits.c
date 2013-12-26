#include "bits.h"

byte bitset_getFirstFree(byte set) {
	byte n = 1;
	byte i = 0;
	while(n < 0x80) {
		if(!(set & n)) {
			return i;
		}

		i++;
		n <<= 1;
	}

	if(!(set & n)) {
		return i;
	} else {
		return -1;
	}
}

byte bitset_test(byte set, byte bitnum) {
	if(bitnum < 8) {
		return (set & (1 << bitnum)) >> bitnum;
	} else {
		return 0;
	}
}