# StockWidget
Widget for displaying the exchange rate of cryptocurrencies on the desktop in real time.

![image](https://user-images.githubusercontent.com/123785508/230630082-d0c5a3b0-bf7c-459c-928e-faccf5047a45.png)

**List of supported cryptocurrency exchanges (API):**
- Binance Global
- Binance US *(available only from the USA)*
- Bybit
- Coinbase
- Kraken
- OKX
- Poloniex

*Data is refreshed every 60 seconds by default, the interval can be changed in the settings.*

## Installation

- Download latest version of the widget https://github.com/mxrcode/StockWidget/releases
- Unpack the **.zip** archive to any place convenient for you
- Run the app with **StockWidget.exe**

*An icon should appear in the tray by clicking on which a menu for interacting with the widget will appear. There you can configure it and add the program to autorun.*

## Usage

- **Pairs** : choose an exchange, search its pairs and add them to the widget. Each pair keeps its own exchange, so the widget can show BTC from Binance next to ETH-EUR from Kraken. Drag the pairs to change their order.
- **Appearance** : screen corner, text color, green / red price changes. The widget can also be dragged anywhere with the mouse (grab it by the text).
- **Other** : refresh interval (10 seconds to 10 minutes), always on top, error notifications.

The settings are stored in `config.cfg` next to the executable: one trading pair per line (`BTC-USDT`, or `kraken_com:ETH-EUR` for a pair from another exchange than `$data_sources`), options start with `$`. The log is written to `qInfo.log` in the same folder.

## Building

Requires Qt 6.5 or newer (developed with Qt 6.12, MinGW 64-bit, installed with the Qt Online Installer: the MinGW, CMake and Ninja tools are taken from `C:\Qt\Tools`).

**Release package** : run `deploy.cmd` (or `.\deploy.ps1 -QtDir C:\Qt\6.12.0\mingw_64` for another Qt kit). It builds the Release version and creates `build\StockWidget-<version>-win64.zip` with all the required DLLs, plus a `.sha256` file for the release notes. The version is taken from `project(VERSION)` in `CMakeLists.txt`.

**Manual build** :
```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build
windeployqt --release --no-translations build/StockWidget.exe
```

Or open `CMakeLists.txt` in Qt Creator.

# License
This software is released under the LGPL-3.0 License, see LICENSE.

The Roboto font (`resources/fonts`) is licensed under the Apache License 2.0, see `resources/fonts/LICENSE-Roboto.txt`.
