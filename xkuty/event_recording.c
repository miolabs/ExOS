
#include "event_recording.h"

#define LEN_EVENT_RECORDS  (128)
#define RECORDS_MASK       (LEN_EVENT_RECORDS - 1)

static int _input_pos = -1;
static unsigned int _input_records [LEN_EVENT_RECORDS];
/*={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,
0x00000000,
0x00000000,
0x00010000,
0x00000000,
0x00000000,
0x00011000,
0x00011000,
0x00001000,
0x00000000,
0x00001000,
0x00001000,
0x00000000
};*/

void _event_init ()
{
	for(int i=0; i<LEN_EVENT_RECORDS; i++) 
		_input_records [ i] = 0;
	_input_pos = 0;
}

void event_record ( unsigned int input_state_bits)
{
	if( _input_pos==-1)
		_event_init ();

	_input_records [ _input_pos] = input_state_bits;
	_input_pos++; 
	_input_pos &= RECORDS_MASK;
}

int event_happening ( const EVREC_CHECK* check_list,  int backlog)
{
	if( _input_pos==-1)
		_event_init ();
	if (backlog <= 0) 
		backlog = 1; 
	if (backlog >= LEN_EVENT_RECORDS) 
		backlog = LEN_EVENT_RECORDS-1;

	// Find check list end
	int curr_check=0;
	while (( check_list[curr_check].check_type != CHECK_END) &&  ( curr_check<LEN_EVENT_RECORDS))
		curr_check++;
	if (( check_list[curr_check].check_type != CHECK_END) || (curr_check==0))
		return 0;
	curr_check--;
	int count = backlog;
	int pos = _input_pos - 1;
	while ( count)
	{
		pos &= RECORDS_MASK;
		int prev_pos = (pos - 1) & RECORDS_MASK;
		unsigned int c = _input_records[pos];	    // current
		unsigned int p = _input_records[prev_pos];	// previous
		unsigned int press   = (c ^ p) & c;
		unsigned int release = (c ^ p) & p;
		unsigned int requested = check_list[curr_check].input_mask;
		switch ( check_list[curr_check].check_type)
		{
			case CHECK_PRESS:
				if (( requested & press) == requested)
					curr_check--;
				break;
			case CHECK_RELEASE:
				if (( requested & release) == requested)
					curr_check--;
				break;
			case CHECK_PRESSED:
				if (( requested & c) == requested)
					curr_check--;
				break;
			case CHECK_RELEASED:
				if (( requested & c) == 0)
					curr_check--;
				break;
		}
		if ( curr_check == -1)
			return 1;	// Bingo!
		pos--;
		count--;
	}

	return 0;
}


