[![EN](https://user-images.githubusercontent.com/9499881/33184537-7be87e86-d096-11e7-89bb-f3286f752bc6.png)](https://github.com/r57zone/OpenVR-ArduinoHMD/blob/master/README.md) 
[![RU](https://user-images.githubusercontent.com/9499881/27683795-5b0fbac6-5cd8-11e7-929c-057833e01fb1.png)](https://github.com/r57zone/OpenVR-ArduinoHMD/blob/master/README.RU.md) 
← Choose language | Выберите язык

# OpenVR Arduino HMD
Driver for OpenVR / SteamVR, which allows to track the head, using any Arduino rotation tracker, for a DIY VR HMD from [HDMI display](http://alii.pub/65dbii) and [headset](http://alii.pub/65dct6).<br>
<br>![](https://user-images.githubusercontent.com/9499881/126820737-5a8f3c5b-f723-4184-94d8-5031f52f2270.gif)

## Setup
1. Connect the second VR display and set the extended mode in the monitor settings.
2. Install [SteamVR](https://store.steampowered.com/app/250820/SteamVR/).
3. Unpack the [driver](https://github.com/r57zone/OpenVR-ArduinoHMD/releases) to a folder "...\Steam\steamapps\common\SteamVR\drivers". Configure the necessary parameters (`windowX`, `windowY`, `windowWidth`, `windowHeight` and others), in the configuration file "...\Steam\steamapps\common\SteamVR\drivers\arduinohmd\resources\settings\default.vrsettings". 
4. Launch SteamVR and in the SteamVR status window click -> Room Settings -> Small Room -> Calibration -> 170 cm. The SteamVR demo can be closed, and the launch of SteamVR Home can be disabled in the SteamVR settings.
5. Put on the HMD, if necessary, centering them with key (by default, `Numpad 5` or `CTRL + ALT + R`, can change the key in the configuration file by typing [the desired key name](https://github.com/r57zone/DualShock4-emulator/blob/master/BINDINGS.md)).

If you are using an Android smartphone, then it will probably be easier to use [OpenTrack driver](https://github.com/r57zone/OpenVR-OpenTrack), there you can also read instructions about streaming images to your smartphone.



You can quick turn on and turn off the extended VR monitor of the HMD using the `HMD Assistant` utility or [MultiMonitorTool](https://www.nirsoft.net/utils/multi_monitor_tool.html).

## Linux display-only build (experimental)

The Linux x86-64 build supports a fixed view with `ArduinoRequire: false`.
It exposes the existing display geometry and lens-distortion functions to SteamVR,
but does not read the Arduino, track the head, or implement the Windows global
hotkeys or HMD Assistant. Requiring an Arduino fails with a driver-log message.
The Visual Studio project remains the Windows build path.

Install CMake 3.16 or newer and a C++ compiler, then run from this source tree:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local/share/openvr-drivers/arduinohmd"
```

This creates `bin/linux64/driver_arduinohmd.so`, a driver manifest, and
`resources/settings/default.vrsettings` beneath the chosen prefix. It does not
register the driver or modify SteamVR's installation. Reinstalling replaces this
settings file; keep a copy of any calibration changes.

For an opt-in SteamVR experiment, close SteamVR, then use its `bin/linux64/vrpathreg`
program to register the **absolute path to the installed driver directory**:

```sh
# Run these commands from your actual SteamVR installation directory.
./bin/linux64/vrpathreg adddriver "$HOME/.local/share/openvr-drivers/arduinohmd"
# To undo registration, close SteamVR and run:
./bin/linux64/vrpathreg removedriver "$HOME/.local/share/openvr-drivers/arduinohmd"
```

The supplied Linux settings are a diagnostic example: 1920x1080 output, 960x1080
per eye, no distortion, unit zoom, and `DebugMode: true` (the existing 30 FPS debug
mode). They are **not a calibrated headset profile**. Set the panel bounds, per-eye
render resolution and optical parameters for the actual device before viewing
through lenses. Disable debug mode when measuring normal presentation cadence.
Lens correction must be applied once, by the component presenting the image.

The test loads the actual shared module in a fake OpenVR host. It checks interface
negotiation, required/missing-tracker refusal, fixed pose, eye viewports, radial
distortion and repeated activation/cleanup. It does not run SteamVR, scan out an
HDMI image, test Wayland/X11 compatibility, or establish optical comfort or latency.
This driver is an OpenVR plugin, not a KWin output plugin or a desktop-capture app.

The build approach follows the Linux work in
[OpenVR-driver-for-DIY #2](https://github.com/r57zone/OpenVR-driver-for-DIY/pull/2)
by Ralf Habacker. Head tracking is a separate next step.

## Arduino & rotation trackers
To track the turns of the head, you will need to buy [Arduino Nano](http://ali.pub/2oy73f) and connect a board with rotation sensors to it, for example, [MPU 6050 GY-521](http://ali.pub/2oy76c), MPU 9250, MPU 3200 GY-85 or any other firmware with output of three float values yaw, pitch, roll or 4 float values of quaternion w, x, y, z and calibration. The data output is binary (3 float or 4 float values), an example can be viewed [here](https://github.com/TrueOpenVR/TrueOpenVR-DIY/blob/master/HMD/Arduino/Arduino.Output.Bin.ino). The output speed should be `115200`.

There is a ready-made Arduino firmware for the [MPU 3200 GY-85](http://alli.pub/5wxnyl), it is called [Razor AHRS](https://github.com/Razor-AHRS/razor-9dof-ahrs/tree/master/Arduino). Along with it comes a program for calibration and demonstration. After calibration, replace the file "Output.ino", in the firmware folder, with [this](https://github.com/TrueOpenVR/TrueOpenVR-DIY/blob/master/HMD/Arduino/Razor_AHRS/Output.ino).
It is important to note here that there are new revisions of GY-85 that are incompatible with this firmware. The following sensors are supported by the firmware: the ADXL345 accelerometer, the ITG-3200 gyroscope and the HMC5843, HMC5883L magnetometers. The calibration instructions can be found on [youtube](https://www.youtube.com/watch?v=J7K_TnzQBZk).

![](https://user-images.githubusercontent.com/9499881/52521767-bd593480-2c95-11e9-923a-648a3018d131.png)

There is a ready-made Arduino firmware for [MPU 6050 GY-521](http://ali.pub/2oy76c). It is necessary to solder according to the scheme, [download libraries](https://github.com/r57zone/X360Advance/releases/download/1.0/Arduino.Firmware.MPU6050.X360Advance.zip), unpack them into the "libraries" folder of the Arduino IDE. Put the tracker on a flat surface, flash the sketch "MPU6050_calibration.ino" and get the data for calibration. Next, you need to flash the sketch ["HMD_MPU6050_DMP6.ino"](https://github.com/TrueOpenVR/TrueOpenVR-DIY/blob/master/HMD/Arduino/HMD_MPU6050_DMP6.ino), by entering the calibration data already received into it.

![](https://user-images.githubusercontent.com/9499881/52521728-e200dc80-2c94-11e9-9628-68ea3ef3dacd.png)

## Configuration file options
Name | Description
------------ | -------------
COMPort | The number of the Arduino COM port can be found in the Devices Manager. Use ports from `1` to `9`, change it in the device properties if necessary.
CenteringKey | The code of the picture centering key, you can change the key in the configuration file by typing [the desired name code](https://github.com/r57zone/DualShock4-emulator/blob/master/BINDINGS.md)).
CrouchPressKey | The code of the crouch key, you can change the key in the configuration file by typing [the desired name code](https://github.com/r57zone/DualShock4-emulator/blob/master/BINDINGS.md)). It is necessary for communication with other drivers, for example, using Razer Hydra controllers and using [this driver](https://github.com/r57zone/Razer-Hydra-SteamVR-driver) you can crouch.
CrouchOffset | The height of the crouch at the press of a button.
FOV | Degree of field of view. You can zoom in, depending on the VR headset lenses.
IPD | Interpupillary distance.
DistanceBetweenEyes | The distance between stereo images, the larger the closer.
DistortionK1, DistortionK2 | Lens distortion factors.
ScreenOffsetX | Horizontal image shift.
ZoomHeight, ZoomWidth | Scaling factors of stereo images.
DisplayFrequency | Screen refresh rate.
RenderWidth, RenderHeight | Image rendering resolution for one eye.
WindowWidth, WindowHeight | Height and width of the displayed window.
WindowX, WindowY | Window offset is required for display on other monitors. For example, to display on the second display, which is displayed on the right, you need to specify the value 1920 (provided that the first display has a resolution of 1920 by 1080). The exact data can be viewed using the [MultiMonitorTool utility](https://www.nirsoft.net/utils/multi_monitor_tool.html), and also with it you can turn off and turn on the second monitor via a bat file.
DebugMode | Debug mode, locked at 30 FPS. After checking, it is recommended to set it to `false` (disable).
ArduinoRequire | Requires a connected Arduino to start the driver. The parameter is necessary for quick tests of controllers, so as not to connect and put on a hmd for tests. To disable, change to `false`.

## Hotkeys
Name | Description
------------ | -------------
Numpad 5, CTRL + ALT + R | Centering the picture.
Page Up, Page Down | Rise or decline.
Numpad 8, 2, 4, 6 | Move forward, backward, left, right.
Numpad 1, 3 | Change yaw.
Numpad 7, 9 | Change roll.
Numpad - | Resetting movements and lifting.

## Known Issues
1. Red screen. You can fix this by selecting the "Headset Window" window.

## Download
>Version for x86 и x64.<br>
**[Download](https://github.com/r57zone/OpenVR-ArduinoHMD/releases)**

## Feedback
`r57zone[at]gmail.com`
