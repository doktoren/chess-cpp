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
eup KBBNK 0 4
eup KBBNK 1 4
eup KRPPK 0 4
eup KRPPK 1 4
eup KRNPK 0 4
eup KRNPK 1 4
quit
