#include "string.h"
////////////////////////////////////// FUNCTIONS FOR TYPE CONVERSION ////////////////////////////////////
size_t strlen2(const unsigned char *stringa);

unsigned char *strcpy2(unsigned char *dst, const unsigned char *src);
unsigned char *strncpy2(unsigned char *dst, const unsigned char *src, size_t len);

int strcmp2(const unsigned char *first, const unsigned char *second);
int strncmp2(const unsigned char *first, const unsigned char *second, size_t n);

unsigned char *strcat2(unsigned char *destination, const unsigned char *source);
unsigned char *strncat2(unsigned char *destination, const unsigned char *source, size_t len);
