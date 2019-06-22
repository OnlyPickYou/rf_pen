#include "../../proj/tl_common.h"
#include "../../proj_lib/blt_ll/blt_ll.h"

/**********************************************************************
 * LOCAL VARIABLES
 */
const u16 my_primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;
static const u16 my_characterUUID = GATT_UUID_CHARACTER;

const u16 my_gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;
const u16 my_devNameUUID = GATT_UUID_DEVICE_NAME;
const u16 my_appearanceUIID = 0x2a01;
const u16 my_periConnParamUUID = 0x2a04;

// Device Name Characteristic Properties
static u8 my_devNameCharacter = 0x12;

// Appearance Characteristic Properties
static u8 my_appearanceCharacter = 0x02;

// Peripheral Preferred Connection Parameters Characteristic Properties
static u8 my_periConnParamChar = 0x02;

u16 my_appearance = GAP_APPEARE_UNKNOWN;

typedef struct
{
  /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMin;
  /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMax;
  /** Number of LL latency connection events (0x0000 - 0x03e8) */
  u16 latency;
  /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
  u16 timeout;
} gap_periConnectParams_t;

gap_periConnectParams_t my_periConnParameters = {20, 40, 0, 1000};

const u8	my_devName [] = {'b', 'l', 'u', 'e', 'L', 'i', 'g', 'h', 't'};

//////////////////////// SPP /////////////////////////////////////////////////////

const u16 my_SppServiceUUID       			= TELINK_SPP_UUID_SERVICE;
const u16 my_SppDataServer2ClientUUID		= TELINK_SPP_DATA_SERVER2CLIENT;
const u16 my_SppDataClient2ServiceUUID		= TELINK_SPP_DATA_CLIENT2SERVER;

const u16 my_SppNameUUID		= GATT_UUID_CHAR_USER_DESC;
const u8  my_SppName[] = {'m', 'y', 'S', 'P', 'P'};


// Spp data from Server to Client characteristic variables
static u8 my_SppDataServer2ClientProp = CHAR_PROP_READ | CHAR_PROP_NOTIFY;

// Spp data from Client to Server characteristic variables
static u8 my_SppDataClient2ServerProp = CHAR_PROP_READ | CHAR_PROP_WRITE;

u8 my_SppDataServer2ClientData[20] = {0xf0};

u8 my_SppDataClient2ServerData[20];


// TM : to modify
attribute_t my_Attributes[] = {
	{13,0,0,0,0,0},	//

	// gatt
	{7,2,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,2,1,1,(u8*)(&my_characterUUID), 		(u8*)(&my_devNameCharacter), 0},
	{0,2,sizeof (my_devName), sizeof (my_devName),(u8*)(&my_devNameUUID), 			(u8*)(my_devName), 0},
	{0,2,1,1,(u8*)(&my_characterUUID), 		(u8*)(&my_appearanceCharacter), 0},
	{0,2,sizeof (my_appearance), sizeof (my_appearance),(u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,2,1,1,(u8*)(&my_characterUUID), 		(u8*)(&my_periConnParamChar), 0},
	{0,2,sizeof (my_periConnParameters), sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},

	// spp
	{6,2,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_SppServiceUUID), 0},
	{0,2,1,1,(u8*)(&my_characterUUID), 		(u8*)(&my_SppDataServer2ClientProp), 0},				//prop
	{0,2,1,1,(u8*)(&my_SppDataServer2ClientUUID), 	(u8*)(my_SppDataServer2ClientData), 0},	//value
	{0,2,1,1,(u8*)(&my_characterUUID), 		(u8*)(&my_SppDataClient2ServerProp), 0},				//prop
	{0,2,16,16,(u8*)(&my_SppDataClient2ServiceUUID), 	(u8*)(my_SppDataClient2ServerData), 0},//value
	{0,2,sizeof (my_SppName), sizeof (my_SppName),(u8*)(&my_SppNameUUID), (u8*)(my_SppName), 0},
};

void	my_att_init ()
{
	blt_set_att_table ((u8 *)my_Attributes);
}
