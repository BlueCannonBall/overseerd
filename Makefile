CXX = g++
LIBS = -lX11
override CXXFLAGS += -Wall -s -Ofast -flto
TARGET = overseerd
PREFIX = /usr/local

$(TARGET): main.cpp
	$(CXX) $< $(CXXFLAGS) $(LIBS) -o $@

.PHONY: clean install

clean:
	$(RM) $(TARGET)

install:
	cp $(TARGET) $(PREFIX)/bin/$(TARGET)
