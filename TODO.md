### TODO

- Set up eclipse environment:
 - https://devzone.nordicsemi.com/tutorials/b/getting-started/posts/development-with-gcc-and-eclipse
 - https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
- Get example code working
- Read through existing code
- Work through the Bluetooth chip datasheet:
 - http://infocenter.nordicsemi.com/pdf/nRF52832_PS_v1.4.pdf
- Look into firmware updates via the Bluetooth module.
  - https://electronics.stackexchange.com/questions/116135/possible-to-re-program-microcontroller-over-bluetooth

  - custom bootloader to accept bin file using the same library used in updating the FPGA board. Ideally integrate this into the existing framework. Most work will be needed on the receiving end so that it can accept the new bin.  Checkout:
  nRF5_SDK_15.0.0_a53641a \ dfu \ secure_bootloader \ pca10040_ble


```c
  //main.c implant repo
  #include "incbin.h"

  //allows use of
  INCBIN(FPGAimg, "FPGAimage.bin");

  bitbang_spi(*(gFPGAimgData+i));
  // see -> external \ incbin-master \ README.md

  //look up this function inside of the main for use in sending the bin file
  //to the chip
  void bitbang_spi(uint8_t data){
    //code
  }

```
