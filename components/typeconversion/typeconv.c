#include "typeconv.h"
////////////////////////////////////// FUNCTIONS FOR TYPE CONVERSION ////////////////////////////////////
size_t strlen2(const unsigned char* stringa){
    return(strlen((const char*) stringa));
}


unsigned char* strcpy2(unsigned char* dst , const unsigned char* src){
    return((unsigned char*) strcpy((char*) dst, (const char*) src));
}

unsigned char* strncpy2(unsigned char* dst , const unsigned char* src, size_t len){
    return((unsigned char*) strncpy((char*) dst, (const char*) src, len));
}

int strcmp2(const unsigned char* first, const unsigned char* second){
    return (strcmp((const char*) first, (const char*) second));
}

int strncmp2(const unsigned char* first, const unsigned char* second, size_t n){
    return (strncmp((const char*) first, (const char*) second, n));
}

unsigned char* strcat2(unsigned char * destination, const unsigned char * source){
    return (unsigned char*)strcat((char*) destination, (const char*) source);
}
unsigned char* strncat2(unsigned char * destination, const unsigned char * source, size_t len){
    return (unsigned char*)strncat((char*) destination, (const char*) source, len);
}


