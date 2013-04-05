/*
 * Liem Do <liemdo@ucla.edu>
 * This file will handle all types of packets
 */

#ifndef SR_HANDLE_H
#define SR_HANDLE_H

#include "sr_protocol.h"

/*
 * Send ICMP packet
 * @return 0 on success of -1 on error
 */
int sr_send_icmp(struct sr_instance* sr, uint8_t *packet, char* interface, uint8_t type, uint8_t code);

/*
 * Send an ARP request or reply
 */
int sr_send_arp(struct sr_instance* sr, unsigned short ar_op, unsigned char ar_tha[ETHER_ADDR_LEN], uint32_t ar_tip);

/*
 * Handle ARP packets
 * @return 0 on success or -1 on error
 */
int sr_handle_arp(struct sr_instance* sr, uint8_t *packet, unsigned int len, char* interface);

/*
 * Get an interface from an ip
 * @return NULL if not found
 */
struct sr_if* sr_get_interface_from_ip(struct sr_instance *sr, uint32_t ip_dest);

/*
 * Find in the routing table the target interface to send the packet through
 */
char * sr_gateway_interface(struct sr_instance *sr, uint32_t ip);

/*
 * Handle IP header
 * @return 0 on success or -1 on error
 */
int sr_handle_ip(struct sr_instance* sr, uint8_t * packet, unsigned int len, char* interface);
/* int sr_handle_arp(struct sr_instance* sr, uint8_t * packet, unsigned int len, char* interface); */

/* -- sr_vns_comm.c -- */
int sr_send_packet(struct sr_instance* , uint8_t* , unsigned int , const char*);
int sr_connect_to_server(struct sr_instance* ,unsigned short , char* );
int sr_read_from_server(struct sr_instance* );

/* -- sr_router.c -- */
void sr_init(struct sr_instance* );
void sr_handlepacket(struct sr_instance* , uint8_t * , unsigned int , char* );

/* -- sr_if.c -- */
void sr_add_interface(struct sr_instance* , const char* );
void sr_set_ether_ip(struct sr_instance* , uint32_t );
void sr_set_ether_addr(struct sr_instance* , const unsigned char* );
void sr_print_if_list(struct sr_instance* );


#endif
