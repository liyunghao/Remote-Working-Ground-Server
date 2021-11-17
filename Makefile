TARGET = ./np_simple ./np_single_proc ./np_multi_proc
SRC = $(TARGET:=.cpp)

.PHONY: all clean

all: $(TARGET) pack.hpp

$(TARGET): % : %.cpp
	g++ -lpthread -lrt $< -o $@

1: np_simple
	./np_simple 5057


2: np_single_proc
	./np_single_proc 5056

3: np_multi_proc
	./np_multi_proc 5058

%: %.cpp
	g++ -lpthread -lrt $< -o $@
	
clean:
	rm $(TARGET)
