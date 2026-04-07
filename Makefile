CXX ?= g++
CXXFLAGS := -Wall -Wextra -std=c++17 -municode -Iinclude
LDFLAGS := -mwindows

TARGET := myapp.exe
SRC := main.cpp \
	src/MainWindow.cpp \
	src/AddNumberDialog.cpp \
	src/CalculatorEngine.cpp \
	src/NumberRepository.cpp \
	src/StringUtils.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /Q $(TARGET) 2>NUL || exit 0
