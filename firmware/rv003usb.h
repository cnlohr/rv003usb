#ifndef _RV003USB_H
#define _RV003USB_H

// Port D.3 = D+
// Port D.4 = D-
// Port D.5 = DPU

#define DEBUG_PIN 2
#define USB_DM 3     //DM MUST be BEFORE DP
#define USB_DP 4
#define USB_DPU 5
#define USB_PORT GPIOD
#define USB_GPIO_BASE USB_PORT##_BASE

#define USB_BUFFER_SIZE 16

#define USB_DMASK ((1<<(USB_DM)) | 1<<(USB_DP))

struct rv003usb_internal
{
	uint8_t usb_buffer[USB_BUFFER_SIZE];
	uint8_t setup_request;
	uint8_t my_address;
};

void usb_pid_handle_setup( uint32_t this_token, struct usb_internal_state_struct * ist ) __attribute__(naked);
void usb_pid_handle_in( uint32_t this_token, struct usb_internal_state_struct * ist ) __attribute__(naked);
void usb_pid_handle_out( uint32_t this_token, struct usb_internal_state_struct * ist )  __attribute__(naked);
void usb_pid_handle_data( uint32_t this_token, struct usb_internal_state_struct * ist, uint32_t which_data )  __attribute__(naked);
void usb_pid_handle_ack( uint32_t this_token, struct usb_internal_state_struct * ist ) __attribute__(naked);

//poly_function = 0 to include CRC.
//poly_function = 2 to exclude CRC.
//This function is provided in assembly
void usb_send_data( uint8_t * data, uint32_t length, uint32_t poly_function );

#endif

