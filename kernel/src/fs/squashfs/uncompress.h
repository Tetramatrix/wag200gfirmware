
#define LZMA_BUFFER 16384 //(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp))) *  sizeof(CProb);

int decompress_lzma_7z(unsigned char* s, unsigned size, unsigned char* d, unsigned *c_byte,char *buffer);

