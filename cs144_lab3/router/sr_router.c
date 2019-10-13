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
        /* check and handle arp packet */
        sr_handle_arp_packet(sr, packet, interface);
        return;
    } else if (e_hdr->ether_type == htons(ethertype_ip)) {
        //check and handle ip packet
        sr_handle_ip_packet(sr, packet, len, interface);
        return;
    } else {
        /* neither arp or ip */
    }
}/* end sr_ForwardPacket */

void sr_handle_arp_packet(struct sr_instance *sr,
                          uint8_t *packet/* lent */,
                          char *interface/* lent */) {
    struct sr_arp_hdr *a_hdr = 0;
    a_hdr = (struct sr_arp_hdr *) (packet + sizeof(struct sr_ethernet_hdr));
    if (a_hdr->ar_op == htons(arp_op_reply)) {
        /* need to implement arp reply */
        sr_handle_arp_reply(sr, a_hdr);
    }
    else{
        /* need to implement arp request */
        sr_handle_arp_req(sr, a_hdr, interface);
    }
}


void sr_handle_ip_packet(struct sr_instance *sr,
                         uint8_t *packet/* lent */,
                         unsigned int len,
                         char *interface/* lent */) {
    struct sr_ip_hdr *ip_hdr = 0;
    uint32_t ip_dst = 0;
    uint32_t next_hop_ip = 0;
    struct sr_arpentry *entry = 0;
    char iface_out[sr_IFACE_NAMELEN];
    ip_hdr = (struct sr_ip_hdr) (packet + sizeof(struct sr_ethernet_hdr));

    /* check sum */
    unit16_t ip_header_checksum = ip_hdr -> ip_sum;
    unit16_t calculate_checksum = cksum(packet, ip_hdr->ip_hl * 4)
    if (ip_header_checksum != calculate_checksum){
        /* check sum error */
        return;
    }
    ip_hdr -> ip_sum = calculate_checksum;

    /* check ttl */
    if (ip_hdr->ip_ttl <= 0)
        return;

    /* check whether ip packet big enough */
    if (length < sizeof(sr_ip_hdr_t)){
        return;
    }

    /* it's for me */
    /* check if the destination of the ip packet is in the list */
    if (sr_ip_des_inlist(sr, ip_hdr->ip_dst)){
        if (ip_hdr->ip_p == ip_protocol_icmp){
            // handle icmp packet

            // need to do icmp check sum first !!!

            sr_icmp_hdr *icmp_hdr = (struct sr_icmp_hdr*)((uint8_t*)(packet + sizeof(struct sr_ethernet_hdr)) + sizeof(struct sr_ip_hdr));
            if (icmp_hdr->icmp_type == 8){
                // handle type 8 icmp echo req
                sr_next_hop_ip_and_iface(sr->routing_table, ip_dst, &next_hop_ip, iface_out);
                send_icmp_echo_reply(sr, packet, len)
            }
            else{
                return;
            }

        }
        /* or it is TCP/UDP */
        else if(ip_hdr->ip_p == ip_protocol_tcp || ip_hdr->ip_p == ip_protocol_udp){
            /* send type 3 icmp port unreachable */
            sr_next_hop_ip_and_iface(sr->routing_table, ip_dst, &next_hop_ip, iface_out);
            send_icmp_port_unreachable(sr, packet, len)
        }
    }

    /* not for me */
    else{
        /* check whether ttl time exceed */
        if (ip_hdr->ip_ttl - 1 <= 0) {
            /* send type 11 icmp time exceed */
            sr_next_hop_ip_and_iface(sr->routing_table, ip_dst, &next_hop_ip, iface_out);
            send_icmp_time_exceeded(sr, packet, len)
        }
        else{
            /* check routing table preform LPM */
            ip_hdr->ip_ttl -= 1;
            ip_hdr->ip_sum = 0;
            ip_hdr->ip_sum = cksum(ip_hdr, sizeof(sr_ip_hdr_t));
            ip_dst = ip_hdr->ip_dst;
            if(!sr_next_hop_ip_and_iface(sr->routing_table, ip_dst, &next_hop_ip, iface_out)/* if routing table not match */){
                /* routing table not match */
                /* send type 3 icmp net unreachable */
                sr_next_hop_ip_and_iface(sr->routing_table, ip_dst, &next_hop_ip, iface_out);
                send_icmp_net_unreachable(sr, packet, len)
            }
            else{
                e_hdr = (sr_ethernet_hdr_t *) packet_to_send;
                entry = sr_arpcache_lookup(&(sr->cache), next_hop_ip);
                if(entry/* check arp cache hit */){
                    /* if hit the entry, send the frame to next hope */
                    memcpy(e_hdr->ether_dhost, entry->mac, ETHER_ADDR_LEN);
                    sr_send_packet(sr, packet_to_send, len, iface_out);
                    free(entry);
                }
                else{
                    /* send arp request */
                    req = sr_arpcache_queuereq(&(sr->cache), next_hop_ip, packet_to_send, len, iface_out);
                    sr_handle_arpreq(sr, req);
                }
            }
        }
    }
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




void sr_handle_arpreq(struct sr_instance* sr, struct sr_arpreq* req) {
	pthread_mutex_lock(&(sr->cache.lock));
	time_t curtime = time(NULL);
	struct sr_if* sr_if = sr_get_interface(sr, req->packets->iface);

	if (difftime(curtime, req->sent) >= 1) {
		if (req->times_sent < 5) {	
			uint8_t* packet = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t););

			struct sr_ethernet_hdr* ether_hdr  = (struct sr_ethernet_hdr*)packet;
			memcpy(ether_hdr->ether_shost, (char*)(sr_if->addr), ETHER_ADDR_LEN);
			memcpy(ether_hdr->ether_dhost, "\xff\xff\xff\xff\xff\xff", ETHER_ADDR_LEN);
			ether_hdr->ether_type = htons(ethertype_arp);

			struct sr_arp_hdr* arp_hdr (sr_arp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
			arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
			arp_hdr->ar_pro = htons(0x0800);
			arp_hdr->ar_hln = ETHER_ADDR_LEN;
			arp_hdr->ar_pln = 4;
			arp_hdr->ar_tip = req->ip;
			arp_hdr->ar_sip = sr_if->ip;
			arp_hdr->ar_op = htons(arp_op_request);
			memcpy(arp_hdr->ar_tha, "\xff\xff\xff\xff\xff\xff", ETHER_ADDR_LEN);
			memcpy(arp_hdr->ar_sha, (char*)(sr_if->addr), ETHER_ADDR_LEN);
			sr_send_packet(sr, packet, sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t), sr_if->name);

			free(packet);
			req->sent = time(NULL);
			req->times_sent++;
		}
		else {
			struct sr_packet* packet = req->packets;
			uint32_t next_ip = 0;
			char iface[sr_IFACE_NAMELEN];
			while (packet){
				struct sr_ip_hdr* ip_hdr = (sr_ip_hdr_t*)(pkt->buf + sizeof(sr_ethernet_hdr_t));
				uint32_t ip_dst = ip_hdr->ip_src;
				unsigned int len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t);
				uint8_t*  new_packet = malloc(len);
				sr_find_next_hop_ip_and_iface(sr->routing_table, ip_dst, &next_ip, iface);
				sr_new_icmp_message( sr, 3,  1,  new_packet, packet->buf, iface)
				sr_arpcache_queuereq(&(sr->cache), next_hop_ip, packet_to_send, len, iface);
				free(packet_to_send);
				packet = packet->next;
			}
			arpreq_destroy(&(sr->cache), req);
		}
	}
	pthread_mutex_unlock(&(sr->cache.lock));
}


