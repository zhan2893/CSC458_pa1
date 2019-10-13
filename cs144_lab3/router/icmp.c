#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sr_utils.h"
#include "sr_icmp.h"


void sr_new_icmp_message(struct sr_instance* sr, uint8_t type, uint8_t code, uint8_t* packet, unit8_t* new_packet, const char* iface) {

	/*If icmp is type 3 or type 11*/
	if (type == 11) {
		struct sr_icmp_hdr* icmp_hdr = (sr_icmp_hdr_t*)(packet_to_send + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));;
			sr_set_headers(sr, new_packet, packet, sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t), iface, uint8_t type, uint8_t code );
	}
	else {
		struct sr_icmp_t3_hdr* icmp_hdr = (sr_icmp_t3_hdr_t*)(packet_to_send + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
		sr_set_headers(sr, new_packet, packet, sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t), iface, uint8_t type, uint8_t code);
	
	}

	icmp_hdr->icmp_type = type;
	icmp_hdr->icmp_code = code;
	icmp_hdr->unused = 0;
	icmp_hdr->next_mtu = 0;


	struct sr_ip_hdr* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
	/*IP header + the first 8 bytes of the original datagram's data. */
	memcpy((uint8_t*)icmp_hdr + 8, ip_hdr, sizeof(sr_ip_hdr_t) + 8)

	icmp_hdr->icmp_sum = 0;
	/* check sum*/
	if (type == 3) {
		icmp_hdr->icmp_sum = cksum(icmp_hdr, sizeof(sr_icmp_t3_hdr_t));
	}
	else {
		icmp_hdr->icmp_sum = cksum(icmp_hdr, sizeof(sr_icmp_hdr_t));
	}
	

}


void sr_set_headers(struct sr_instance* sr, uint8_t* new_packet, uint8_t* packet, unsigned int len, const char* iface, uint8_t type, uint8_t code) {
	/* Get Ethernet header addresses */
	struct sr_ethernet_hdr* new_ether_hdr = (sr_ethernet_hdr_t*)new_packet;
	struct sr_ethernet_hdr* ether_hdr = (sr_ethernet_hdr_t*) packet;;
	struct sr_if* sr_if = sr_if = sr_get_interface(sr, iface);

	/* Set Ethernet header*/
	new_ether_hdr->ether_type = ether_hdr->ether_type;
	memcpy(new_ether_hdr->ether_shost, sr_if->addr, ETHER_ADDR_LEN);


	/* Get IP header addresses */
	struct sr_ip_hdr* new_ip_hdr = (sr_ip_hdr_t*)(new_packet + sizeof(sr_ethernet_hdr_t));
	struct sr_ip_hdr* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));

	/* Set IP header*/
	new_ip_hdr->ip_tos = ip_hdr->ip_tos;
	new_ip_hdr->ip_len = htons(len);
	new_ip_hdr->ip_id = 0;
	new_ip_hdr->ip_off = 0;
    new_ip_hdr->ip_ttl = 64; 
	new_id_hdr->ip_p = ip_protocol_icmp;
	new_ip_hdr->ip_sum =  cksum(ip_hdr, sizeof(sr_ip_hdr_t));
	new_ip_hdr->ip_dst = ip_hdr->ip_src;
	if (type == 3 && code == 3) {
		new_ip_hdr->ip_src =  ip_hdr->ip_dst;
	}
	else {
		new_ip_hdr->ip_src = sr_if->ip;
	}
}

void sr_new_icmp_reply(struct sr_instance* sr, uint8_t* new_packet, uint8_t* packet, const char* iface) {
	/*Get headers Addresses*/
	struct sr_ip_hdr* new_ip_hdr = (sr_ip_hdr_t*)(new_packet + sizeof(sr_ethernet_hdr_t));
	struct sr_icmp_hdr* new_icmp_hdr = (sr_icmp_hdr_t*)(new_packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));

	struct sr_ip_hdr* ip_hdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
	struct sr_icmp_hdr* icmp_hdr = (sr_icmp_t8_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));

	unsigned int len =  ntohs(ip_hdr->ip_len) - sizeof(sr_ip_hdr_t) - sizeof(sr_icmp_hdr_t);

	/* Set headers */
	sr_set_headers(sr, new_packet , packet, ntohs(ip_hdr->ip_len), iface, 3,3 );
	new_ip_hdr->ip_src = ip_hdr->ip_dst;
	new_ip_hdr->ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
	new_icmp_hdr->icmp_type = 0;
	new_icmp_hdr->icmp_code = 0;
	new_icmp_hdr->icmp_sum = 0;

	/* Check sum*/
	memcpy((uint8_t*)new_icmp_hdr + sizeof(sr_icmp_hdr_t), (uint8_t*)icmp_hdr + sizeof(sr_icmp_hdr_t), len);
	new_icmp_hdr->icmp_sum = cksum(new_icmp_hdr, sizeof(sr_icmp_hdr_t) + len);

}