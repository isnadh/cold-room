ESP_32-OTA-STM32

"MUST Do It"

#copy to  platform.txt 
tools.stm32_ota.cmd=stm32_ota
tools.stm32_ota.path.macosx={runtime.hardware.path}/tools/macosx
tools.stm32_ota.path.linux={runtime.hardware.path}/tools/linux
tools.stm32_ota.path.linux64={runtime.hardware.path}/tools/linux64
tools.stm32_ota.upload.params.verbose=-d
tools.stm32_ota.upload.params.quiet=
tools.stm32_ota.upload.pattern="{path}/{cmd}" {upload.IPAddress} "{build.path}/{build.project_name}.bin"


# coppy to board.txt 
genericSTM32F103C.menu.upload_method.httpMethod=STM32 OTA
genericSTM32F103C.menu.upload_method.httpMethod.upload.protocol=stm32_ota
genericSTM32F103C.menu.upload_method.httpMethod.upload.tool=stm32_ota
genericSTM32F103C.menu.upload_method.httpMethod.upload.IPAddress=http://192.168.0.66 
# Change ip address to 8266 configure for router

file firmware upload to http :: 192.168.0.66


view this repo :: 

https://github.com/csnol/STM32-OTA

