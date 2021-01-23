#!/bin/make
CFLAGS =
LDFLAGS=
CC = gcc
CXX = g++

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

.cpp.o:
	$(CXX) $(CFLAGS) -c -o $*.o $<

all: speak testmotors sing

speak: speak.o
	$(CC) -o $@ $^ $(LDFLAGS) -lespeak

testmotors: testmotors.o
	$(CC) -o $@ $^ $(LDFLAGS)

sing: sing.o
	$(CC) -o $@ $^ $(LDFLAGS) -lm
