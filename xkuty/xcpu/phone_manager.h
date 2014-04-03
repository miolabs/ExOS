
#define XCPU_PHONE_LOGS  6
#define XCPU_VIEW_PHONES  (XCPU_PHONE_LOGS - 1)
#define XCPU_NEW_PHONE   XCPU_VIEW_PHONES

#define XCPU_PHONE_NAME_LEN (19)

typedef struct
{
	char flags;		// Bit 0: Slot empty or used, bit 1: name complete or incomplete
	char name[XCPU_PHONE_NAME_LEN];	// Bigger names will not fit the screen 
} XCPU_PHONE_REG;

int phone_name_cheksum      (XCPU_PHONE_REG* phone);
int phone_manager_count     (XCPU_PHONE_REG* phones);
void phone_manager_confirm  (XCPU_PHONE_REG* phones);
void phone_manager_remove   (XCPU_PHONE_REG* phones, int entry, unsigned int checksum);