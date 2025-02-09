#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "tusb_types.h"
#include "cdc.h"
#include "rv003usb.h"

void ProcessRNDISControl();

int trigger_tx_packet;

int main()
{
	SystemInit();
	usb_setup();

	uint32_t next_event = SysTick->CNT + 48000000;

	while(1)
	{
		do
		{
			uint32_t * ue = GetUEvent();
			if( !ue )
				break;
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
		} while(1);

		if( (int32_t)( SysTick->CNT - next_event ) > 0 )
		{
			trigger_tx_packet = 1;
			next_event = SysTick->CNT + 48000000;
		}

		ProcessRNDISControl();
	}
}

#define RNDIS_MSG_COMPLETION	0x80000000
#define RNDIS_MSG_PACKET	0x00000001	/* 1-N packets */
#define RNDIS_MSG_INIT		0x00000002
#define RNDIS_MSG_INIT_C	(RNDIS_MSG_INIT|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_HALT		0x00000003
#define RNDIS_MSG_QUERY		0x00000004
#define RNDIS_MSG_QUERY_C	(RNDIS_MSG_QUERY|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_SET		0x00000005
#define RNDIS_MSG_SET_C		(RNDIS_MSG_SET|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_RESET		0x00000006
#define RNDIS_MSG_RESET_C	(RNDIS_MSG_RESET|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_INDICATE	0x00000007
#define RNDIS_MSG_KEEPALIVE	0x00000008
#define RNDIS_MSG_KEEPALIVE_C	(RNDIS_MSG_KEEPALIVE|RNDIS_MSG_COMPLETION)

#define RNDIS_MAJOR_VERSION		0x00000001
#define RNDIS_MINOR_VERSION		0x00000000

/* codes for "status" field of completion messages */
#define	RNDIS_STATUS_SUCCESS			0x00000000
#define RNDIS_STATUS_PENDING			0x00000103

#define RNDIS_DF_CONNECTIONLESS		0x00000001U
#define RNDIS_DF_CONNECTION_ORIENTED	0x00000002U
#define RNDIS_DF_RAW_DATA		0x00000004U

// https://learn.microsoft.com/en-us/previous-versions/ff570621(v=vs.85)
// https://www.utasker.com/docs/uTasker/uTaskerRNDIS.pdf
uint32_t REMOTE_NDIS_INITIALIZE_CMPLT[13] = {
	RNDIS_MSG_INIT_C, 52,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS,
	RNDIS_MAJOR_VERSION, RNDIS_MINOR_VERSION,
	RNDIS_DF_CONNECTIONLESS, 0, // "connectionless"
	1, 1558,
	3, 0, 0
};

uint32_t REMOTE_NDIS_SET_CMPLT[4] = {
	RNDIS_MSG_SET_C, 16,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS
};

/* Required Object IDs (OIDs) */
#define OID_GEN_SUPPORTED_LIST            0x00010101
#define OID_GEN_HARDWARE_STATUS           0x00010102
#define OID_GEN_MEDIA_SUPPORTED           0x00010103
#define OID_GEN_MEDIA_IN_USE              0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD         0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE        0x00010106
#define OID_GEN_LINK_SPEED                0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE     0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE      0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE       0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE        0x0001010B
#define OID_GEN_VENDOR_ID                 0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION        0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER     0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD         0x0001010F
#define OID_GEN_DRIVER_VERSION            0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE        0x00010111
#define OID_GEN_PROTOCOL_OPTIONS          0x00010112
#define OID_GEN_MAC_OPTIONS               0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS      0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS      0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION     0x00010116
#define OID_GEN_SUPPORTED_GUIDS           0x00010117
#define OID_GEN_NETWORK_LAYER_ADDRESSES   0x00010118
#define OID_GEN_TRANSPORT_HEADER_OFFSET   0x00010119
#define OID_GEN_MACHINE_NAME              0x0001021A
#define OID_GEN_RNDIS_CONFIG_PARAMETER    0x0001021B
#define OID_GEN_VLAN_ID                   0x0001021C

#define OID_802_3_CURRENT_ADDRESS         0x01010102
#define OID_802_3_MAXIMUM_LIST_SIZE       0x01010104
#define OID_802_3_PERMANENT_ADDRESS       0x01010101
#define OID_GEN_RCV_OK                    0x00020102
#define OID_GEN_XMIT_OK                   0x00020101 


// from uTaskerRNDIS.doc
const uint32_t REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST[] = {
	RNDIS_MSG_QUERY_C, 0x00000080,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS,
	0x00000064, 0x00000010,
	OID_GEN_SUPPORTED_LIST, /* Can be replaced*/


	0x00010102, // OID_GEN_HARDWARE_STATUS
	0x00010103, // OID_GEN_MEDIA_SUPPORTED
	0x00010104, // OID_GEN_MEDIA_IN_USE
	0x00010106, // OID_GEN_MAXIMUM_FRAME_SIZE
	0x00010107, // OID_GEN_LINK_SPEED
	0x0001010A, // OID_GEN_TRANSMIT_BLOCK_SIZE
	0x0001010B, // OID_GEN_RECEIVE_BLOCK_SIZE
	0x0001010C, // OID_GEN_VENDOR_ID
	0x0001010D, // OID_GEN_VENDOR_DESCRIPTION
	0x0001010E, // OID_GEN_CURRENT_PACKET_FILTER
	0x00010111, // OID_GEN_MAXIMUM_TOTAL_SIZE
	0x00010114, // OID_GEN_MEDIA_CONNECT_STATUS
	0x00010116, // OID_GEN_VENDOR_DRIVER_VERSION
	0x00020101, // OID_GEN_XMIT_OK
	0x00020102, // OID_GEN_RCV_OK
	0x00020103, // OID_GEN_XMIT_ERROR
	0x00020104, // OID_GEN_RCV_ERROR
	0x00020105, // OID_GEN_RCV_NO_BUFFER
	0x01010101, // OID_802_3_PERMANENT_ADDRESS [start of 802.3 Ethernet operational characteristics]
	0x01010102, // OID_802_3_CURRENT_ADDRESS
	0x01010103, // OID_802_3_MULTICAST_LIST
	0x01010104, // OID_802_3_MAXIMUM_LIST_SIZE
	0x01020101, // OID_802_3_RCV_ERROR_ALIGNMENT [start of Ethernet statistics]
	0x01020102, // OID_802_3_XMIT_ONE_COLLISION
	0x01020103, // OID_802_3_XMIT_MORE_COLLISIONS*/
#if 0
	RNDIS_MSG_QUERY_C, 0x00000048,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS,
	0x00000030, 0x00000010,
	OID_GEN_SUPPORTED_LIST, /* Can be replaced*/
	OID_802_3_CURRENT_ADDRESS,
	OID_802_3_PERMANENT_ADDRESS,
	OID_GEN_SUPPORTED_LIST,
	OID_802_3_MAXIMUM_LIST_SIZE,
	OID_GEN_LINK_SPEED,
	OID_GEN_MAXIMUM_TOTAL_SIZE,
	OID_GEN_MAXIMUM_FRAME_SIZE,
	OID_GEN_MEDIA_CONNECT_STATUS,
	OID_GEN_RCV_OK,
	OID_GEN_XMIT_OK,
	OID_GEN_VENDOR_DESCRIPTION
#endif

};

const uint32_t REMOTE_NDIS_RESET_CMPLT[] = {
	RNDIS_MSG_RESET_C, 0x00000010,
	RNDIS_STATUS_SUCCESS, 0
};


const uint32_t REMOTE_NDIS_KEEPALIVE_CMPLT[] = {
	RNDIS_MSG_KEEPALIVE_C, 0x00000010,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS
};


const uint32_t REMOTE_NDIS_QUERY_CMPLT_property[] = {
	RNDIS_MSG_QUERY_C, 0x00000018,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS,
	0x00000004, 0x00000010,
	0xaaaaaaaa,
};

const uint32_t REMOTE_NDIS_QUERY_CMPLT_OID_802_3_CURRENT_ADDRESS[] = {
	RNDIS_MSG_QUERY_C, 0x0000001c,
	0xaaaaaaaa, RNDIS_STATUS_SUCCESS,
	0x00000006, 0x00000010,
	0xaaaaaaaa,
	0xaaaaaaaa,
};

uint32_t RNDIS_SCRATCH[36]; // First 4 bytes are confirmation.
volatile uint32_t RNDIS_OUT;
uint32_t * ep0replyBack;  // Note: Must be aligned.s
volatile int32_t ep0replylenBack;


void ProcessRNDISControl()
{
	uint32_t RNDIS_IN = RNDIS_SCRATCH[0];
	uint32_t * scratch = &RNDIS_SCRATCH[1];

	if( RNDIS_IN )
	{
		uint32_t reply = scratch[2];
		switch( scratch[0] )
		{
			case RNDIS_MSG_INIT: // 2
				memcpy( scratch, REMOTE_NDIS_INITIALIZE_CMPLT, sizeof( REMOTE_NDIS_INITIALIZE_CMPLT ) );
				scratch[2] = reply;
				RNDIS_OUT = sizeof( REMOTE_NDIS_INITIALIZE_CMPLT );
				break;
			case RNDIS_MSG_SET: // 5
				switch( scratch[3] )
				{
				case OID_GEN_CURRENT_PACKET_FILTER:
				default:
					LogUEvent( 1234, scratch[3], scratch[6], scratch[7] );
					memcpy( scratch, REMOTE_NDIS_SET_CMPLT, sizeof( REMOTE_NDIS_SET_CMPLT ) );
					scratch[2] = reply;
					RNDIS_OUT = sizeof( REMOTE_NDIS_SET_CMPLT );
				}
				break;
			case RNDIS_MSG_RESET: // 6
				memcpy( scratch, REMOTE_NDIS_RESET_CMPLT, sizeof(REMOTE_NDIS_RESET_CMPLT) );
				//scratch[2] = reply;
				RNDIS_OUT = sizeof( REMOTE_NDIS_RESET_CMPLT );
				break;
			case RNDIS_MSG_KEEPALIVE: // 8
				memcpy( scratch, REMOTE_NDIS_KEEPALIVE_CMPLT, sizeof(REMOTE_NDIS_KEEPALIVE_CMPLT) );
				scratch[2] = reply;
				RNDIS_OUT = sizeof( REMOTE_NDIS_KEEPALIVE_CMPLT );
				break;
			case RNDIS_MSG_QUERY: // 4
			{
				uint32_t oid = scratch[3];
				//temp = scratch[3];  // OID we are interested in.
				switch( oid )
				{
				case OID_802_3_CURRENT_ADDRESS:
				case OID_802_3_PERMANENT_ADDRESS:
					memcpy( scratch, REMOTE_NDIS_QUERY_CMPLT_OID_802_3_CURRENT_ADDRESS, sizeof(REMOTE_NDIS_QUERY_CMPLT_OID_802_3_CURRENT_ADDRESS) );
					scratch[2] = reply;
					scratch[6] = 0x22222222;//(ESIG->UID0 & (~1)) | 2;
					scratch[7] = 0x22222222;//ESIG->UID1;
					RNDIS_OUT = sizeof( REMOTE_NDIS_QUERY_CMPLT_OID_802_3_CURRENT_ADDRESS );
					break;
				case OID_GEN_SUPPORTED_LIST:
					memcpy( scratch, REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST, sizeof(REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST) );
					scratch[2] = reply;
					LogUEvent( 33, RNDIS_OUT, ep0replylenBack, 0 );
					RNDIS_OUT = sizeof( REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST );
					break;
				default:
					memcpy( scratch, REMOTE_NDIS_QUERY_CMPLT_property, sizeof(REMOTE_NDIS_QUERY_CMPLT_property) );
					scratch[2] = reply;
					switch( oid )
					{
					case OID_GEN_VENDOR_ID:           scratch[6] = 0xff; break;
					case OID_802_3_MAXIMUM_LIST_SIZE: scratch[6] = 0; break;
					case OID_GEN_LINK_SPEED:          scratch[6] = 80; break; // 8Kbps
					case OID_GEN_MAXIMUM_TOTAL_SIZE:  scratch[6] = 1024+12;  break;
					case OID_GEN_MAXIMUM_FRAME_SIZE:  scratch[6] = 1024;  break;
					case OID_GEN_XMIT_OK:             scratch[6] = 7788;  break;
					default:
						LogUEvent( 11111, oid, 0, 0 );
					case OID_GEN_HARDWARE_STATUS: // 0 = NdisHardwareStatusReady
					case OID_GEN_RCV_OK: // Ignored
					case OID_GEN_VENDOR_DESCRIPTION:
					case OID_GEN_MEDIA_CONNECT_STATUS: // 0 = NdisMediaStateConnected
						scratch[6] = 0; // default
					}
					RNDIS_OUT = sizeof( REMOTE_NDIS_QUERY_CMPLT_property );
					break;
				}
				break;
			}
			default:
				LogUEvent( 8889, scratch[0], scratch[1], scratch[2] );
				break;
		}
		RNDIS_SCRATCH[0] = 0;
	}
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		if( RNDIS_OUT && ep0replylenBack == 0 )
		{
			uint32_t pair[2];
			static uint32_t paired;
			paired++;
			pair[0] = 0x01;
			pair[1] = 0;
			usb_send_data( pair, 8, 0, sendtok );
			//LogUEvent( 0, 0x33333333, pair[0], pair[1] );
			ep0replyBack = (void*)(&RNDIS_SCRATCH[1]);
			ep0replylenBack = RNDIS_OUT;
			//LogUEvent( 0, 0x34443333, RNDIS_SCRATCH[0], RNDIS_SCRATCH[1] );
			RNDIS_OUT = 0;
			return;
		}
	}
	if( endp == 3 )
	{
		static uint32_t pkt[] =  {

			RNDIS_MSG_PACKET,
			0x70, // total len
			0x24, // offsetc
			0x44, // length
			0, 0, 0, // OOB
			0, 0, // Per-packet.
			0, 0, 
			0xffffffff,
			0x3322ffff,
			0x22222222,
			0x00450008,
			0x84463200,
			0x11100000,
			0xa8c0fefe, // 192.168
			0xffff0104, // 8.8 in IP
			0x3ee1ffff,
			0x1e00e46c,
			0xfe5e0128,
			0x01000000,
			0x00000000,
			0x77040000,
			0x00646170,
			0x01000100,
			0x01000100,
		};
			
		if( trigger_tx_packet )
		{
			uint32_t ts = trigger_tx_packet - 1;

			if( ts >= sizeof(pkt)/4 ) 
			{
				usb_send_empty( sendtok );
				trigger_tx_packet = 1;
			}
			else
			{
				usb_send_data( pkt + ts, 8, 0, sendtok );
				trigger_tx_packet += 2;
			}
		}
		else
		{
			usb_send_empty( sendtok );
		}
		return;
	}
	usb_send_empty( sendtok );
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	if( s->wRequestTypeLSBRequestMSB == 0x80a1)
	{
		e->opaque = (uint8_t*)"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		e->max_len = 0x1c;
	}
	else if( s->wRequestTypeLSBRequestMSB == ( ( USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE ) | ( CDC_REQUEST_SEND_ENCAPSULATED_COMMAND << 8 ) ) )
	{
		// 0x021 -> Incoming data.
		e->opaque = (void*)RNDIS_SCRATCH;
		e->max_len = s->wLength;
		ist->setup_request = 2;
		//LogUEvent( 0, 0x5555, (unsigned)e->opaque, e->max_len );
	}
	else if( s->wRequestTypeLSBRequestMSB == ( ( USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE ) | ( CDC_REQUEST_GET_ENCAPSULATED_RESPONSE << 8 ) ) )
	{
		// 0x1a1
		// See https://android.googlesource.com/kernel/msm/+/android-5.0.2_r0.2/drivers/usb/gadget/f_rndis.c
		// and see https://www.utasker.com/docs/uTasker/uTaskerRNDIS.pdf
		e->opaque = (void*)ep0replyBack;
		e->max_len = ep0replylenBack;
		ep0replyBack = 0;
		ep0replylenBack = 0;
	}
	//LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	//LogUEvent( SysTick->CNT, current_endpoint, ((uint32_t*)data)[0], ((uint32_t*)data)[1] );
	e->count++;
}




