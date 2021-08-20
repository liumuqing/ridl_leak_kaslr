all: leak uname
leak: leak.c
	gcc -O3 -o leak leak.c
uname: uname.c
	gcc -O3 -o uname uname.c
