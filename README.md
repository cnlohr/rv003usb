# Using USB on the CH32V003

Project is not ready for prime time.  Proof of concept works, we are on track for proper releases soon.  See the list below.  Can you help out?

  :white_check_mark: Able to go on-bus and receive USB frames  
  :white_check_mark: Compute CRC in-line while receiving data  
  :black_square_button: Bit Stuffing (Works, needs more testing)  
  :white_check_mark: Sending USB Frames  
  :white_check_mark: High Level USB Stack in C  
  :black_square_button: Make USB timing more precise.  
  :white_square_button: Rework sending code to send tokens/data separately. (CL)  
  :white_square_button: Further optimize Send/Receive PHY code.  
  * :white_square_button: Enable systick-based retiming to correct timing mid-frame.  
  :white_square_button: Optimize high-level stack.  
  :white_square_button: Fit in bootloader  
  :white_square_button: Use HID custom messages.  
  :white_square_button: API For self-flashing + printf  


but not really anything else

this is just a checkin for end of stream #5

# rv003usb
Bit banged USB stack for the RISC-V WCH CH32V003
