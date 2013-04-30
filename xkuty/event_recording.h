
enum
{
	CHECK_PRESS = 1,
	CHECK_RELEASE,
	CHECK_PRESSED,
	CHECK_RELEASED,
	//
	CHECK_END
};

typedef struct 
{
	unsigned int input_mask, check_type;
} EVREC_CHECK;	// Event record check

void event_record    ( unsigned int input_state_bits);
int  event_happening ( const EVREC_CHECK* check_list);
