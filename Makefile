all: ECP TES user 
ECP: ECP.c ECP.h
	gcc -o ECP ECP.c
TES: TES.c
	gcc -o TES TES.c
user: user.c
	gcc -o user user.c

clean:
	rm ECP TES user

