// max buffer length
#define LINE_MAX 2400
#define FIELD_MAX 255
#define NUMCELL_MAX 20

////////////////////////////////////////////// MESSAGE HANDLING FUNCTIONS //////////////////////////////////////////////
int strcmpACK(const unsigned char *rcv, const unsigned char *cmd);
unsigned char calcola_CRC(int totale);
void pack_str(unsigned char *msg_to_send, const unsigned char *valore, unsigned char *k, int *totale);
void pack_num(unsigned char *msg_to_send, unsigned char valore, unsigned char *k, int *totale);
unsigned char *pack_msg(unsigned char addr_from, unsigned char addr_to, const unsigned char *cmd, const unsigned char *param, unsigned char rep_counts);
unsigned char unpack_msg(const unsigned char *msg, unsigned char allowed_addr_from, unsigned char allowed_addr_to, unsigned char **cmd, unsigned char **param, unsigned char *acknowldged_rep_counts);
