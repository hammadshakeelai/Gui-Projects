# Makefile.win for Flappy Bird game

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -I.
LDFLAGS = -static -mwindows -lgdi32

# Project files
TARGET = FlappyBird.exe
SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del $(OBJECTS) $(TARGET)

.PHONY: all clean