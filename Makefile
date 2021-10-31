CC = g++
override CFLAGS += -Wall -Wno-unused-result -s -O2 -lX11 -lxdo
TARGET = discordd
PREFIX = /usr/local

$(TARGET): main.cpp
	$(CC) $< $(CFLAGS) -o $@

.PHONY: clean install

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(PREFIX)/bin/$(TARGET)
