#ifndef SR_ICMP_H
#define SR_ICMP_H



void sr_new_icmp(struct sr_instance* sr, uint8_t type, uint8_t code, uint8_t* packet, unit8_t* new_packet, const char* iface);
void sr_set_headers(struct sr_instance* sr, uint8_t* new_packet, uint8_t* packet, unsigned int len, const char* iface, uint8_t type, uint8_t code);
void sr_new_icmp_reply(struct sr_instance* sr, uint8_t* new_packet, uint8_t* packet, const char* iface);

#endif