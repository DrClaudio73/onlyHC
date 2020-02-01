#include "typeconv.h"
////////////////////////////////////// FUNCTIONS FOR TYPE CONVERSION ////////////////////////////////////
size_t strlen2(const unsigned char* stringa){
    return(strlen((const char*) stringa));
}

unsigned char* strcpy2(unsigned char* dst , const unsigned char* src){
    return((unsigned char*) strcpy((char*) dst, (const char*) src));
}

int strcmp2(const unsigned char* first, const unsigned char* second){
    return (strcmp((const char*) first, (const char*) second));
}

unsigned char* strcat2(unsigned char * destination, const unsigned char * source){
    return (unsigned char*)strcat((char*) destination, (const char*) source);
}

int strncmp2(const unsigned char* first, const unsigned char* second, size_t n){
    return (strncmp((const char*) first, (const char*) second, n));
}