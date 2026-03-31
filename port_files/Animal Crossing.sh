#!/bin/bash

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt


[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"

get_controls

GAMEDIR=/$directory/ports/ac-gc
CONFDIR="$GAMEDIR/conf/"

mkdir -p "$GAMEDIR/conf"

cd $GAMEDIR

> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

export XDG_DATA_HOME="$CONFDIR"
export LD_LIBRARY_PATH="/usr/lib32:$GAMEDIR/libs.${DEVICE_ARCH}:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
chmod +x ./AnimalCrossing
chmod +x ./AnimalCrossing.arm64

$GPTOKEYB "AnimalCrossing" &
pm_platform_helper "$GAMEDIR/AnimalCrossing"

if [ "$DEVICE_HAS_ARMHF" != "Y" ]; then
    ./AnimalCrossing.arm64
    echo "TRIMUI DEVICE"
elif [ "$CFW_NAME" == "muOS" ]; then
    export PORT_32BIT="Y"
    LD_LIBRARY_PATH=/usr/lib32 LD_PRELOAD=/usr/lib32/libSDL2.so ./AnimalCrossing 
    echo "Running muOS"
else
    export PORT_32BIT="Y"
    ./AnimalCrossing
    echo "ARMHF and not muOS!"
fi

pm_finish
