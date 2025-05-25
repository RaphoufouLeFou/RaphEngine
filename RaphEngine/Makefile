# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++23 -Iinclude -I/usr/include/freetype2 -I. `sdl2-config --cflags` -DRAPHENGINE_EXPORTS -fPIC
LDFLAGS :=


# Source files and output
SRC := $(shell find . -name '*.cpp')
OBJ := $(patsubst %.cpp, Build/Objs/%.o, $(SRC))
TARGET := RaphEngine

# Build rules
all: $(TARGET)
	

$(TARGET): $(OBJ)
	ar rcs Build/lib$@.a $^
	sudo cp Build/lib$@.a /usr/local/lib

	sudo rm -rf /usr/local/include/RaphEngine
	sudo mkdir -p /usr/local/include/RaphEngine
	sudo cp include/*.h /usr/local/include/RaphEngine

Build/Objs/%.o: %.cpp
	mkdir -p Build/Objs
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f Build/Objs/*.o Build/lib$(TARGET).a
	rm -rf Build/

.PHONY: all clean
