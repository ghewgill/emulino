env = Environment(CFLAGS = "-Wall -Werror")
env.Program("emulino", ["emulino.c", "eeprom.c", "usart.c"])
env.Command("avr.inc", ["mkinst.py", "instructions.txt"], "/opt/local/bin/python2.5 mkinst.py")
