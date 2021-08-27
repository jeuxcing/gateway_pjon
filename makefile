upload: compile
	arduino-cli upload -p ${TTY} --fqbn arduino:avr:nano gateway_pjon

compile: gateway_pjon.ino
	arduino-cli compile --fqbn arduino:avr:nano gateway_pjon.ino

list:
	arduino-cli board list
