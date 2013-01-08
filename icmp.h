/*
   Copyright [2010] [Richard Bross]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#ifndef __ICMP_H
#define __ICMP_H

// Definition of type field values.
#define	ICMP_ECHOREPLY		0		// echo reply
#define	ICMP_UNREACH		3		// dest unreachable, codes:
#define	ICMP_SOURCEQUENCH	4		// packet lost, slow down
#define	ICMP_REDIRECT		5		// shorter route, codes:
#define	ICMP_ECHO			8		// echo service
#define	ICMP_ROUTERADVERT	9		// router advertisement
#define	ICMP_ROUTERSOLICIT	10		// router solicitation
#define	ICMP_TIMXCEED		11		// time exceeded, code:
#define	ICMP_PARAMPROB		12		// ip header bad
#define	ICMP_TSTAMP			13		// timestamp request
#define	ICMP_TSTAMPREPLY	14		// timestamp reply
#define	ICMP_IREQ			15		// information request
#define	ICMP_IREQREPLY		16		// information reply
#define	ICMP_MASKREQ		17		// address mask request
#define	ICMP_MASKREPLY		18		// address mask reply
#define	ICMP_MAXTYPE		18

// Code field values
	// ICMP_UNREACH codes
#define		ICMP_UNREACH_NET			0		// bad net
#define		ICMP_UNREACH_HOST			1		// bad host
#define		ICMP_UNREACH_PROTOCOL		2		// bad protocol
#define		ICMP_UNREACH_PORT			3		// bad port
#define		ICMP_UNREACH_NEEDFRAG		4		// IP_DF caused drop
#define		ICMP_UNREACH_SRCFAIL		5		// src route failed
#define		ICMP_UNREACH_NET_UNKNOWN	6		// unknown net
#define		ICMP_UNREACH_HOST_UNKNOWN	7		// unknown host
#define		ICMP_UNREACH_ISOLATED		8		// src host isolated
#define		ICMP_UNREACH_NET_PROHIB		9		// prohibited access
#define		ICMP_UNREACH_HOST_PROHIB	10		// ditto
#define		ICMP_UNREACH_TOSNET			11		// bad tos for net
#define		ICMP_UNREACH_TOSHOST		12		// bad tos for host
	// ICMP_REDIRECT codes
#define		ICMP_REDIRECT_NET			0		// for network
#define		ICMP_REDIRECT_HOST			1		// for host
#define		ICMP_REDIRECT_TOSNET		2		// for tos and net
#define		ICMP_REDIRECT_TOSHOST		3		// for tos and host
	//ICMP_PARAMPROB codes
#define		ICMP_TIMXCEED_INTRANS		0		// ttl==0 in transit
#define		ICMP_TIMXCEED_REASS			1		// ttl==0 in reass
#define		ICMP_PARAMPROB_OPTABSENT	1		// req. opt. absent

#define ICMP_RESPONSE_TOOFEW	-1
#define ICMP_RESPONSE_NONECHO	-2
#define ICMP_RESPONSE_INVID		-3

#define PING_RESPONSE_SUCCESS			1000
#define PING_RESPONSE_TIMEOUT			1001
#define PING_RESPONSE_ERROR				1002
#define PING_RESPONSE_TRACING			1003
#define PING_RESPONSE_CANCELLED			1004
#define PING_RESPONSE_UNREACHABLE		1005
#define PING_RESPONSE_SOURCEROUTEFAIL	1006

#define ICMP_PACKET_MIN		8			// Minimum 8 byte icmp packet (just header)
#define	IP_PACKET_MIN		64			// Minimum 64 byte ip packet
#define ICMP_RETURNED_DATA	8			// ICMP returns first 8 bytes of data from original packet

#define	PING_COUNT	3
#define DATA_PORTION_SIZE   32

//
//  The IP header
//
typedef struct iphdr {
#if defined(UNIX)
/* Unix compilers pack bit fields from high order to low order bits, so this
   ordering puts the version in the high four bits of the byte. */
	unsigned char   version:4;      // Version of IP
	unsigned char   h_len:4;        // Length of the header
#else
/* Microsoft compiler packs bit fields from low order to high order bits, so
   this ordering puts the version in the high four bits. */
	unsigned char   h_len:4;        // Length of the header
	unsigned char   version:4;      // Version of IP
#endif
	unsigned char   tos;            // Type of service
	unsigned short  total_len;      // Total length of the packet
	unsigned short  ident;          // Unique identifier
	unsigned short  frag_and_flags; // Flags
	unsigned char   ttl;
	unsigned char   proto;          // Protocol (TCP, UDP etc)
	unsigned short  checksum;       // IP checksum
	unsigned long	sourceIP;
	unsigned long	destIP;
} IpHeader;

//
// ICMP header
//
typedef struct _ihdr
	{
    unsigned char   i_type;
    unsigned char   i_code;				// Type sub code
    unsigned short	i_cksum;
	union {
		unsigned char	ih_pptr;		// ICMP_PARAMPROB
		struct in_addr	ih_gwaddr;		// ICMP_REDIRECT
		struct ih_idseq {
					unsigned short	icd_id;
					unsigned short	icd_seq;
						} ih_idseq;

		int				ih_void;

			// ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191)
		struct ih_pmtu {
					short			ipm_void;
					short			ipm_nextmtu;
						} ih_pmtu;
			} icmp_hun;						

	union	{
		struct id_ts {
					unsigned long	its_otime;
					unsigned long	its_rtime;
					unsigned long	its_ttime;
					} id_ts;
		struct id_ip	{
					struct			iphdr idi_ip;
						// options and then 64 bits of data
						} id_ip;
		unsigned long	id_mask;
		char			id_data[1];
			} icmp_dun;
	}	IcmpHeader;


typedef struct _i_LSRR	{		// Loose routing format
	unsigned char	nopt;
	unsigned char	code;
	unsigned char	len;
	unsigned char	ptr;
	unsigned long	ip_addr[9];
} IPLooseRoute;

#endif

