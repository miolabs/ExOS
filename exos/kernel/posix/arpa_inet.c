#include <arpa/inet.h>
#include <stdint.h>
#include <kernel/machine/hal.h>	// FIXME

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

in_addr_t inet_addr(const char *s)
{
	int done;
	unsigned int value;
	unsigned char a, b, c, d;
	done = __decl_uint((char *)s, &value);
	if (done && value < 256 && s[done] == '.')
	{
		a = value; s += done + 1;
		done = __decl_uint((char *)s, &value);
		if (done && value < 256 && s[done] == '.')
		{
			b = value; s += done + 1;
			done = __decl_uint((char *)s, &value);
			if (done && value < 256 && s[done] == '.')
			{
				c = value; s += done + 1;
				done = __decl_uint((char *)s, &value);
				if (done && value < 256 && s[done] == '\0')
				{
					d = value;
					return (in_addr_t) ((a) | (b << 8) | (c << 16) | (d << 24));
				}
			}
		}
	}
	return (in_addr_t)0;
}

