// IP Stack. IP Datagram Support
// by Miguel Fides

#include "ip.h"
#include "icmp.h"
#include "udp_io.h"
#include "tcp_io.h"
#include "arp.h"
#include <kernel/panic.h>

const IP_ENDPOINT __ep_broadcast = {
	.MAC = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 
	.IP = {255, 255, 255, 255} };
const IP_ADDR __ip_any = { .Value = 0 };

static int _packet_id = 0;

void net_ip_initialize()
{
	__udp_io_initialize();
	__tcp_io_initialize();
}

void *net_ip_get_payload(IP_HEADER *ip, unsigned short *plength)
{
	int header_length = ip->HeaderLength << 2;
	if (plength) *plength = NTOH16(ip->TotalLength) - header_length; 
	return (void *)ip + header_length;
}

int net_ip_input(NET_ADAPTER *adapter, ETH_HEADER *eth, IP_HEADER *ip)
{
	unsigned short checksum = net_ip_checksum((NET16_T *)ip, ip->HeaderLength << 2);
	if (checksum == 0)
	{
		if (ip->DestinationIP.Value == adapter->IP.Value || 
			ip->DestinationIP.Value == (adapter->IP.Value | ~adapter->NetMask.Value) ||
			ip->DestinationIP.Value == 0xffffffff)
		{
			IP_PROTOCOL protocol = ip->Protocol;
			switch(protocol)
			{
				case IP_PROTOCOL_ICMP:
					net_arp_set_hw_addr(&ip->SourceIP, &eth->Sender);
					net_icmp_input(adapter, eth, ip);
					break;
				case IP_PROTOCOL_TCP:
					net_arp_set_hw_addr(&ip->SourceIP, &eth->Sender);
					return net_tcp_input(adapter, eth, ip);
				case IP_PROTOCOL_UDP:
					net_arp_set_hw_addr(&ip->SourceIP, &eth->Sender);
					return net_udp_input(adapter, eth, ip);
			}
		}
	}
	return 0;
}

void *net_ip_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output, unsigned hdr_size, IP_ENDPOINT *destination, IP_PROTOCOL protocol)
{
	hdr_size += sizeof(IP_HEADER);
	IP_HEADER *ip = (IP_HEADER *)net_adapter_output(adapter, output, hdr_size, &destination->MAC, ETH_TYPE_IP);
	if (ip != NULL)
	{
		ip->HeaderLength = 5;
		ip->Version = IP_VER_IPv4;
		ip->DiffServ = destination->DiffServ.Value;
		ip->Id = HTON16(_packet_id++);
		ip->Fragment = HTON16(0);
		ip->TTL = 64;
		ip->Protocol = protocol;
		ip->HeaderChecksum = HTON16(0);
		ip->SourceIP = adapter->IP;
		ip->DestinationIP = destination->IP;
		return (void *)ip + sizeof(IP_HEADER);
	}
	return NULL;
}

int net_ip_send_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output, unsigned payload)
{
	ETH_HEADER *eth = (ETH_HEADER *)output->Buffer.Buffer;

	// fill length field in ip header
	IP_HEADER *ip = (IP_HEADER *)((void *)eth + sizeof(ETH_HEADER));
	unsigned short ip_length = payload + sizeof(IP_HEADER);
	ip->TotalLength = HTON16(ip_length);

	// fill checksum field in ip_header
	unsigned short header_checksum = net_ip_checksum((NET16_T *)ip, ip->HeaderLength << 2);
	ip->HeaderChecksum = HTON16(header_checksum);

	return net_adapter_send_output(adapter, output);
}

unsigned short net_ip_checksum(NET16_T *data, unsigned byte_count)
{
	unsigned long sum = 0;
	int word_count = byte_count >> 1;
	if (byte_count & 1)
	{
		((unsigned char *)data)[byte_count] = 0;
		word_count++;
	}
	for(int i = 0; i < word_count; i++)
	{
		sum += NTOH16(data[i]);
	}
	sum += (sum >> 16);	// add carry to do 1's complement sum
	return (unsigned short)~sum;
}

int net_ip_get_adapter_and_resolve(NET_ADAPTER **padapter, IP_ENDPOINT *ep)
{
	IP_ADDR addr = ep->IP;
	NET_ADAPTER *adapter = net_adapter_find(addr);
	if (adapter == NULL)
	{
		adapter = net_adapter_find_gateway(addr);
		if (adapter != NULL) addr = adapter->Gateway;
	}
	if (adapter != NULL)
	{
		for(int i = 0; i < 5; i++)
		{
			if (adapter->Speed == 0)	 // link is down
			{
				exos_thread_sleep(1000);
				continue;	// down
			}
			
			if (net_ip_resolve(adapter, ep))
			{
				*padapter = adapter;
				return 1;
			}
		}
	}
	return 0;
}

int net_ip_resolve(NET_ADAPTER *adapter, IP_ENDPOINT *ep)
{
	if (adapter == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	// FIXME: we should try to match broadcast ip first!

	int found = net_arp_obtain_hw_addr(adapter, &ep->IP, &ep->MAC);
	if (!found &&
		0 == ~(ep->IP.Value | adapter->NetMask.Value))	// broadcast ip
	{
		ep->MAC = IP_ENDPOINT_BROADCAST->MAC;
		found = 1;
	}
	return found;
}

int net_ip_set_addr(NET_ADAPTER *driver, IP_ADDR ip, IP_ADDR mask, IP_ADDR gateway)
{
	driver->IP = ip;
	driver->NetMask = mask;
	driver->Gateway = gateway;
	net_arp_initialize();
}


