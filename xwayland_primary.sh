#!/bin/bash


PRIMARY_MONITOR=${1}


function setprimary {
	echo "SET PRIMARY TO ${PRIMARY_MONITOR}"
	xrandr --output $PRIMARY_MONITOR --primary
}

function handle {
	if [ ${1:0:14} == "xwaylandupdate"]; then
		setprimary
	fi

}

#always set it on start
setprimary
socat -u UNIX-CONNECT:/tmp/hypr/$(echo $HYPRLAND_INSTANCE_SIGNATURE)/.socket2.sock STDOUT | while read line; do handle $line; done

