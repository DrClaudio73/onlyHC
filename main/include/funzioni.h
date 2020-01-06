#define ADDR_MASTER_STATION 1
#define ADDR_SLAVE_STATION 2
#define ADDR_MOBILE_STATION 3
#define DEBUG 1


unsigned char calcola_CRC(int totale);
void pack_str(unsigned char* msg_to_send, const unsigned char* valore, unsigned char* k, int* totale);
void pack_num(unsigned char* msg_to_send, unsigned char valore, unsigned char* k, int* totale);

unsigned char* pack_msg(unsigned char uart_controller, unsigned char addr_from, unsigned char addr_to, const unsigned char* cmd, const unsigned char* param);

unsigned char unpack_msg(const unsigned char* msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char** cmd, unsigned char** param);
