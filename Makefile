CXX = g++
CXXFLAGS = -Wall -std=c++17 $(shell pkg-config --cflags xft)
LIBS = -lX11 $(shell pkg-config --libs xft)

TARGET = myapp
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)