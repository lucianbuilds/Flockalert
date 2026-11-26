# ESP32 GPS Coordinate Alert

A portable ESP32 GPS proximity-alert device. It reads position and speed from a u-blox GPS receiver, compares the current position with a bundled coordinate database, and shows an alert on a 128 x 64 OLED display when a stored location is nearby.

The device also works as a compact GPS speedometer and has a built-in Wi-Fi configuration page, so common settings can be changed without rebuilding the firmware.

> Use this project responsibly and in accordance with local laws, road-safety rules, and privacy expectations. It is an informational aid, not a substitute for attentive driving.

<!-- Add your build photo at docs/images/device-build.jpg, then remove this comment:
![Completed ESP32 GPS Coordinate Alert device](docs/images/device-build.jpg)
-->

## Features

- Live GPS speed in MPH or km/h
- Nearest stored coordinate and current latitude/longitude
- Visual proximity alert when within the configured alert distance
- Separate persistent `ALPRs passed` and `Speed cams passed` counters stored in ESP32 non-volatile storage
- Wi-Fi configuration portal started by holding the BOOT button
- Configurable GPS, OLED, and speaker GPIO assignments
- Configurable GPS baud rate, satellite requirement, alert distances, and display options
- Large coordinate database kept separate from the main sketch

## Hardware

| Part | Default connection | Notes |
| --- | --- | --- |
| ESP32 DevKit | - | Standard ESP32 development board |
| u-blox GPS TX | GPIO 23 | Connects to the ESP32 GPS receive pin |
| u-blox GPS RX | Not connected | Default firmware setting is `-1` (unused) |
| SSD1306 OLED SDA | GPIO 21 | I2C data |
| SSD1306 OLED SCL | GPIO 22 | I2C clock |
| Passive piezo buzzer (optional) | GPIO 25 | Plays a chime when an ALPR or speed camera is nearby; see [Buzzer Chimes](#buzzer-chimes) |
| BOOT button | GPIO 0 | Hold for 3 seconds to open configuration mode |

Power the GPS and OLED from the voltage appropriate for your specific modules. Make sure all modules share a common ground.

<!-- Add a clear wiring photo at docs/images/wiring.jpg, then remove this comment:
![ESP32, GPS, and OLED wiring](docs/images/wiring.jpg)
-->

## Project Layout

```text
Flockalert/
├── README.md
├── flockalert/
│   └── esp32/
│       ├── esp32.ino        Main firmware: GPS, display, and proximity checks
│       ├── coordinates.h    Stored coordinate database
│       ├── buzzer.h         Chime sequences and buzzer playback
│       ├── config.h         Factory/default settings
│       └── configportal.h   Wi-Fi portal, saved settings, and counter storage
└── docs/
    └── images/              Optional README photos
```

## Before You Upload

Install the Arduino IDE, then add ESP32 board support through the Board Manager. Select your ESP32 board, usually **ESP32 Dev Module** for a standard DevKit.

Install these libraries through the Arduino Library Manager:

- `TinyGPSPlus`
- `Adafruit GFX Library`
- `Adafruit SSD1306`

Open [esp32.ino](flockalert/esp32/esp32.ino) in the Arduino IDE and upload it to the board. The three header files in the same folder are included automatically by the sketch.

If your board needs BOOT held while uploading, hold it only until the upload begins, then release it.

## Normal Operation

At startup, the OLED shows the number of loaded coordinate entries and the saved counter. Once the GPS has a usable position, the normal display shows:

```text
      37.4
       MPH

Nearest: 218ft
37.80451,-122.27123
```

- **Speed** comes from the GPS receiver and is shown in miles per hour.
- **Nearest** is the straight-line distance to the closest entry in `coordinates.h`. It is shown in feet up to 2000 ft and in miles beyond that, or in meters and kilometers if **Distance units** is set to Meters / km.
- **Coordinates** are the current GPS latitude and longitude.
- If a minimum satellite count is configured, the device waits until that requirement is met before using a GPS position.

## Proximity Alerts

Each time the GPS position updates, the firmware compares it with every stored coordinate using `TinyGPSPlus::distanceBetween()`.

When the closest coordinate is inside the alert radius, and **Switch to full-screen alert** is enabled, the normal screen is replaced by an alert screen:

```text
       NEAR

    Flock Safety ALPR

      32.7 MPH
```

With **Switch to full-screen alert** turned off, the speed screen stays visible and the bottom line becomes an inverted bar such as `ALPR NEAR 120ft`, or `SPEED CAM 120ft` for a speed camera. Speed stays readable at all times.

The firmware remembers the active coordinate while you are in the alert area. It increments `ALPRs passed` (or `Speed cams passed` for entries whose name starts with `Speed Cam`) when entering an alert, and can increment again only if a different closest coordinate becomes active. The alert clears after the nearest coordinate is farther away than the configured clear distance.

The counter is stored in ESP32 non-volatile storage, so it remains after power-off or restart. It can be reset from the configuration page.

## Buzzer Chimes

Turn on **Play a chime when an ALPR or speed camera is nearby** on the Wi-Fi configuration page to get an audible alert. Pick one of four tones under **Chime tone**:

| Tone | Sound |
| --- | --- |
| Two-tone | Two descending notes |
| Rising | Four rising notes |
| Warble | Alternating high and low notes |
| Quick beep | Two short high beeps |

The chime plays each time a new alert starts, or when a different ALPR becomes the closest one in range. It does not repeat while you stay in the same alert area.

Use a **passive** piezo buzzer, which needs a signal to make sound. Connect its positive lead to GPIO 25 and its negative lead to GND. An **active** buzzer makes one fixed tone, so it sounds the same for every tone choice.

## Wi-Fi Configuration

1. Power on the device.
2. Hold the ESP32 **BOOT** button for 3 seconds.
3. On a phone or computer, join the Wi-Fi network:

   | Setting | Value |
   | --- | --- |
   | Network name | `GPS-ALPR-Config` |
   | Password | Unique to each board (`gps-` + 6 characters), shown on the OLED |
   | Configuration address | `http://192.168.4.1` |

4. Open `http://192.168.4.1` in a browser.
5. Change the settings you need, then choose **Save & Restart**.

The device stays in configuration mode until it restarts. Saved settings are restored automatically on future boots.

<!-- Add a configuration-page screenshot at docs/images/config-page.png, then remove this comment:
![Wi-Fi configuration page](docs/images/config-page.png)
-->

### Available Settings

| Group | Settings |
| --- | --- |
| Pins | GPS RX, GPS TX, OLED SDA, OLED SCL, buzzer |
| GPS | Baud rate and minimum satellites |
| Alerts | Normal alert distance, alert clear distance, Highway Mode speed, and highway alert distance |
| Display | OLED refresh interval, coordinate decimals, speed text size, switch to full-screen alert, distance units (feet / miles or meters / km) |
| Buzzer | Enable chime, chime tone |
| Maintenance | Reset the ALPR or speed cam counter, or restore all defaults |

The portal rejects duplicate GPIO assignments among the GPS, OLED, and speaker connections. A GPS TX setting of `-1` leaves the ESP32 transmit pin unused.

## Adding or Updating Coordinates

Edit [coordinates.h](flockalert/esp32/coordinates.h). Each entry has a display name, latitude, and longitude:

```cpp
{ "Flock Safety ALPR", 37.782190, -122.219840 },
```

Add entries inside the `waypoints[]` list, separated with commas, then upload the sketch again. The firmware calculates `WAYPOINT_COUNT` automatically.

Keep coordinates in decimal degrees. Latitude is listed first, followed by longitude.

### Regenerating from OpenStreetMap

[cameras.overpassql](tools/overpass/cameras.overpassql) pulls ALPRs (the cameras DeFlock maps) and speed cameras from OpenStreetMap:

1. Paste the query into [Overpass Turbo](https://overpass-turbo.eu), move the map to your area, and click **Run**.
2. Save the result: **Export > raw data directly from Overpass API**, or copy the **Data** tab.
3. Write a new `coordinates.h` from it. This replaces the existing file:

```powershell
./tools/overpass/make-coordinates.ps1 -InputFile export.csv
```

Use `-FromClipboard` instead of `-InputFile` if you copied the Data tab. Red-light cameras can be turned on in the query.

### Uploading camera data over Wi-Fi

You can load new camera data without rebuilding the firmware. Open the configuration page and use **Upload Camera Data** to send the file saved from Overpass Turbo, unedited. It is stored on the ESP32's flash (LittleFS) and used instead of `coordinates.h` until you click **Use Built-in List**. Cameras are held in RAM, which fits roughly 15,000-20,000 entries. If a file is too large, the upload is rejected and the built-in list stays in use.

## Smarter Alerts

- **Direction-aware:** above 8 MPH, a camera only triggers an alert if it is ahead of you (within 60 degrees of your heading). Cameras mapped as facing a cross street are skipped. When you are slower or stopped, every camera counts.
- **Camera clusters:** cameras within 15 m (50 ft) of the last one counted are treated as the same site, so a busy intersection counts and chimes once.
- **Speed limits:** if a speed camera has a mapped limit, the alert bar shows it (`CAM 25MPH 350ft`) and the chime only plays when you are over it.
- **Separate chime:** speed cameras always play a triple beep, so you can tell them from ALPRs by ear.

The thresholds are in [config.h](flockalert/esp32/config.h) under `SMARTER ALERTS`.

## Default Settings

| Setting | Default |
| --- | --- |
| GPS receive GPIO | 23 |
| GPS transmit GPIO | Disabled (`-1`) |
| GPS baud rate | 9600 |
| OLED SDA / SCL | GPIO 21 / GPIO 22 |
| OLED I2C address | `0x3C` |
| Buzzer / speaker GPIO | 25 |
| Alert distance | 50 ft |
| Alert clear distance | 75 ft |
| Minimum satellites | 0 (no minimum) |
| Display refresh interval | 250 ms |
| Coordinate precision | 5 decimal places |
| Speed text size | 4 |
| Switch to full-screen alert | Off |
| Distance units | Feet / miles |
| Buzzer chime | On |
| Chime tone | Two-tone |

The factory defaults live in [config.h](flockalert/esp32/config.h). Settings changed through the Wi-Fi page override these defaults until the device is reset to defaults.

## Highway Mode

Highway Mode automatically activates when GPS speed is **above** the configured threshold. It uses the configured highway alert distance and adds `HWY` to the normal OLED display.

The default behavior is:

| Speed | Alert distance |
| --- | --- |
| 45 MPH or below | 50 ft |
| Above 45 MPH | 150 ft |

An alert that begins in Highway Mode stays active until the device has moved beyond the highway radius or the configured clear distance, whichever is greater. This prevents an alert from turning off immediately when the highway radius is larger than the normal clear distance.

## Troubleshooting

| Symptom | Things to check |
| --- | --- |
| OLED stays blank | Confirm SDA, SCL, power, ground, and the OLED I2C address (`0x3C` by default). |
| `Waiting for fix...` remains on screen | Move the GPS antenna outdoors with a clear view of the sky. Confirm GPS TX is connected to ESP32 GPIO 23 and that the baud rate matches the receiver. |
| Speed or position is wrong | Give the GPS time to acquire satellites; configure a minimum satellite value if you want the firmware to ignore weak fixes. |
| No proximity alerts | Verify the coordinate format, make sure the point is within the alert distance, and confirm that a valid GPS location is displayed. |
| Configuration network does not appear | Hold BOOT for the full 3 seconds. The OLED should show the Wi-Fi name and address. |
| Settings did not change | Save from the Wi-Fi page and wait for the ESP32 to restart. |

## Current Scope

This firmware is focused on GPS location, speed display, coordinate proximity alerts, persistent settings, and the standalone configuration portal. GPIO 25 drives the optional passive buzzer for chimes. MP3 and SD-card audio are not included.
