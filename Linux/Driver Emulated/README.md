The files in this folder have to be copied to the AnalogDevicesInc/Linux folder in order to work.
It contains the driver and the devicetree necessary for the driver to be launched.
In order to compile you will have to export some variables:
    "export ARCH=arm"
    "export CROSS_COMPILE=<path-to-compiler>/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-"
With "make -j<number-of-threads>" you can create the uImage, but before that 
you will have to use "make versatile-pb-adi-emu.dtb" in order to create the devicetree blob and "make versatile_adi_defconfig"
in order to create the config file.

You can use qemu to launch an emulator to test out the driver using:
    "qemu-system-arm  -machine versatilepb  -kernel arch/arm/boot/zImage  -dtb arch/arm/boot/dts/versatile-pb-adi-emu.dtb  -drive file=<path-to-rootfs>/iio_rootfs/rootfs.ext2,if=scsi,format=raw  -append "root=/dev/sda console=ttyAMA0,115200"  -serial stdio -net nic,model=rtl8139 -net user,hostfwd=tcp::30431-:30431,hostfwd=tcp::2222-:22"
and you can connect to the emulator via:
    "ssh root@localhost -p 2222"
