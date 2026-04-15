#!/usr/bin/env bash
# release.sh — build, strip, and upload a test binary to Discord
#
# Usage:
#   DISCORD_WEBHOOK=https://discord.com/api/webhooks/... ./release.sh
#
# Or export it in your shell profile so you don't have to type it each time.

set -euo pipefail

BINARY_NAME="AnimalCrossing"
BUILD_DIR="$(dirname "$0")/build"
BIN_DIR="$BUILD_DIR/bin"
BINARY="$BIN_DIR/$BINARY_NAME"
STRIPPED="$BIN_DIR/${BINARY_NAME}_stripped"
RELEASE_ZIP="$BIN_DIR/release.zip"

# ── Webhook ────────────────────────────────────────────────────────────────────
if [[ -z "${DISCORD_WEBHOOK:-}" ]]; then
    echo "Error: DISCORD_WEBHOOK is not set."
    echo "  export DISCORD_WEBHOOK=https://discord.com/api/webhooks/<id>/<token>"
    exit 1
fi

# ── Build ──────────────────────────────────────────────────────────────────────
echo ">>> Building..."
cmake -S "$(dirname "$0")/pc" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -Wno-dev -q
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

if [[ ! -f "$BINARY" ]]; then
    echo "Error: build succeeded but binary not found at $BINARY"
    exit 1
fi

# ── Strip ──────────────────────────────────────────────────────────────────────
echo ">>> Stripping..."
cp "$BINARY" "$STRIPPED"
strip "$STRIPPED"

ORIG_SIZE=$(du -sh "$BINARY"  | cut -f1)
NEW_SIZE=$(du -sh  "$STRIPPED" | cut -f1)
echo "    $ORIG_SIZE → $NEW_SIZE"

# ── Package ────────────────────────────────────────────────────────────────────
echo ">>> Packaging..."
rm -f "$RELEASE_ZIP"
(cd "$BIN_DIR" && zip -q "$RELEASE_ZIP" "${BINARY_NAME}_stripped" shaders/*)

ZIP_SIZE=$(du -sh "$RELEASE_ZIP" | cut -f1)
echo "    release.zip: $ZIP_SIZE"

# Discord webhooks cap file uploads at 25 MB
MAX_BYTES=$((25 * 1024 * 1024))
ACTUAL_BYTES=$(stat -c%s "$RELEASE_ZIP")
if (( ACTUAL_BYTES > MAX_BYTES )); then
    echo "Error: release.zip is $(du -sh "$RELEASE_ZIP" | cut -f1) — exceeds Discord's 25 MB limit."
    echo "Consider stripping more aggressively or compressing assets separately."
    exit 1
fi

# ── Upload ─────────────────────────────────────────────────────────────────────
# Get short git description for the message
GIT_DESC=$(git -C "$(dirname "$0")" describe --tags --always --dirty 2>/dev/null || echo "unknown")

echo ">>> Uploading to Discord ($GIT_DESC)..."
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
    -F "payload_json={\"content\":\"**Test build** \`$GIT_DESC\` — strip size $NEW_SIZE\"}" \
    -F "file=@$RELEASE_ZIP;filename=AnimalCrossing_${GIT_DESC}.zip" \
    "$DISCORD_WEBHOOK")

if [[ "$HTTP_CODE" == "200" || "$HTTP_CODE" == "204" ]]; then
    echo ">>> Done! Build uploaded to Discord."
else
    echo "Error: Discord returned HTTP $HTTP_CODE"
    exit 1
fi
