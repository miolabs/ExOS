#include <arpa/inet.h>
#include <stdint.h>

uint32_t inline htonl(uint32_t value)
{
	uint8_t octets[] = { value >> 24, value >> 16, value >> 8, value };
	return *(uint32_t *)octets;
}

uint16_t inline htons(uint16_t value)
{
	uint8_t octets[] = { value >> 8, value };
	return *(uint16_t *)octets;
}

uint32_t inline ntohl(uint32_t value)
{
	uint8_t *octets = (uint8_t *)&value;
	return (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3]; 
}

uint16_t inline ntohs(uint16_t value)
{
	uint8_t *octets = (uint8_t *)&value;
	return (octets[0] << 8) | octets[1]; 
}


