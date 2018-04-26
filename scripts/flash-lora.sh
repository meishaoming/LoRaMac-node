arm-none-eabi-gdb -nx --batch \
	-ex 'target extended-remote /dev/cu.usbmodem82A99FE1' \
	-x /opt/black_magic_probe_flash.scr \
	./src/apps/LoRaMac/LoRaMac-classA

