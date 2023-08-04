#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "tusb_types.h"
#include "cdc.h"
#include "rv003usb.h"


void ProcessRNDISControl();

int main()
{
	SystemInit();
	usb_setup();
	while(1)
	{
		uint32_t * ue = GetUEvent();
		if( ue )
		{
			printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
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
//https://www.utasker.com/docs/uTasker/uTaskerRNDIS.pdf
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


// from uTaskerRNDIS.doc
uint32_t REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST[] = {
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
	0x01020103, // OID_802_3_XMIT_MORE_COLLISIONS
};
uint32_t RNDIS_SCRATCH[36];
uint32_t RNDIS_IN;
uint32_t RNDIS_OUT;

void ProcessRNDISControl()
{
	if( RNDIS_IN )
	{
		LogUEvent( 8888, RNDIS_SCRATCH[0], RNDIS_SCRATCH[1], RNDIS_SCRATCH[2] );
		uint32_t reply = RNDIS_SCRATCH[2];
		uint32_t temp = 0;
		switch( RNDIS_SCRATCH[0] )
		{
			case RNDIS_MSG_INIT:
				memcpy( RNDIS_SCRATCH, REMOTE_NDIS_INITIALIZE_CMPLT, sizeof( REMOTE_NDIS_INITIALIZE_CMPLT ) );
				RNDIS_SCRATCH[2] = reply;
				RNDIS_OUT = sizeof( REMOTE_NDIS_INITIALIZE_CMPLT );
				break;

			case RNDIS_MSG_SET:
				memcpy( RNDIS_SCRATCH, REMOTE_NDIS_SET_CMPLT, sizeof( REMOTE_NDIS_SET_CMPLT ) );
				RNDIS_SCRATCH[2] = reply;
				RNDIS_OUT = sizeof( REMOTE_NDIS_SET_CMPLT );
				break;

			case RNDIS_MSG_QUERY:
				temp = RNDIS_SCRATCH[3];  // OID we are interested in.
				memcpy( RNDIS_SCRATCH, REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST, sizeof( REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST ) );
				RNDIS_SCRATCH[2] = reply;
				RNDIS_OUT = sizeof( REMOTE_NDIS_QUERY_CMPLT_OID_GEN_SUPPORTED_LIST );
				LogUEvent( 8889, RNDIS_SCRATCH[2], RNDIS_SCRATCH[3], RNDIS_OUT );
				break;

		}
		RNDIS_IN = 0;
	}
}

uint32_t * ep0replyBack;
int32_t ep0replylenBack;

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		if( RNDIS_OUT )
		{
			usb_send_data( "\x01\00\x00\x00\x00\x00\x00\x00", 8, 0, sendtok );
			LogUEvent( 0, 0x33333333, 0, 0 );

			ep0replyBack = RNDIS_SCRATCH;
			ep0replylenBack = RNDIS_OUT;
			RNDIS_OUT = 0;

/*
			int tsend = (ep1replylen > 8 )? 8 : ep1replylen;
			usb_send_data( ep1reply, tsend, 0, sendtok );
			LogUEvent( 1111, endp, ep1reply[0], ep1reply[1] );
			ep1reply += tsend/4;
			ep1replylen -= tsend;
			e->count++;
*/
			return;
		}
	}
	else
	{
		LogUEvent( SysTick->CNT, endp, 0, -1 );
		usb_send_empty( sendtok );
	}
/*
	if( endp )
	{
		static uint8_t tsajoystick[8] = { 0x00, 0x01, 0x10, 0x00 };
		tsajoystick[0]++;  // Go left->right fast
		tsajoystick[2]^=1; // Alter button 1.
		usb_send_data( tsajoystick, 3, 0, sendtok );
	}
	else
	{
		// If it's a control transfer, nak it.
		usb_send_empty( sendtok );
	}
*/
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s )
{
	if( s->wRequestTypeLSBRequestMSB == 0x80a1)
	{
		e->opaque = (uint8_t*)"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		e->max_len = 0x1c;
	}
	else if( s->wRequestTypeLSBRequestMSB == ( ( USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE ) | ( CDC_REQUEST_SEND_ENCAPSULATED_COMMAND << 8 ) ) )
	{
		e->max_len = s->wLength;
		e->opaque = 1;
	}
	else if( s->wRequestTypeLSBRequestMSB == ( ( USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE ) | ( CDC_REQUEST_GET_ENCAPSULATED_RESPONSE << 8 ) ) )
	{
		// 0x1a1
		// See https://android.googlesource.com/kernel/msm/+/android-5.0.2_r0.2/drivers/usb/gadget/f_rndis.c
		// and see https://www.utasker.com/docs/uTasker/uTaskerRNDIS.pdf
		e->opaque = ep0replyBack;
		e->max_len = ep0replylenBack;
		LogUEvent( 0, 0xaaaa5555, ep0replyBack, ep0replylenBack );
		ep0replyBack = 0;
		ep0replylenBack = 0;
	}
	//LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	static int copyp2;
	if( e->opaque == 1 )
	{
/*
		if( e->count == 0 )
		{
			switch( ((uint32_t*)data)[0] )
			{
			case RNDIS_MSG_INIT:
				ep1reply = &REMOTE_NDIS_INITIALIZE_CMPLT[0];
				ep1replylen = sizeof(REMOTE_NDIS_INITIALIZE_CMPLT);
				break;
			case RNDIS_MSG_SET:
				ep1reply = &REMOTE_NDIS_SET_CMPLT[0];
				ep1replylen = sizeof(REMOTE_NDIS_SET_CMPLT);
				break;
			case RNDIS_MSG_QUERY:
				ep1reply = &REMOTE_NDIS_QUERY_CMPLT[0];
				ep1replylen = sizeof(REMOTE_NDIS_QUERY_CMPLT);
				copyp2 = 1; // for OID
				break;
			default:
				LogUEvent( 7777, current_endpoint, ((uint32_t*)data)[0], ((uint32_t*)data)[1] );
				ep1replylen = 0;
				break;
			}
		}
		else if( e->count == 1 )
		{
			if( ep1reply )
				ep1reply[2] = ((uint32_t*)data)[0];
		}
		else if( e->count == 2 )
		{
			if( queryreply )
			{
				uint32_t oid = ((uint32_t*)data)[0];
				ep1reply[4] = oid;
				if( oid == 
				copyp2 = 0;
			}
		}
*/
		int rem = e->max_len - (e->count<<3);
		int place = e->count << 1;
		if( rem > 0 && place < sizeof(RNDIS_SCRATCH) )
		{
			RNDIS_SCRATCH[place] = ((uint32_t*)data)[0];
			RNDIS_SCRATCH[place+1] = ((uint32_t*)data)[1];
		}
		//LogUEvent( 1010, rem, place, 0 );
		if( rem <= 8 )
		{
			RNDIS_IN = e->max_len >> 2;
			e->opaque = 0;
		}
		//LogUEvent( 9999, e->count, ((uint32_t*)data)[0], ((uint32_t*)data)[1] );
	}
	else
	{
		LogUEvent( SysTick->CNT, current_endpoint, ((uint32_t*)data)[0], ((uint32_t*)data)[1] );
	}
	e->count++;
}




