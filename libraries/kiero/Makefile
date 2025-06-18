CXX = g++
AR = ar

CFLAGS = -std=c++11 -g -Wall -Werror -Wformat
CFLAGS += -I../MinHook/include

libkiero.a: ./kiero.o
	$(AR) rcs $@ $^

./kiero.o: ./kiero.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	-del *.o
	-del *.a
