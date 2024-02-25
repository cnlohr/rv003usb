# Touch Device

Connected to touch pads,

| Pad | Button |
| -- | -- |
| A1 | BTN_LEFT |
| D6 | BTN_RIGHT |
| A2 | BTN_UP |
| D5 | BTN_DOWN |
| D4 | BTN_SELECT |
| D3 | BTN_START |
| C4 | BTN_A |
| D2 | BTN_B |

It maps left/right and up/down to axes, and all the buttons to buttons.

Assuming USB is as follows:
```
#define USB_PIN_DP 2
#define USB_PIN_DM 1
#define USB_PIN_DPU 0
#define USB_PORT C
```

