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

install: $(TARGET)
	echo "installing into $(DESTDIR)..."
	install -d $(DESTDIR)
	install -d $(DESTDIR)/usr/bin
	install desk-control $(DESTDIR)/usr/bin
	install desk-control-speech.sh $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/desk-control
	install conf/* $(DESTDIR)/usr/share/desk-control/
	install -d $(DESTDIR)/etc/systemd/system/multi-user.target.wants
	install systemd/* $(DESTDIR)/etc/systemd/system/multi-user.target.wants

desk-control: desk-control.o $(OBJS)

.phony: clean
clean:
	rm -f $(TARGET) *.o
