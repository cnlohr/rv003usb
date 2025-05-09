# usb cdc uart

This demo provides a USB-UART at 115200 baud with the following pinout:

| Signal             | Macro         | Port | Pin |
|--------------------|---------------|------|-----|
| USB D+             | USB_PIN_DP    | C    | 3   |
| USB D–             | USB_PIN_DM    | C    | 4   |
| USB pull-up (DPU)  | USB_PIN_DPU   | C    | 5   |
| UART TX            | —             | D    | 5   |
| UART RX            | —             | D    | 6   |

⚠️ This only works on linux as Windows apparently doesn't like low speed CDC devices... (untested but inferred from other projects from this repo)

it's been tested with continuous pastes at up to 256 characters both ways. Change the circular buffer sizes as needed.