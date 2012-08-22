# Do not change the lines below. It connects to XBoard.
# (if not compiled with XBOARD then most of it is ignored)
xboard
protover 2
accepted usermove
accepted ping
accepted myname
accepted san
accepted analyze
accepted colors
accepted sigint
accepted sigterm
accepted done
new
random
level 40 5 0
hard
ping 1
time 30000
otim 30000
force
dir
# More commands can be added below:
enter endgame database
eup KRNNK 0 4
eup KRNNK 1 4
eup KRBPK 0 4
eup KRBPK 1 4
eup KRBNK 0 4
eup KRBNK 1 4
quit
