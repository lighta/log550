/*
 * protocol.h
 *
 * Created: 2015-03-08 11:33:32
 *  Author: aj98150
 */ 


#ifndef PROTOCOL_H_
#define PROTOCOL_H_

/**
 * \brief 
 * 
 * \param dest : char[]
 * \param source : char[]
 * \param dest_offset
 * \param t : size(t)
 * 
 */
#define memcpy2(dest, source, offset, t){\
	char data[] = source;\
	memcpy((&dest[0])+offset,&data[0],t);\
}

/************************************************************************/
/*                            GENERALS SIZES                            */
/************************************************************************/
#define MAC_SIZE			6
#define IPV4_SIZE			4
#define PROTOCOL_CODE_SIZE	2

/************************************************************************/
/*                            GENERALS CODES                            */
/************************************************************************/

#define ETHERNET_PROTOCOL_CODE	{0x00,0x01}
#define ARP_PROTOCOL_CODE		{0x08,0x06}
#define IP_PROTOCOL_CODE		{0x08,0x00}

#define ARP_OPCODE_REQUEST	{0x00,0x01}
#define ARP_OPCODE_REPLY	{0x00,0x02}



/************************************************************************/
/*                            ETHERNET                                  */
/************************************************************************/
#define ETHERNET_HEADER_OFFSET			0
#define ETHERNET_HEADER_SIZE			MAC_SIZE + MAC_SIZE + PROTOCOL_CODE_SIZE


#define ETHERNET_TARGET_MAC_OFFSET		0
#define ETHERNET_SOURCE_MAC_OFFSET		ETHERNET_TARGET_MAC_OFFSET + MAC_SIZE
#define ETHERNET_PROTOCOL_CODE_OFFSET	ETHERNET_SOURCE_MAC_OFFSET + MAC_SIZE


/************************************************************************/
/*								 ARP (IPV4)	                            */
/************************************************************************/

#define ARP_HARDWARE_TYPE_SIZE		PROTOCOL_CODE_SIZE
#define ARP_PROTOCOL_TYPE_SIZE		PROTOCOL_CODE_SIZE
#define ARP_HARDWARE_SIZE_SIZE		1
#define ARP_PROTOCOL_SIZE_SIZE		1
#define ARP_OPCODE_SIZE				PROTOCOL_CODE_SIZE
#define ARP_IPSIZE					IPV4_SIZE //ipv4, ipv6 ?

#define ARP_HARDWARE_TYPE_OFFSET	0
#define ARP_PROTOCOL_TYPE_OFFSET	ARP_HARDWARE_TYPE_OFFSET+ARP_HARDWARE_TYPE_SIZE
#define ARP_HARDWARE_SIZE_OFFSET	ARP_PROTOCOL_TYPE_OFFSET+ARP_PROTOCOL_TYPE_SIZE
#define ARP_PROTOCOL_SIZE_OFFSET	ARP_HARDWARE_SIZE_OFFSET+ARP_HARDWARE_SIZE_SIZE
#define ARP_OPCODE_OFFSET			ARP_PROTOCOL_SIZE_OFFSET+ARP_PROTOCOL_SIZE_SIZE
#define ARP_SENDER_MAC_OFFSET		ARP_OPCODE_OFFSET+ARP_OPCODE_SIZE
#define ARP_SENDER_IP_OFFSET		ARP_SENDER_MAC_OFFSET+MAC_SIZE
#define ARP_TARGET_MAC_OFFSET		ARP_SENDER_IP_OFFSET+ARP_IPSIZE
#define ARP_TARGET_IP_OFFSET		ARP_TARGET_MAC_OFFSET+MAC_SIZE // last element


#define ARP_HEADER_OFFSET			ETHERNET_HEADER_OFFSET + ETHERNET_HEADER_SIZE
#define ARP_HEADER_SIZE				ARP_TARGET_IP_OFFSET + ARP_IPSIZE

/************************************************************************/
/*								 IPV4		                            */
/************************************************************************/
#define IPV4_VERSION_HL_SIZE		1
#define IPV4_SERVICES_SIZE			1
#define PV4_TOTAL_LENGHT_SIZE		2
#define IPV4_ID_SIZE				2
#define IPV4_FLAGS_SIZE				1
#define IPV4_FRAG_SIZE				2
#define IPV4_TTL_SIZE				1
#define IPV4_PROTOCOL_SIZE			1
#define IPV4_CHECKSUM_SIZE			2

#define IPV4_VERSION_HL_OFFSET		0
#define IPV4_SERVICES_OFFSET		IPV4_VERSION_HL_OFFSET+IPV4_VERSION_HL_SIZE
#define IPV4_TOTAL_LENGHT_OFFSET	IPV4_SERVICES_OFFSET+IPV4_SERVICES_SIZE
#define IPV4_ID_OFFSET				IPV4_TOTAL_LENGHT_OFFSET+PV4_TOTAL_LENGHT_SIZE
#define IPV4_FLAGS_OFFSET			IPV4_ID_OFFSET+IPV4_ID_SIZE
#define IPV4_FRAG_OFFSET			IPV4_FLAGS_OFFSET+IPV4_FLAGS_SIZE
#define IPV4_TTL_OFFSET				IPV4_FRAG_OFFSET+IPV4_FRAG_SIZE
#define IPV4_PROTOCOL_OFFSET		IPV4_TTL_OFFSET+IPV4_TTL_SIZE
#define IPV4_CHECKSUM_OFFSET		IPV4_PROTOCOL_OFFSET+IPV4_PROTOCOL_SIZE
#define IPV4_SOURCE_IP_OFFSET		IPV4_CHECKSUM_OFFSET+IPV4_CHECKSUM_SIZE
#define IPV4_TARGET_IP_OFFSET		IPV4_SOURCE_IP_OFFSET+IPV4_SIZE


#define IPV4_HEADER_OFFSET			ETHERNET_HEADER_OFFSET + ETHERNET_HEADER_SIZE
#define IPV4_HEADER_SIZE			IPV4_TARGET_IP_OFFSET + IPV4_SIZE

/************************************************************************/
/*								 UDP		                            */
/************************************************************************/

#define UDP_PORT_SRC_SIZE	2
#define UDP_PORT_DST_SIZE	2
#define UDP_LEN_SIZE	2
#define UDP_CRC_SIZE	2

#define UDP_PORT_SRC_OFFSET			0
#define UDP_PORT_DST_OFFSET			UDP_PORT_SRC_OFFSET+UDP_PORT_SRC_SIZE
#define UDP_LEN_OFFSET				UDP_PORT_DST_OFFSET+UDP_PORT_DST_SIZE
#define UDP_CRC_OFFSET				UDP_LEN_OFFSET+UDP_LEN_SIZE

#define UDP_HEADER_OFFSET			IPV4_HEADER_OFFSET+IPV4_HEADER_SIZE //or IPV6_HEADER
#define UDP_HEADER_SIZE				UDP_CRC_OFFSET+UDP_CRC_SIZE


#endif /* PROTOCOL_H_ */
