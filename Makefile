ifeq ($(CXX),g++)
    EXTRA_FLAGS = --no-gnu-unique
else
    EXTRA_FLAGS =
endif

all:
	$(CXX) -shared -fPIC $(EXTRA_FLAGS) main.cpp gollum.cpp -o libhyprgollum.so -g `pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon` -std=c++2b -O2 -Wno-unused-result
clean:
	rm ./libhyprgollum.so
