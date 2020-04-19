#ifndef _CRIPTER_FUNCS_
#define _CRIPTER_FUNCS_

#include <stddef.h>
#include "sdkconfig.h"
// max buffer length
#define LINE_MAX 2400
#define FIELD_MAX 255
#define NUMCELL_MAX 20

#define CONF_KEY_SEED CONFIG_KEY_SEED
#define CONF_ACA_NID CONFIG_ACA
#define CONF_IV CONFIG_IV
#define MAX_CRIPTER_BLOCKS 12
////////////////////////////////////////////// CRIPTER FUNCTIONS //////////////////////////////////////////////
unsigned char *my_hexstr_to_bytes(unsigned char *buffer, const unsigned char *instr, size_t bytes_size, const char *name);
void toUpperstr(unsigned char *instr, size_t len, const char *name);
int my_pad_from_hexstr(unsigned char P[MAX_CRIPTER_BLOCKS][16], unsigned char *instr, size_t strlen);
int my_pad_direct(unsigned char P[MAX_CRIPTER_BLOCKS][16], unsigned char *instr, size_t strlen);
int crittalinea(unsigned char *linecripted, const unsigned char *linein);
int decrittalinea(unsigned char *linedecripted, const unsigned char *lineincripted);
int eval_IV(unsigned char *IV);
int eval_key(unsigned char *key);

#endif //_CRIPTER_FUNCS_