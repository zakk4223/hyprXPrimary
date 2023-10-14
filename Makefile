PLUGIN_NAME=hyprXPrimary
INSTALL_LOCATION=${HOME}/.local/share/hyprload/plugins/bin

all: check_build $(PLUGIN_NAME).so

install: all
	mkdir -p ${INSTALL_LOCATION}
	cp $(PLUGIN_NAME).so ${INSTALL_LOCATION}

check_build:
	g++ -shared -Wall -fPIC --no-gnu-unique main.cpp -o $(PLUGIN_NAME).so -g  -DWLR_USE_UNSTABLE `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++23

clean:
	rm -f ./$(PLUGIN_NAME).so
