CXX = g++
CXXFLAGS = -std=c++17 -Wall

SRC = main.cpp core/ollama_interface.cpp core/memory.cpp core/system_handler.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = lurkuitae_terminal

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(TARGET)
