TARGET = ./np_simple ./np_single_proc ./np_multi_proc
SRC = $(TARGET:=.cpp)

.PHONY: all clean

all: $(TARGET)

$(TARGET): % : %.cpp
	g++ $< -o $@

clean:
	rm $(TARGET)
