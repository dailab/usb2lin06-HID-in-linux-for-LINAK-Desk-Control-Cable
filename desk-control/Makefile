TARGET=desk-control

OBJS=\
	control_thread.o \

CXXFLAGS=-std=c++14
CXXFLAGS+=-g -O0

LDLIBS+=\
	-lpthread \
	-lstdc++ \
	-lusb-1.0 \

all: $(TARGET)

desk-control: desk-control.o $(OBJS)

.phony: clean
clean:
	rm -f $(TARGET) *.o
