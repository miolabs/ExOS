#ifndef KIMALDI_RF_H
#define KIMALDI_RF_H

typedef struct
{
	long long Id;
} RF_CARD;

typedef void (*RF_CALLBACK)(RF_CARD *card, void *state);

typedef struct
{
	int Count;
	void *UserState;
	RF_CALLBACK Callback;
	char Buffer[10];
} RF_STATE;

void rf_initialize();
void rf_init_state(RF_STATE *state, RF_CALLBACK callback, void *userstate);
void rf_parse(RF_STATE *state, char *buffer, int length);

#endif // KIMALDI_RF_H

