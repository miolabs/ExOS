// IP Stack. IP Datagram Support
// by Miguel Fides

#include "ip.h"
#include "icmp.h"
//#include "udp_io.h"
//#include "tcp_io.h"
#include "arp.h"
#include "net_service.h"
#include <kernel/memory.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "ip", __VA_ARGS__);

const IP_ENDPOINT __ep_broadcast = {
	.MAC = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 
	.IP = {255, 255, 255, 255} };

static int _packet_id = 0;

void net_ip_initialize()
{
//	__udp_io_initialize();
//	__tcp_io_initialize();
}

void net_ip_init_config(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	net_ip_config_t *ip_config = (net_ip_config_t *)exos_mem_alloc(sizeof(net_ip_config_t), EXOS_MEMF_CLEAR);
	ASSERT(ip_config != NULL, KERNEL_ERROR_NOT_ENOUGH_MEMORY);
	ip_config->Base.Protocol = ETH_TYPE_IP;

	net_service_set_config(adapter, &ip_config->Base);
}

bool net_ip_get_config(net_adapter_t *adapter, net_ip_config_t **pconfig)
{
	return net_service_get_config(adapter, ETH_TYPE_IP, (net_config_t **)pconfig);
}

void *net_ip_get_payload(ip_header_t *ip, unsigned short *plength)
{
	int header_length = ip->HeaderLength << 2;
	if (plength) *plength = NTOH16(ip->TotalLength) - header_length; 
	return (void *)ip + header_length;
}

bool net_ip_input(net_adapter_t *adapter, eth_header_t *eth, ip_header_t *ip)
{
	net_ip_config_t *config;
	if (!net_ip_get_config(adapter, &config))
	{
		_verbose(VERBOSE_ERROR, "ip input on adapter without ip config!");
		return false;
	}
	
	bool queued = false;
	unsigned short checksum = net_ip_checksum((net16_t *)ip, ip->HeaderLength << 2);
	if (checksum == 0)
	{
		if (ip->DestinationIP.Value == config->IP.Value || 
			ip->DestinationIP.Value == (config->IP.Value | ~config->Mask.Value) ||
			ip->DestinationIP.Value == 0xffffffff)
		{
			ip_protocol_t protocol = ip->Protocol;
			switch(protocol)
			{
				case IP_PROTOCOL_ICMP:
					net_arp_set_hw_addr(&ip->SourceIP, &eth->Sender);
					queued = net_icmp_input(adapter, eth, ip);
					break;
				//case IP_PROTOCOL_TCP:
				//	net_arp_set_hw_addr(&ip->SourceIP, &eth->Sender);
				//	return net_tcp_input(adapter, eth, ip);
				//case IP_PROTOCOL_UDP:
				//	net_arp_set_hw_addr(&ip->SourceIP, &eth->Sender);
				//	return net_udp_input(adapter, eth, ip);
			}
		}
	}
	else _verbose(VERBOSE_DEBUG, "incoming packet with bad checksum!");
	return queued;
}

void *net_ip_output(net_adapter_t *adapter, net_buffer_t *output, unsigned length, const IP_ENDPOINT *destination, ip_protocol_t protocol)
{
	net_ip_config_t *config;
	if (!net_ip_get_config(adapter, &config))
	{
		_verbose(VERBOSE_ERROR, "ip output on adapter without ip config!");
		return NULL;
	}

	length += sizeof(ip_header_t);
	ip_header_t *ip = (ip_header_t *)net_output(adapter, output, length, &destination->MAC, ETH_TYPE_IP);
	ASSERT(ip != NULL, KERNEL_ERROR_KERNEL_PANIC);
	ip->HeaderLength = 5;
	ip->Version = IP_VER_IPv4;
	ip->DiffServ = destination->DiffServ.Value;
	ip->Id = HTON16(_packet_id++);
	ip->Fragment = HTON16(0);
	ip->TTL = 64;
	ip->Protocol = protocol;
	ip->HeaderChecksum = HTON16(0);
	ip->SourceIP = config->IP;
	ip->DestinationIP = destination->IP;
	return (void *)ip + sizeof(ip_header_t);
}

bool net_ip_send_output(net_adapter_t *adapter, net_buffer_t *output, unsigned payload)
{
	eth_header_t *eth = (eth_header_t *)(output->Root.Buffer + output->Root.Offset);

	// fill length field in ip header
	ip_header_t *ip = (ip_header_t *)((void *)eth + sizeof(eth_header_t));
	unsigned short ip_length = payload + sizeof(ip_header_t);
	ip->TotalLength = HTON16(ip_length);

	// fill checksum field in ip_header
	unsigned short header_checksum = net_ip_checksum((net16_t *)ip, ip->HeaderLength << 2);
	ip->HeaderChecksum = HTON16(header_checksum);

	return net_adapter_send_output(adapter, output);
}

unsigned short net_ip_checksum(net16_t *data, unsigned byte_count)
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

bool net_ip_get_adapter_and_resolve(net_adapter_t **padapter, IP_ENDPOINT *ep)
{
/*
	IP_ENDPOINT gw_ep = (IP_ENDPOINT) { .IP = ep->IP };
	NET_ADAPTER *adapter = net_adapter_find(gw_ep.IP);
	if (adapter == NULL)
	{
		adapter = net_adapter_find_gateway(gw_ep.IP);
		if (adapter != NULL) gw_ep.IP = adapter->Gateway;
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
			
			if (net_ip_resolve(adapter, &gw_ep))
			{
				*padapter = adapter;
				ep->MAC = gw_ep.MAC;
				return 1;
			}
		}
	}
	return 0;
*/
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

bool net_ip_resolve(net_adapter_t *adapter, IP_ENDPOINT *ep)
{
/*
	if (adapter == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (adapter->Speed == 0)
		return 0; // adapter is down
	
	if (0 == ~(ep->IP.Value | adapter->NetMask.Value)) // broadcast ip
	{
		ep->MAC = IP_ENDPOINT_BROADCAST->MAC;
		return 1;
	}

	return net_arp_obtain_hw_addr(adapter, &ep->IP, &ep->MAC);
*/
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

bool net_ip_set_addr(net_adapter_t *driver, ip_addr_t ip, ip_addr_t mask, ip_addr_t gateway)
{
//	driver->IP = ip;
//	driver->NetMask = mask;
//	driver->Gateway = gateway;
//	net_arp_initialize();
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}


