
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include "uncompress.h"
#include "LzmaDecode.h"
#define malloc vmalloc
#define free vfree
#if 0
static void dumpHex(unsigned char *start, int len)
{
    unsigned char *ptr = start,
    		  *end = start + len;
    int i;

	while (ptr < end) {
	    for (i=0;i<16 && ptr < end;i++)
		    printk("%4x", *ptr++);
	    printk("\n");
	}
}
#endif
int decompress_lzma_7z(unsigned char* s, unsigned size, unsigned char* d, unsigned *c_byte,char *buffer)
{
	int lc, lp, pb;
	unsigned char prop0 ;
        unsigned char properties[5];
	unsigned long lzmaInternalSize;
	char *lzmaInternalData=buffer;
	int outSizeProcessed;
	int result =0;
	int outSize=0,ii;
	unsigned char b; 
	
	memcpy(properties,s,sizeof(properties));
	
	
	for (ii = 0; ii < 4; ii++)
	{
		b=*(s+sizeof(properties)+ii);
		outSize += (unsigned int)(b) << (ii * 8);
	}

	prop0 = properties[0];
	if (prop0 >= (9*5*5))
	{
		return 1;
	}
	for (pb = 0; prop0 >= (9 * 5); 
	pb++, prop0 -= (9 * 5));
	for (lp = 0; prop0 >= 9; 
	lp++, prop0 -= 9);
	lc = prop0;
	
	lzmaInternalSize = 
		(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp))) *  sizeof(CProb)
;
	if(lzmaInternalSize > LZMA_BUFFER)
 		lzmaInternalData = malloc(lzmaInternalSize);
	
//	printk("lzmaInternalSize=%d\n",lzmaInternalSize);

	result = LzmaDecode(lzmaInternalData, lzmaInternalSize,
        	lc, lp, pb, s+13, size-13,d, outSize, &outSizeProcessed);
	
        *c_byte=outSizeProcessed;
	
	if(lzmaInternalSize > LZMA_BUFFER)
		free(lzmaInternalData);

	return 0;
}


