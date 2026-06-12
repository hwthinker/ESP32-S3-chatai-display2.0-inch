#!/bin/bash
# Build script - compiles all Arduino projects and merges binaries
BOOT_APP0="/c/Users/hardware/AppData/Local/Arduino15/packages/esp32/hardware/esp32/3.3.10/tools/partitions/boot_app0.bin"
LOG="firmware/build-log.txt"
mkdir -p firmware
echo "=== Build log started $(date) ===" > "$LOG"

# Projects to skip (already built successfully)
SKIP="00-esp32S3-WS2812BTest"

for DIR in Source-code/*/; do
    NAME=$(basename "$DIR")
    [ "$NAME" = "$SKIP" ] && continue
    INO="$DIR$NAME.ino"
    if [ ! -f "$INO" ]; then
        echo "SKIP $NAME (no .ino found)" | tee -a "$LOG"
        continue
    fi

    echo ""
    echo "=== Compiling $NAME ==="
    BUILD="${DIR}build"
    rm -rf "$BUILD"
    mkdir -p "$BUILD"

    if ! arduino-cli compile --fqbn esp32:esp32:esp32s3 "$DIR" --output-dir "$BUILD" > "$BUILD/compile.log" 2>&1; then
        echo "FAILED to compile $NAME" | tee -a "$LOG"
        echo "--- Error from $NAME ---" >> "$LOG"
        tail -40 "$BUILD/compile.log" >> "$LOG"
        echo "" >> "$LOG"
        continue
    fi
    echo "Compile OK: $NAME"

    BOOTLOADER="$BUILD/$NAME.ino.bootloader.bin"
    PARTITIONS="$BUILD/$NAME.ino.partitions.bin"
    APPBIN="$BUILD/$NAME.ino.bin"
    OUTBIN="firmware/$NAME.bin"

    if ! esptool --chip esp32s3 merge-bin -o "$OUTBIN" --flash-mode dio --flash-freq 80m --flash-size 16MB \
        0x0 "$BOOTLOADER" 0x8000 "$PARTITIONS" 0xe000 "$BOOT_APP0" 0x10000 "$APPBIN" > "$BUILD/merge.log" 2>&1; then
        echo "FAILED to merge $NAME" | tee -a "$LOG"
        cat "$BUILD/merge.log" >> "$LOG"
        continue
    fi
    echo "Merged OK: $OUTBIN"
done

echo ""
echo "=== All done. Firmware: ==="
ls -lh firmware/*.bin 2>/dev/null
