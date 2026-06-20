# LANRADIO v0.1

Ethernet internet radio receiver for ESP32-S3, built with ESP-IDF and ESP-ADF.
It receives an MP3 Icecast/Shoutcast stream through Ethernet, decodes it to PCM
and sends it to an external I2S DAC such as PCM5102A or UDA1334A.

## Hardware

The exact board and PHY have not been identified yet, so pin mappings are
deliberately centralized in `main/lanradio_config.h`.

* Ethernet controller: default configuration is W5500 over SPI. Change the
  `LANRADIO_W5500_*` pins for the actual board. A LAN8720 board needs a small
  alternative RMII driver, because ESP32-S3 itself has no integrated EMAC.
* DAC: PCM5102A or UDA1334A connected by I2S (BCLK, LRCLK/WS, DOUT and GND).
  Neither DAC needs MCLK.
* UART: 115200 baud, commands in a terminal or `idf.py monitor`.

> Do not power a 3.3 V ESP32-S3 I/O pin from a 5 V I2S module. Use a DAC that
> accepts 3.3 V logic, and share ground.

## Build

Install ESP-IDF 5.x and ESP-ADF, then set both environments. From this folder:

```powershell
$env:ADF_PATH = 'C:\esp\esp-adf'
idf.py set-target esp32s3
idf.py build flash monitor
```

If ESP-IDF reports that the W5500 driver is disabled, run `idf.py menuconfig`
and enable `Component config → Ethernet → Support SPI Ethernet modules → W5500`.

The project intentionally fails at CMake configuration when `ADF_PATH` is not
set, because the MP3 decoder and streaming pipeline come from ESP-ADF.

## UART commands

```
help             Show commands
list             List the built-in stations
play <1..10>     Start a station
stop             Stop playback
vol <0..100>     Set volume
info             Show network and player state
```

The built-in station list is only a starter list. Store user stations in NVS
and add a web UI after the physical Ethernet and audio path are verified.

## First bring-up

1. Set the actual Ethernet and I2S pins in `main/lanradio_config.h`.
2. Flash the firmware and wait for `ETH_EVENT_CONNECTED` and an IPv4 address.
3. Run `play 1`; the pipeline will reconnect after recoverable stream errors.
4. If there is no audio, verify BCLK/LRCLK/DOUT with a logic analyzer before
   changing decoder settings.
