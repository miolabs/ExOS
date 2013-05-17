#include "rf.h"

static int _parse_id(char *buffer, RF_CARD *card);

void rf_initialize()
{
}

void rf_init_state(RF_STATE *state, RF_CALLBACK callback, void *userstate)
{
	state->Callback = callback;
	state->UserState = userstate;
	state->Count = -1;
}

void rf_parse(RF_STATE *state, char *buffer, int length)
{
	for (int i = 0; i < length; i++)
	{
		char c = buffer[i];

		if (state->Count < 0)
		{
			if (c == 0x02) state->Count = 0;
		}
		else if (state->Count < 10)
		{
			state->Buffer[state->Count++] = c;
		}
		else
		{
			RF_CARD id;
			_parse_id(state->Buffer, &id);
			char *data = (char *)&id.Id;
			char crc = 0;
			for (int j = 0; j < 5; j++) crc ^= data[j];
			crc = (crc & 0xF) ^ (crc >> 4);
			crc = (crc < 10) ? crc + '0' : crc + 'A' - 10;
			if (c == crc)
			{
				state->Callback(&id, state->UserState);
			}
			state->Count = -1;
		}
	}
}

static int _parse_id(char *buffer, RF_CARD *card)
{
	long long acc = 0;
	for (int i = 0; i < 10; i++)
	{
		char c = buffer[i];
		if (c >= '0' && c <= '9')
			acc = (acc << 4) | (c - '0');
		else if (c >= 'A' && c <= 'F')
			acc = (acc << 4) | (c + 10 - 'A');
		else return 0;
	}
	card->Id = acc;
	return 1;
}
