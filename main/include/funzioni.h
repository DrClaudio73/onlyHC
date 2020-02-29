// ********************************************
//#define DR_REG_RNG_BASE 0x3ff75144
typedef struct MioDB
{
    unsigned char DATA[9];
    unsigned char ORA[7];
    unsigned int num_APRI_received;
    unsigned int num_TOT_received;
} miodb_t;

////////////////////////////////////////////// CORE FUNCTIONS //////////////////////////////////////////////
