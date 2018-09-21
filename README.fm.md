# 测试移植

## FM-V03

对照电路图，主要注意两个地方：

1. PA 选择 RF_PACONFIG_PASELECT_PABOOST
2. STM32 的引脚中断不能区分 Port。画电路时不要使用相同序号引脚。V3 电路中有这个问题，把 DIO3/4/5 置为 NC。对于 LoRa 调制方式，sx1276 驱动中使实际使用了 DIO0/1/2/3 这四个中断。

## 编译：

APPLICATION 有：

* LoRaMac
* ping-pong
* rx-sensi
* tx-cw


```
cmake -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain-arm-none-eabi.cmake" \
-DBOARD="FM-V03" -DAPPLICATION="LoRaMac" \
-DCLASS="classC" ..

cmake -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain-arm-none-eabi.cmake" \
-DBOARD="FM-V03" -DAPPLICATION="rx-sensi" \
-DCLASS="classC" ..
```

烧写

```
arm-none-eabi-gdb -nx --batch \
	-ex 'target extended-remote /dev/cu.usbmodem82A99FE1' \
	-x /opt/black_magic_probe_flash.scr \
	./src/apps/LoRaMac/LoRaMac-classA
	
arm-none-eabi-gdb -nx --batch \
    -ex 'target extended-remote /dev/cu.usbmodem82A99FE1' \
    -x /opt/black_magic_probe_flash.scr \
    ./src/apps/rx-sensi/rx-sensi
	
arm-none-eabi-gdb -nx --batch \
        -ex 'target extended-remote /dev/cu.usbmodem82A99FE1' \
        -x /opt/black_magic_probe_flash.scr \
        ./src/apps/ping-pong/ping-pong
```

