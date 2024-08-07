[![EN](https://user-images.githubusercontent.com/9499881/33184537-7be87e86-d096-11e7-89bb-f3286f752bc6.png)](https://github.com/r57zone/OpenVR-ArduinoHMD/blob/master/README.md) 
[![RU](https://user-images.githubusercontent.com/9499881/27683795-5b0fbac6-5cd8-11e7-929c-057833e01fb1.png)](https://github.com/r57zone/OpenVR-ArduinoHMD/blob/master/README.RU.md) 
# OpenVR Arduino HMD
Драйвер для OpenVR / SteamVR, позволяющий отслеживать голову, с помощью любого Arduino трекера вращения, для самодельного VR шлема из [HDMI дисплея](http://alii.pub/65dbii) и [гарнитуры](http://alii.pub/65dct6).<br>
<br>![](https://user-images.githubusercontent.com/9499881/126820737-5a8f3c5b-f723-4184-94d8-5031f52f2270.gif)

## Настройка
1. Подключите второй VR дисплей и установите расширенный режим в настройках мониторов.
2. Установите [SteamVR](https://store.steampowered.com/app/250820/SteamVR/).
3. Распакуйте [драйвер](https://github.com/r57zone/OpenVR-ArduinoHMD/releases) в папку "...\Steam\steamapps\common\SteamVR\drivers". Настройте необходимые параметры (`windowX`, `windowY`, `windowWidth`, `windowHeight` и другие), в файле конфигурации "...\Steam\steamapps\common\SteamVR\drivers\arduinohmd\resources\settings\default.vrsettings". 
4. Запустите SteamVR и окне статуса SteamVR нажать -> Настройки комнаты -> Маленькая комната -> Калибровка -> 170 см. SteamVR демонстрация может быть закрыта, а запуск SteamVR Home может быть отключен в настройках SteamVR.
5. Наденьте шлем, при необходимости отцентрируйте его, с помощью клавиши (по умолчанию `Numpad 5` или `CTRL + ALT + R`, изменить кнопку можно в файле конфигурации, вписав [нужное название кнопки](https://github.com/r57zone/DualShock4-emulator/blob/master/BINDINGS.RU.md)).

Если вы используете Android смартфон, то вероятно будет удобнее использовать [OpenTrack драйвер](https://github.com/r57zone/OpenVR-OpenTrack), там же можно прочитать инструкции о стриминге изображения на смартфон.



Быстро включать и выключать расширенный VR монитор шлема можно, с помощью оболочки `HMD Assistant` или [MultiMonitorTool](https://www.nirsoft.net/utils/multi_monitor_tool.html).

## Arduino и трекеры вращения
Подойдут любые COM трекеры с выводом 3 float значения yaw, pitch, roll или 4 float значения кватерниона w, x, y, z. Скорость вывода должна составлять.

Для отслеживания поворотов головы понадобится купить [Arduino Nano](http://ali.pub/2oy73f) и подключить к ней плату с датчиками вращения, например, [MPU 6050 GY-521](http://ali.pub/2oy76c), MPU 9250, MPU 3200 GY-85 или любую другую при наличии прошивки с выводом трёх float значений yaw, pitch, roll (рысканье, тангаж, крен) или 4 float значения кватерниона w, x, y, z и калибровкой. Вывод данных происходит бинарный (3 float или 4 float значения), пример можно посмотреть [тут](https://github.com/TrueOpenVR/TrueOpenVR-DIY/blob/master/HMD/Arduino/Arduino.Output.Bin.ino). При использовании кватерниона измените значение `RotationQuaternion` на `true`. Скорость вывода должна составлять `115200`.

Готовая прошивка Arduino есть для [MPU 3200 GY-85](http://alli.pub/5wxnyl), называется она [Razor AHRS](https://github.com/Razor-AHRS/razor-9dof-ahrs/tree/master/Arduino). Вместе с ней идет программа для калибровки и демонстрации. После калибровки замените файл "Output.ino", в папке с прошивкой, на [этот](https://github.com/TrueOpenVR/TrueOpenVR-DIY/blob/master/HMD/Arduino/Razor_AHRS/Output.ino).
Здесь важно отметить, что появились новые ревизии GY-85, которые несовместимы с этой прошивкой. Прошивкой поддерживаются следующие сенсоры: акселерометр ADXL345, гироскоп ITG-3200 и магнитометры HMC5843, HMC5883L. Инструкцию по калибровке можно найти на [youtube](https://www.youtube.com/watch?v=J7K_TnzQBZk).

![](https://user-images.githubusercontent.com/9499881/52521767-bd593480-2c95-11e9-923a-648a3018d131.png)

Готовая прошивка Arduino есть для [MPU 6050 GY-521](http://ali.pub/2oy76c). Необходимо спаять по схеме, [загрузить библиотеки](https://github.com/r57zone/X360Advance/releases/download/1.0/Arduino.Firmware.MPU6050.X360Advance.zip), распаковать их в папку "libraries" Arduino IDE. Положить трекер на ровную поверхость, прошить скетч "MPU6050_calibration.ino" и получить данные для калибровки. Далее нужно прошить скетч ["HMD_MPU6050_DMP6.ino"](https://github.com/TrueOpenVR/TrueOpenVR-DIY/blob/master/HMD/Arduino/HMD_MPU6050_DMP6.ino), вписав в него уже полученные данные калибровки.

![](https://user-images.githubusercontent.com/9499881/52521728-e200dc80-2c94-11e9-9628-68ea3ef3dacd.png)

## Параметры файла конфигурации
Название | Описание
------------ | -------------
COMPort | Номер COM порта Arduino, можно посмотреть в диспетчере устройств. Используйте порты от `1` до `9`, измените при необходимости его в свойствах устройства.
CenteringKey | Код кнопки центрирования изображения, изменить кнопку можно в файле конфигурации, вписав [нужное название кнопки](https://github.com/r57zone/DualShock4-emulator/blob/master/BINDINGS.RU.md)).
CrouchPressKey | Код кнопки приседания, изменить кнопку можно в файле конфигурации, вписав [нужное название кнопки](https://github.com/r57zone/DualShock4-emulator/blob/master/BINDINGS.RU.md)). Необходимо для связи с другими драйверами, например, используя контроллеры Razer Hydra и используя [этот драйвер](https://github.com/r57zone/Razer-Hydra-SteamVR-driver) можно приседать.
CrouchOffset | Высота приседания по нажатию кнопки.
FOV | Градус поля зрения. Можно увеличить, в зависимости от линз VR гарнитуры.
IPD | Межзрачковое расстояние.
DistanceBetweenEyes | Расстояние между стерео изображениями, чем больше, тем ближе.
DistortionK1, DistortionK2 | Коэффициенты искажения линз.
ScreenOffsetX | Сдвиг изображения по горизонтали.
ZoomHeight, ZoomWidth | Коэффициенты масштабирования стерео изображений.
DisplayFrequency | Частота обновления экрана.
RenderWidth, RenderHeight | Разрешение рендера изображения для одного глаза.
WindowWidth, WindowHeight | Высота и ширина выводимого окна.
WindowX, WindowY | Смещение окна, требуется для отображения на других мониторах (расширенных). Например, для отображения на втором дисплее, который отображается справа, нужно указать значение 1920 (при условии, что первый дисплей имеет разрешение 1920 на 1080). Точные данные можно просмотреть, с помощью [MultiMonitorTool утилиты](https://www.nirsoft.net/utils/multi_monitor_tool.html), которая также может выключать и включить второй монитор, через bat-файл.
DebugMode | Режим отладки, заблокирован на 30 FPS. Рекомендуется после проверки отключить (поставить `false`).
ArduinoRequire | Требование подключенного Arduino, для запуска драйвера. Параметр необходим для быстрых тестов контроллеров, чтобы не подключать и одевать шлем для тестов. Для отключения измените на `false`.

## Горячие клавиши
Название | Описание
------------ | -------------
Numpad 5, CTRL + ALT + R | Центрирование изображения.
Page Up, Page Down | Подняться или опуститься.
Numpad 8, 2, 4, 6 | Перемещение вперед, назад, влево, вправо. 
Numpad 1, 3 | Изменить yaw (рысканье).
Numpad 7, 9 | Изменить roll (крен).
Numpad - | Сброс перемещений и поднятия.

## Известные проблемы
1. Красный экран. Исправить это можно выбрав окно "Headset Window".

## Загрузка
>Версия для x86 и x64.<br>
**[Загрузить](https://github.com/r57zone/OpenVR-ArduinoHMD/releases)**

## Обратная связь
`r57zone[собака]gmail.com`