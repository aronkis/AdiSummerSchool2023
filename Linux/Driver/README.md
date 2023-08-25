The files in this folder have to be copied to the AnalogDevicesInc/Linux folder in order to work.
It contains the driver and the devicetree necessary for the driver to be launched.
In order to compile you will have to export some variables:
    "export ARCH=arm"
    "export CROSS_COMPILE=<path-to-compiler>/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-"
With "make -j<number-of-threads> UIMAGE_LOADADDR=0x8000 uImage" you can create the uImage, but before that 
you will have to use "make adi-ad5592r.dtb" in order to create the devicetree blob and "make zynq_xcomm_adv7511_defconfig" to 
create the config file.