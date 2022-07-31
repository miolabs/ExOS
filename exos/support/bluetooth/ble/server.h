#ifndef EXOS_BLE_SERVER_H
#define EXOS_BLE_SERVER_H

#include <kernel/list.h>

typedef char EXOS_BLE_UUID[16];
#define BLE_SERVICE_UUID(u16) { } // TODO
#define BLE_CHAR_UUID(u16) { } // TODO

typedef struct __EXOS_BLE_SERVICE EXOS_BLE_SERVICE;
typedef struct __EXOS_BLE_CHAR EXOS_BLE_CHAR;

typedef enum
{
	EXOS_BLE_OK = 0,
	EXOS_BLE_ERROR_REJECTED,
	EXOS_BLE_ERROR_SERVICE_INVALID,
	EXOS_BLE_ERROR_SERVICE_NOT_READY,
	EXOS_BLE_ERROR_CHARACTERISTIC_INVALID,
	EXOS_BLE_ERROR_CHARACTERISTIC_ALREADY_ADDED,
	EXOS_BLE_ERROR_BOND_REQUIRED,
} EXOS_BLE_ERROR;

typedef enum
{
	EXOS_BLE_EVENT_NOTHING = 0,
	EXOS_BLE_EVENT_CONNECT,
	EXOS_BLE_EVENT_DISCONNECT,
	EXOS_BLE_EVENT_READ_CHARACTERISTIC,
	EXOS_BLE_EVENT_WRITE_CHARACTERISTIC,
} EXOS_BLE_EVENT;

typedef enum
{
	EXOS_BLE_ATT_OK = 0,
	EXOS_BLE_ATT_NOT_IMPLEMENTED,
	// TODO: check response codes for compatibility with CoreBluetooth / Nordic ACI / whatever
} EXOS_BLE_ATT_ERROR;

typedef struct
{
	EXOS_BLE_EVENT Event;
	EXOS_BLE_CHAR *Characteristic;
	void *Data;
	unsigned short Offset;
	unsigned short Length;
} EXOS_BLE_REQUEST;

typedef struct
{
	unsigned short Dummy;
	unsigned char Passkey[6];
} EXOS_BLE_BOND_DATA;

typedef EXOS_BLE_ATT_ERROR (* EXOS_BLE_SERVICE_HANDLER)(EXOS_BLE_SERVICE *service, EXOS_BLE_REQUEST *req);
typedef EXOS_BLE_ATT_ERROR (* EXOS_BLE_BOND_CALLBACK)(EXOS_BLE_BOND_DATA *data);

typedef enum
{
	EXOS_BLE_SERVICE_NONE = 0,
	EXOS_BLE_SERVICE_PRIMARY = (1<<0),
	EXOS_BLE_SERVICE_CREATE_MASK = EXOS_BLE_SERVICE_PRIMARY,
} EXOS_BLE_SERVICE_FLAGS;

struct __EXOS_BLE_SERVICE
{
	EXOS_NODE Node;
	EXOS_BLE_SERVICE_FLAGS Flags;
	const EXOS_BLE_UUID *UUID;
	EXOS_BLE_SERVICE_HANDLER Handler;
	EXOS_LIST Characteristics;
};

struct __EXOS_BLE_CHAR
{
	EXOS_NODE Node;
	EXOS_BLE_SERVICE *Service; // NOTE: NULL when not added to any service
	const EXOS_BLE_UUID *UUID;
	unsigned short Size;
	unsigned char Index;
	unsigned char Reserved;
};

typedef struct
{
	EXOS_LIST Services;
} EXOS_BLE_SERVER;

typedef enum
{
	EXOS_BLECF_NONE = 0,
	EXOS_BLECF_BOND = (1<<0),
} EXOS_BLE_CONNECT_FLAGS;

// prototypes
void exos_ble_server_initialize(EXOS_BLE_BOND_CALLBACK callback);
void exos_ble_service_create(EXOS_BLE_SERVICE *service, const EXOS_BLE_UUID *uuid, EXOS_BLE_SERVICE_HANDLER handler, EXOS_BLE_SERVICE_FLAGS flags);
void exos_ble_server_add_service(EXOS_BLE_SERVICE *service);
EXOS_BLE_ERROR exos_ble_service_add_char(EXOS_BLE_SERVICE *service, EXOS_BLE_CHAR *characteristic);
EXOS_BLE_ERROR exos_ble_server_start_advertising(EXOS_BLE_SERVICE *services[], unsigned int count, unsigned int adv_interval, EXOS_BLE_CONNECT_FLAGS flags);
EXOS_BLE_ERROR exos_ble_server_disconnect();

int exos_ble_server_is_advertising();
int exos_ble_server_is_connected();
int exos_ble_server_is_bonded();
int exos_ble_server_wait_connexion(int timeout);

void exos_ble_characteristic_create(EXOS_BLE_CHAR *characteristic, const EXOS_BLE_UUID *uuid, unsigned short size);
EXOS_BLE_ERROR exos_ble_update_characteristic(EXOS_BLE_CHAR *characteristic, void *data);

// hal callbacks
void exos_ble_send_event_to_all_services(EXOS_BLE_EVENT event);
void exos_ble_handle_received_data(int pipe, unsigned char *data, int data_length);
void exos_ble_bond_event(EXOS_BLE_BOND_DATA *data);

#endif // EXOS_BLE_SERVER_H

