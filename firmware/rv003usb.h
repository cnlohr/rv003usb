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

#define USB_BUFFER_SIZE 16

#define USB_DMASK ((1<<(USB_DM)) | 1<<(USB_DP))

#endif

