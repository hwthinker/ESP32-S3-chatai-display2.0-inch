```bash
esptool --chip esp32s3 -p COM8 erase_flash
esptool --chip esp32s3 -p COM8 -b 921600 write_flash 0x0 merged-binary.bin
```

atau sekali perintah
```bash
esptool --chip esp32s3 -p COM8 -b 921600 erase_flash write_flash 0x0 merged-binary.bin
```

> [!NOTE]
>
> Ganti `COM8` dengan port COM yang sesuai dengan perangkat di komputer Anda.
