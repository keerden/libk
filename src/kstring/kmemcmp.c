#include <kstring.h>

/*** memcmp ***
 * 
 * The memcmp function compares the first n characters of the object pointed to by s1 to
 * the first n characters of the object pointed to by s2. 310)
 * 
 * Returns
 * 
 * The memcmp function returns an integer greater than, equal to, or less than zero,
 * accordingly as the object pointed to by s1 is greater than, equal to, or less than the object
 * pointed to by s2.
 * 
 ***/


int kmemcmp(const void* s1, const void* s2, size_t n)
{
	const unsigned char* a = (const unsigned char*) s1;
	const unsigned char* b = (const unsigned char*) s2;

	while(n--) {
		if(*a < *b)
			return -1;
		else if(*a > *b)
			return 1;
		a++;
		b++;
	}
	return 0;
}
