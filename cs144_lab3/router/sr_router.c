/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance *sr) {
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);

    /* Add initialization code here! */

} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance *sr,
                     uint8_t *packet/* lent */,
                     unsigned int len,
                     char *interface/* lent */) {
    /* REQUIRES */
    assert(sr);
    assert(packet);
    assert(interface);

    printf("*** -> Received packet of length %d \n", len);

    /* fill in code here */

    // check whether packet big enough
    if (length < sizeof(sr_ethernet_hdr_t)){
        return;
    }

    struct sr_ethernet_hdr *e_hdr = 0;
    e_hdr = (struct sr_ethernet_hdr *) packet;
    if (e_hdr->ether_type == htons(ethertype_arp)) {
        //check and handle arp packet
        sr_handle_arp_packet(sr, packet, interface);
        return;
    } else if (e_hdr->ether_type == htons(ethertype_ip)) {
        //check and handle ip packet
        sr_handle_ip_packet(sr, packet, len, interface);
        return;
    } else {
        // neither arp or ip
    }
}/* end sr_ForwardPacket */

void sr_handle_arp_packet(struct sr_instance *sr,
                          uint8_t *packet/* lent */,
                          char *interface/* lent */) {
    struct sr_arp_hdr *a_hdr = 0;
    a_hdr = (struct sr_arp_hdr *) (packet + sizeof(struct sr_ethernet_hdr));
    if (a_hdr->ar_op == htons(arp_op_reply)) {
        // need to implement arp reply
        sr_handle_arp_reply(sr, a_hdr);
    } else if (a_hdr->ar_op == hton(arp_op_request))
        // need to implement arp request
        sr_handle_arp_req(sr, a_hdr, interface);
}


void sr_handle_ip_packet(struct sr_instance *sr,
                         uint8_t *packet/* lent */,
                         unsigned int len,
                         char *interface/* lent */) {
    struct sr_ip_hdr *ip_hdr = 0;
    ip_hdr = (struct sr_ip_hdr) (packet + sizeof(struct sr_ethernet_hdr));

    // check sum
    unit16_t ip_header_checksum = ip_hdr -> ip_sum;
    unit16_t calculate_checksum = cksum(packet, ip_hdr->ip_hl * 4)
    if (ip_header_checksum != calculate_checksum){
        // check sum error
        return;
    }
    ip_hdr -> ip_sum = calculate_checksum;

    // check ttl
    if (ip_hdr->ip_ttl <= 0)
        return;

    // check whether ip packet big enough
    if (length < sizeof(sr_ip_hdr_t)){
        return;
    }

    // it's for me
    // check if the destination of the ip packet is in the list
    if (sr_ip_des_inlist(sr, ip_hdr->ip_dst)){
        if (ip_hdr->ip_p == ip_protocol_icmp){
            // handle icmp packet

            // need to do icmp check sum first !!!

            sr_icmp_hdr *icmp_hdr = (struct sr_icmp_hdr*)((uint8_t*)(packet + sizeof(struct sr_ethernet_hdr)) + sizeof(struct sr_ip_hdr));
            if (icmp_hdr->icmp_type == 8){
                // handle type 8 icmp echo req
                send_icmp_echo_reply(sr, packet, len)
            }
            else{
                return;
            }

        }
        // or it is TCP/UDP
        else if(ip_hdr->ip_p == ip_protocol_tcp || ip_hdr->ip_p == ip_protocol_udp){
            // send type 3 icmp port unreachable
            send_icmp_port_unreachable(sr, packet, len)
        }
    }

    // not for me
    else{
        // check whether ttl time exceed
        if (ip_hdr->ip_ttl - 1 <= 0) {
            // send type 11 icmp time exceed
            send_icmp_time_exceeded(sr, packet, len)
        }
        else{
            // check routing table preform LPM
            ip_hdr->ip_ttl -= 1;
            ip_hdr->ip_sum = 0;
            ip_hdr->ip_sum = cksum(ip_hdr, sizeof(sr_ip_hdr_t));
            ip_dst = ip_hdr->ip_dst;
            if(/* if routing table not match */){
                // routing table not match
                // send type 3 icmp net unreachable
                send_icmp_net_unreachable(sr, packet, len)
            }
            else{
                if(/* check arp cache hit */){
                    // if hit the entry, send the frame to next hope

                }
                else{
                    // send arp request
                }
            }
        }
    }




}





void send_icmp_echo_reply(struct sr_instance *sr, uint8_t *packet, unsigned int len){
    uint8_t* icmp_echo_reply_packet = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));

    // fill in packet and send icmp packet reply
    // fill with type 0 3 11
}


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
	//Get Ethernet header addresses
	struct sr_ethernet_hdr* new_ether_hdr = (sr_ethernet_hdr_t*)new_packet;
	struct sr_ethernet_hdr* ether_hdr = (sr_ethernet_hdr_t*) packet;;
	struct sr_if* sr_if = sr_if = sr_get_interface(sr, iface);

	/* Set Ethernet header*/
	new_ether_hdr->ether_type = ether_hdr->ether_type;
	memcpy(new_ether_hdr->ether_shost, sr_if->addr, ETHER_ADDR_LEN);


	/*Get IP header addresses */
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

