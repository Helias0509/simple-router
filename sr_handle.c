#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sr_protocol.h"
#include "sr_if.h"
#include "sr_router.h"
#include "sr_rt.h"
#include "sr_utils.h"
#include "sr_handle.h"

char * sr_gateway_interface(struct sr_instance *sr, uint32_t ip) {
	struct sr_rt *rt_node = sr->routing_table;
	while (rt_node) {
		if ((ip & rt_node->mask.s_addr) == rt_node->dest.s_addr) {
			return rt_node->interface;
		}
		rt_node = rt_node->next;
	}

	return NULL;
}

int sr_send_icmp(struct sr_instance* sr, uint8_t *packet, char* interface, uint8_t type, uint8_t code) {
	size_t icmp_hdr_size = 0;
	size_t up_to_icmp_size = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t);
	sr_ip_hdr_t *ihdr_old = (sr_ip_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));

	switch(type) {
		case 0:
			icmp_hdr_size = ntohs(ihdr_old->ip_len) - ihdr_old->ip_hl*4;
			break;
		case 11:
			icmp_hdr_size = sizeof(sr_icmp_t11_hdr_t);
			break;
		case 3:
			icmp_hdr_size = sizeof(sr_icmp_t3_hdr_t);
			break;
		default:
			fprintf(stderr, "ICMP type not supported");
			return -1;
	}

	unsigned int len_new = up_to_icmp_size + icmp_hdr_size;
	uint8_t *packet_new = (uint8_t *) malloc(len_new);
	bzero(packet_new, len_new);
	struct sr_if *if_st = sr_get_interface(sr, interface);

	/* ethernet header */
	sr_ethernet_hdr_t *ehdr = (sr_ethernet_hdr_t *) packet_new;
	sr_ethernet_hdr_t *ehdr_old = (sr_ethernet_hdr_t *) packet;
	memcpy(ehdr->ether_dhost, ehdr_old->ether_shost, ETHER_ADDR_LEN);
	memcpy(ehdr->ether_shost, ehdr_old->ether_dhost, ETHER_ADDR_LEN);
	ehdr->ether_type = htons(ethertype_ip);

	/* ip header */
	sr_ip_hdr_t *ihdr = (sr_ip_hdr_t *) (packet_new + sizeof(sr_ethernet_hdr_t));
	ihdr->ip_dst = ihdr_old->ip_src;
	ihdr->ip_hl = 5;
	ihdr->ip_id = 0;
	ihdr->ip_p = ip_protocol_icmp;
	ihdr->ip_src = if_st->ip;
	ihdr->ip_tos = 0;
	ihdr->ip_off = htons(IP_DF);
	ihdr->ip_ttl = INIT_TTL;
	ihdr->ip_v = 4;
	/* icmp */
	sr_icmp_t0_hdr_t *icmp_hdr_old = (sr_icmp_t0_hdr_t *) (packet + up_to_icmp_size);
	sr_icmp_t0_hdr_t *icmp_t0_hdr = (sr_icmp_t0_hdr_t *) (packet_new + up_to_icmp_size);
	sr_icmp_t11_hdr_t *icmp_t11_hdr = (sr_icmp_t11_hdr_t *) (packet_new + up_to_icmp_size);
	sr_icmp_t3_hdr_t *icmp_t3_hdr = (sr_icmp_t3_hdr_t *) (packet_new + up_to_icmp_size);

	switch(type) {
		case 0:
			icmp_t0_hdr->icmp_code = code;
			icmp_t0_hdr->icmp_type = type;
			icmp_t0_hdr->identifier = icmp_hdr_old->identifier;
			icmp_t0_hdr->sequence_number = icmp_hdr_old->sequence_number;
			icmp_t0_hdr->timestamp = icmp_hdr_old->timestamp;
			memcpy(icmp_t0_hdr->data, icmp_hdr_old->data, icmp_hdr_size - ICMP_ZERO_HEADER_SIZE);
			icmp_t0_hdr->icmp_sum = cksum(packet_new + up_to_icmp_size, icmp_hdr_size);
			break;

		case 11:
			icmp_t11_hdr->icmp_code = code;
			icmp_t11_hdr->icmp_type = type;
			memcpy(icmp_t11_hdr->data, packet + sizeof(sr_ethernet_hdr_t), ihdr->ip_hl*4 + 8);
			icmp_t11_hdr->icmp_sum = cksum(packet_new + up_to_icmp_size, icmp_hdr_size);
			break;

		case 3:
			icmp_t3_hdr->icmp_code = code;
			icmp_t3_hdr->icmp_type = type;
			memcpy(icmp_t3_hdr->data, packet + sizeof(sr_ethernet_hdr_t), ihdr->ip_hl*4 + 8);
			icmp_t3_hdr->icmp_sum = cksum(packet_new + up_to_icmp_size, icmp_hdr_size);
			break;
	}

	/* We must calculate the ICMP length first before
	 * we can caculate the checksum for IP header
	 */
	ihdr->ip_len = htons(20 + icmp_hdr_size);
	ihdr->ip_sum = cksum(packet_new + sizeof(sr_ethernet_hdr_t), ihdr->ip_hl * 4);

	Debug("*** Sending an ICMP packet ***\n");
	/*print_hdr_icmp(packet_new + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));*/

	/* send now */
	int result = sr_send_packet(sr, packet_new, len_new, interface);
	free(packet_new);

	return result;
}

int sr_for_us_arp(struct sr_instance *sr, sr_arp_hdr_t *arp_hdr) {
	struct sr_if *ifs = sr->if_list;

	/* return immediately if it's the broadcast address */
	if(memcmp(arp_hdr->ar_tha, "000000", ETHER_ADDR_LEN) == 0) {
		return 1;
	}

	while (ifs) {
		if (memcmp(ifs->addr, arp_hdr->ar_tha, ETHER_ADDR_LEN) == 0) {
			return 1;
		}
		ifs = ifs->next;
	}
	return 0;
}

int sr_send_arp(struct sr_instance* sr, unsigned short ar_op, unsigned char ar_tha[ETHER_ADDR_LEN], uint32_t ar_tip) {
	unsigned int len_new = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
	uint8_t *packet_new = (uint8_t *)malloc(len_new);
	bzero(packet_new, len_new);
	char *interface = sr_gateway_interface(sr, ar_tip);
	struct sr_if *if_st = sr_get_interface(sr, interface);

	/* ethernet frame */
	sr_ethernet_hdr_t *ehdr = (sr_ethernet_hdr_t *) packet_new;
	if (ar_op == arp_op_request)
		memset(ehdr->ether_dhost, 0xff, ETHER_ADDR_LEN);
	else
		memcpy(ehdr->ether_dhost, ar_tha, ETHER_ADDR_LEN);

	memcpy(ehdr->ether_shost, if_st->addr, ETHER_ADDR_LEN);
	ehdr->ether_type = htons(ethertype_arp);

	/* arp header */
	sr_arp_hdr_t *arp_hdr = (sr_arp_hdr_t *) (packet_new + sizeof(sr_ethernet_hdr_t));
	arp_hdr->ar_hln = ETHER_ADDR_LEN;
	arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
	arp_hdr->ar_op = htons(ar_op);
	arp_hdr->ar_pln = 4;
	arp_hdr->ar_pro = htons(ethertype_ip);
	memcpy(arp_hdr->ar_sha, if_st->addr, ETHER_ADDR_LEN);
	arp_hdr->ar_sip = if_st->ip;
	memcpy(arp_hdr->ar_tha, ar_tha, ETHER_ADDR_LEN);
	arp_hdr->ar_tip = ar_tip;

	Debug("*** Sending an ARP packet ***\n");
	/*print_hdrs(packet_new, len_new); */

	int result = sr_send_packet(sr, packet_new, len_new, interface);
	free(packet_new);
	return result;
}

int sr_handle_arp(struct sr_instance* sr, uint8_t *packet, unsigned int len, char* interface) {
	if (len < sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t)) {
		fprintf(stderr, "Invalid ARP header size");
		return -1;
	}

	sr_arp_hdr_t *arp_hdr = (sr_arp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t));

	if (arp_hdr->ar_hrd != htons(arp_hrd_ethernet)) {
		fprintf(stderr, "ARP hardware format not supported");
		return -1;
	}

	if (arp_hdr->ar_pro != htons(ethertype_ip)) {
		fprintf(stderr, "ARP header not valid: IPv4 only");
		return -1;
	}

	struct sr_if *target_if = sr_get_interface_from_ip(sr, arp_hdr->ar_tip);
	/* Reply or request? */
	if (arp_hdr->ar_op == htons(arp_op_request)) { /* request */
		if (!target_if) {
			Debug("ARP request NOT for our router\n");
			return -1;
		} else {
			Debug("ARP request for our router. Sending a reply...\n");
			return sr_send_arp(sr, arp_op_reply, arp_hdr->ar_sha, arp_hdr->ar_sip);
		}
	} else { /* reply */
		/* Only cache if the target IP is one of our router's interfaces' IP address */
		Debug("Receive ARP reply at interface %s\n", interface);
		struct sr_arpreq *req = NULL;
		if(target_if) { /* Target is our router */
			req = sr_arpcache_insert(&(sr->cache), arp_hdr->ar_sha, arp_hdr->ar_sip);
		} else {
			req = sr->cache.requests;
			while (req) {
				if (req->ip != arp_hdr->ar_sip)
					req = req->next;
			}
			if (!req) {
				fprintf(stderr, "We don't have anything to do with this ARP reply.");
				return -1;
			}
		}

		struct sr_packet *pk_st = req->packets;
		while (pk_st) {
			sr_ethernet_hdr_t *ehdr_pk = (sr_ethernet_hdr_t *) pk_st->buf;
			struct sr_if *sending_if = sr_get_interface(sr, interface);
			memcpy(ehdr_pk->ether_dhost, arp_hdr->ar_sha, ETHER_ADDR_LEN);
			memcpy(ehdr_pk->ether_shost, sending_if->addr, ETHER_ADDR_LEN);
			sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *) (pk_st->buf + sizeof(sr_ethernet_hdr_t));
			ip_hdr->ip_sum = 0;
			ip_hdr->ip_ttl -= 1;
			ip_hdr->ip_sum = cksum(ip_hdr, ip_hdr->ip_hl*4);
			sr_send_packet(sr, pk_st->buf, pk_st->len, interface);
			pk_st = pk_st->next;
		}

		sr_arpreq_destroy(&(sr->cache), req);
	}

	return 0;
}

struct sr_if* sr_get_interface_from_ip(struct sr_instance *sr, uint32_t ip_dest) {
	struct sr_if *ifs = sr->if_list;
	while (ifs) {
		if (ip_dest == ifs->ip) {
			return ifs;
		}
		ifs = ifs->next;
	}

	return NULL;
}

int sr_handle_ip(struct sr_instance* sr, uint8_t * packet, unsigned int len, char* interface) {
	/* verify length */
	int minlen = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t);
	if (len < minlen) {
		fprintf(stderr, "Invalid IP header size");
		return -1;
	}
	sr_ethernet_hdr_t *ehdr = (sr_ethernet_hdr_t *) packet;
	sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));

	/* verify checksum */
	if(!cksum(ip_hdr, ip_hdr->ip_hl)) {
		fprintf(stderr, "Invalid IP header checksum");
		return -1;
	}

	struct sr_if *target_if = sr_get_interface_from_ip(sr, ip_hdr->ip_dst);

	if (!target_if) { /* not for us, forward it */
		if(ip_hdr->ip_ttl <= 1) {
			/* create a new ICMP type 11 message - time exceeded */
			Debug("TTL Expired\n");
			return sr_send_icmp(sr, packet, interface, 11, 0);
		}

		Debug("The Ip packet is not for us. Forwarding...\n");
		struct sr_rt *rt_node = sr->routing_table;
		while(rt_node) {
			if ((ip_hdr->ip_dst & rt_node->mask.s_addr) == rt_node->dest.s_addr) {
				struct sr_if *out_if = sr_get_interface(sr, rt_node->interface);
				memcpy(ehdr->ether_shost, out_if->addr, ETHER_ADDR_LEN);

				/* Searching the destination MAC address through ARP cache */
				struct sr_arpentry *arp_e = sr_arpcache_lookup(&(sr->cache), rt_node->gw.s_addr);
				if (arp_e) {
					memcpy(ehdr->ether_dhost, arp_e->mac, ETHER_ADDR_LEN);
					free(arp_e);
					ip_hdr->ip_ttl -= 1;
					ip_hdr->ip_sum = 0;
					ip_hdr->ip_sum = cksum(ip_hdr, ip_hdr->ip_hl*4);
					return sr_send_packet(sr, packet, len, rt_node->interface);
				} else {
					handle_arpreq(sr, sr_arpcache_queuereq(&(sr->cache), rt_node->gw.s_addr, packet, len, interface));
					return 0;
				}
			}
			rt_node = rt_node->next;
		}

		/* Destination host unreachable */
		Debug("Host unreachable\n");
		return sr_send_icmp(sr, packet, interface, 3, 1);

	} else { /* handle it */
		Debug("The IP packet is for us\n");
		if (ip_hdr->ip_p != ip_protocol_icmp) {
			Debug("Not ICMP protocol");
			return sr_send_icmp(sr, packet, interface, 3, 3); /* port unreachable */
		} else {
			/* Ignore if it's not an echo request */
			sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
			if (icmp_hdr->icmp_type == 8 && icmp_hdr->icmp_code == 0) {
				Debug("Receive an ICMP Echo request\n");
				return sr_send_icmp(sr, packet, interface, 0, 0);
			}
			else
				return 0;
		}
	}

	return 0;
}
