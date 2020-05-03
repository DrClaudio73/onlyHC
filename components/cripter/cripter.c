#include "cripter.h"
//IO Functions
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include "string.h"
#include "esp_log.h"
#include "typeconv.h"

//mbed library
//#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/sha1.h"
#include "mbedtls/aes.h"

static const char *TAG_C = "CRIPTER";
////////////////////////////////////////////// CRIPTING FUNCTIONS //////////////////////////////////////////////
//converts an input string in uppercase letters only
//input "instr" and its "len" length
//"name" is used ony for displaying debugging infos
//output "instr" is modified in uppercase letters only
void toUpperstr(unsigned char *instr, size_t len, const char *name)
{
    unsigned char *p = instr;
    int instrlen = 0;
    //mbedtls_printf("%s--toUpperstr(): - name: %s , instr: %s\n len: %d\n", TAG_C, name, instr,len);
    fflush(stdout);
    while (instrlen < (int)len)
    {
        instr[instrlen++] = toupper(*p); //*p;
        p++;
    }
    //mbedtls_printf("%s--UPPER(%s): %s\r\n", TAG_C, name, instr);
    fflush(stdout);
    return;
}

// converts a string representing a byte array in hexadecimal notation (two characters for each byte so "A3" will be evaluated as 0xa3)
// hence instr has double length with respect to output buffer
unsigned char *my_hexstr_to_bytes(unsigned char *buffer, const unsigned char *instr, size_t bytes_size, const char *name)
{
    unsigned char *p = instr;
    int instrlen = 0;
    int n = 0;
    while ((sscanf((char *)p, "%02X", &n) > 0) && (instrlen < (int)bytes_size))
    {
        buffer[instrlen++] = (unsigned char) n;
        p += 2;
    }
    return buffer;
}

//breaks down in 16 bytes blocks an input string representing the ASCII coding of a bytes array  
//input: a string representing the ASCII coding of a bytes array
//output: a padded P array of bytes blocks, each block being 16 bytes long
//return the number of blocks
int my_pad_from_hexstr(unsigned char P[MAX_CRIPTER_BLOCKS][16], unsigned char *instr, size_t strlen)
{
    unsigned char *p = instr;
    char name[25];
    char n_blocks = 0;
    if (strlen % 32 == 0)
    {
        mbedtls_printf("Caso 1  \n");
        for (int i = 0; i < (int)strlen / 32; i++)
        {
            n_blocks++;
            sprintf(name, "p[%d]", i);
            my_hexstr_to_bytes(P[i], p, 16, name);
            p = p + 32;
        }
        mbedtls_printf("Caso 1 - len: %d, n_blocks:  %d\n", strlen, n_blocks);
    }
    else
    {
        //mbedtls_printf("Caso 2  \n");
        for (int i = 0; i <= (int)strlen / 32; i++)
        {
            n_blocks++;
            sprintf(name, "p[%d]", i);
            my_hexstr_to_bytes(P[i], p, 16, name); //questo serve se ciascun byte è inizialmente rappresentato con codifica ASCII l'esadecimale della codifica ASCII
            p = p + 32;
        }
        mbedtls_printf("Caso 2 - len: %d, n_blocks:  %d\n", strlen, n_blocks);
    }
    return n_blocks;
}

//breaks down in 16 bytes blocks an input bytes array  
//input: a bytes array
//output: a padded P array of bytes blocks, each block being 16 bytes long
//return the number of blocks
int my_pad_direct(unsigned char P[MAX_CRIPTER_BLOCKS][16], unsigned char *instr, size_t strlen)
{
    unsigned char *p = instr;
    //char name[25];
    char n_blocks = 0;
    if (strlen % 16 == 0)
    {
        //mbedtls_printf("Caso 1  \n");
        for (int i = 0; i < (int)strlen / 16; i++)
        {
            n_blocks++;
            //sprintf(name, "p[%d]", i);
            for (int h = 0; h < 16; h++)
            {
                P[i][h] = p[h];
            }
            p = p + 16;
        }
        mbedtls_printf("Caso 1(direct) - len: %d, n_blocks:  %d\n", strlen, n_blocks);
    }
    else
    {
        //mbedtls_printf("Caso 2  \n");
        for (int i = 0; i <= (int)strlen / 16; i++)
        {
            n_blocks++;
            //sprintf(name, "p[%d]", i);
            for (int h = 0; h < 16; h++)
            {
                P[i][h] = p[h];
            }
            p = p + 16;
        }
            mbedtls_printf("Caso 2(direct) - len: %d, n_blocks:  %d\n", strlen, n_blocks);
    }
    return n_blocks;
}

int crittalinea(unsigned char *linecripted, const unsigned char *linein)
{
    int n_blocks = 0;
    unsigned char P[MAX_CRIPTER_BLOCKS][16];
    unsigned char C[MAX_CRIPTER_BLOCKS][16];
    memset(P, 0, sizeof(P)); //P will host the bytes to be encypted
    memset(C, 0, sizeof(C)); //C will host the bytes encrypted

    unsigned char line1[LINE_MAX];
    memset(line1, 0, sizeof(line1));
    strcpy2(line1, linein);
    n_blocks = my_pad_direct(P, line1, strlen2(line1));

    for (int j = 0; j < n_blocks; j++)
    {
        mbedtls_printf("\n  P[%d] = ", j);
        for (int i = 0; i < 16; i++)
            mbedtls_printf("%02X", P[j][i]);
    }
    mbedtls_printf("\n");
    // encrypting
    unsigned char key[16];
    memset(key, 0, sizeof(key));
    eval_key(key);
    unsigned char IV[16];
    memset(IV, 0, sizeof(IV));
    eval_IV(IV);

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    for (int j = 0; j < n_blocks; j++)
    {
        if (j == 0)
        {
            for (int i = 0; i < 16; i++)
                C[j][i] = (unsigned char)(IV[i]);
        }
        else
        {
            for (int i = 0; i < 16; i++)
                C[j][i] = (unsigned char)(C[j - 1][i]);
        }
        for (int i = 0; i < 16; i++)
            C[j][i] = (unsigned char)(P[j][i] ^ C[j][i]);
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, C[j], C[j]);
    }

    mbedtls_printf("\n  \033[1;36mAES_ECB('%s') = \n", line1);

    for (int j = 0; j < n_blocks; j++)
    {
        mbedtls_printf("\n  C[%d] = ", j);
        for (int i = 0; i < 16; i++)
            mbedtls_printf("\033[1;36m%02X", C[j][i]);
    }
    mbedtls_printf("\r\n");

    for (int j = 0; j < n_blocks; j++)
    {
        for (int l = 0; l < 16; l++)
        {
            sprintf((char *)linecripted + j * 32 + l*2, "%02X", C[j][l]);
            //my_hexstr_to_bytes(linecripted+j*16,D[j],16,"linedecripted");
        }
    }
    mbedtls_printf("    \033[1;36m-linecrypted: %s\r\n", linecripted);
    ESP_LOG_BUFFER_HEXDUMP(TAG_C, linecripted, strlen2(linecripted), ESP_LOG_INFO);
    mbedtls_aes_free(&aes_ctx);
    return MBEDTLS_EXIT_SUCCESS;
}

//decript CBC mode an input string representing the ASCCII coding of coded byte stream
int decrittalinea(unsigned char *linedecripted, const unsigned char *lineincripted)
{
    int n_blocks = 0;
    unsigned char C[MAX_CRIPTER_BLOCKS][16];
    memset(C, 0, sizeof(C)); //D will host the bytes decrypted
    unsigned char D[MAX_CRIPTER_BLOCKS][16];
    memset(D, 0, sizeof(D)); //D will host the bytes decrypted

    unsigned char line1[LINE_MAX];
    memset(line1, 0, sizeof(line1));
    strcpy2(line1, lineincripted);
    n_blocks = my_pad_from_hexstr(C, line1, strlen2(line1));

    for (int j = 0; j < n_blocks; j++)
    {
        mbedtls_printf("\n  \033[1;36mC[%d] = ", j);
        for (int i = 0; i < 16; i++)
            mbedtls_printf("\033[1;36m%02X", C[j][i]);
    }
    mbedtls_printf("\n");

    unsigned char key[16];
    memset(key, 0, sizeof(key));
    eval_key(key);
    unsigned char IV[16];
    memset(IV, 0, sizeof(IV));
    eval_IV(IV);

    //decrypting
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
    for (int j = 0; j < n_blocks; j++)
    {
        for (int i = 0; i < 16; i++)
            D[j][i] = (unsigned char)(C[j][i]);

        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, D[j], D[j]);

        if (j == 0)
        {
            for (int i = 0; i < 16; i++)
                D[j][i] = (unsigned char)(D[j][i] ^ IV[i]);
        }
        else
        {
            for (int i = 0; i < 16; i++)
                D[j][i] = (unsigned char)(D[j][i] ^ C[j - 1][i]);
        }
    }

    for (int j = 0; j < n_blocks; j++)
    {
        mbedtls_printf("\n  \033[1;33mD[%d] = ", j);
        for (int i = 0; i < 16; i++)
            mbedtls_printf("\033[1;33m%02X", D[j][i]);
    }
    mbedtls_printf("\n\n");

    for (int j = 0; j < n_blocks; j++)
    {
        for (int l = 0; l < 16; l++)
        {
            linedecripted[j * 16 + l] = D[j][l];
        }
    }
    mbedtls_printf("    \033[1;31m-linedecrypted: %s\r\n", linedecripted);
    ESP_LOG_BUFFER_HEXDUMP(TAG_C, linedecripted, strlen2(linedecripted), ESP_LOG_INFO);
    mbedtls_aes_free(&aes_ctx);
    return MBEDTLS_EXIT_SUCCESS;
}

int eval_key(unsigned char *key)
{
    int ret;
    unsigned char digest[20];
    memset(digest, 0, sizeof(digest));

    unsigned char KEY_SEED[40];
    memset(KEY_SEED, 0, sizeof(KEY_SEED));
    strcpy2(KEY_SEED, (unsigned char *)CONF_KEY_SEED);
    toUpperstr(KEY_SEED, strlen2(KEY_SEED), "KEY_SEED");

    unsigned char ACA_NID[17];
    unsigned char BLOB_STR[512];
    unsigned char BLOB_BYTES[512];

    memset(ACA_NID, 0, sizeof(ACA_NID));
    memset(BLOB_STR, 0, sizeof(BLOB_STR));
    memset(BLOB_BYTES, 0, sizeof(BLOB_BYTES));

    unsigned char ACA[13];
    memset(ACA, 0, sizeof(ACA));
    strcpy2(ACA, (unsigned char *)CONF_ACA_NID);

    if (strlen2(ACA) == 12)
    {
        //mbedtls_printf("Correct NID (%s) length: %d\r\n", ACA, strlen2(ACA));
        toUpperstr(ACA, strlen2(ACA), "ACA");
        strcpy2(ACA_NID, ACA);
        //mbedtls_printf("ACA_NID: %s\r\n", ACA);
    }
    else
    {
        mbedtls_printf("NID (%s) length (%d) is NOT correct\r\n", ACA, strlen2(ACA));
        return (MBEDTLS_EXIT_FAILURE);
    }

    strcpy2(BLOB_STR, (unsigned char *)"PP");
    strcat2(BLOB_STR, KEY_SEED);
    strcat2(BLOB_STR, ACA_NID);
    //mbedtls_printf("BLOB_STR: %s length (%d).\r\n", BLOB_STR, strlen2(BLOB_STR));
    //mbedtls_printf("\n  SHA1('%s') = ", BLOB_STR);

    if ((ret = mbedtls_sha1_ret((unsigned char *)BLOB_STR, strlen2(BLOB_STR), digest)) != 0)
        return (MBEDTLS_EXIT_FAILURE);

    /*for (i = 0; i < 20; i++)
        mbedtls_printf("%02X", digest[i]);

    mbedtls_printf("\n");
    mbedtls_printf("\n  key = ");
    for (i = 0; i < 16; i++)
    {
        key[i] = digest[i];
        mbedtls_printf("%02X", key[i]);
    }
    mbedtls_printf("\r\n");*/
    return MBEDTLS_EXIT_SUCCESS;
}

int eval_IV(unsigned char *IV)
{
    unsigned char VIN[33];
    memset(VIN, 0, sizeof(VIN));
    strcpy2(VIN, (unsigned char *)CONF_IV);
    toUpperstr(VIN, strlen2(VIN), "VIN");
    my_hexstr_to_bytes(IV, VIN, 16, "VIN");
    /*mbedtls_printf("\n  IV = ");
    for (int i = 0; i < 16; i++)
    {
        mbedtls_printf("%02X", IV[i]);
    }
    mbedtls_printf("\r\n");*/
    return MBEDTLS_EXIT_SUCCESS;
}