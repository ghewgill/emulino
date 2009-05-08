env = Environment(CPPFLAGS = "-Wall -Werror")
env.Program("emulino", ["emulino.cpp"])
env.Command("avr.inc", ["mkinst.py", "instructions.txt"], "/opt/local/bin/python2.5 mkinst.py")
