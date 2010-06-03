liborgparser.a : orgparser.o
	ar rcs liborgparser.a orgparser.o

orgparser.o : main.c orgparser.h
	gcc -Wall -std=c99 -s -c main.c -o orgparser.o
