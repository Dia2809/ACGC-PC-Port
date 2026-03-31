#!/bin/bash
# deploy.sh - Send the Animal Crossing binary and assets to the device via SSH
#
# Usage: ./deploy.sh

set -e

DEVICE="root@192.168.178.142"
REMOTE_DIR="/storage/games-external/roms/ports/ac-gc"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$SCRIPT_DIR/pc/build32/bin"

echo "=== Deploying to $DEVICE:$REMOTE_DIR ==="

# Create remote directory structure

# Send the binary
echo "Sending AnimalCrossing binary..."
scp "$BIN_DIR/AnimalCrossing" "$DEVICE:$REMOTE_DIR/AnimalCrossing"

# Send shaders if they exist
if [ -d "$BIN_DIR/shaders" ] && [ "$(ls -A "$BIN_DIR/shaders" 2>/dev/null)" ]; then
    echo "Sending shaders..."
    scp "$BIN_DIR/shaders/"* "$DEVICE:$REMOTE_DIR/shaders/"
fi

# Send the PortMaster launch script if it exists
if [ -f "$SCRIPT_DIR/port_files/Animal Crossing.sh" ]; then
    echo "Sending launch script..."
    scp "$SCRIPT_DIR/port_files/Animal Crossing.sh" "$DEVICE:$REMOTE_DIR/Animal Crossing.sh"
    ssh "$DEVICE" "chmod +x '$REMOTE_DIR/Animal Crossing.sh'"
fi

# Make binary executable
ssh "$DEVICE" "chmod +x '$REMOTE_DIR/AnimalCrossing'"

echo ""
echo "=== Deploy complete ==="
echo "Binary: $DEVICE:$REMOTE_DIR/AnimalCrossing"
