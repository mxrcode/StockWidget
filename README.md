# StockWidget
Widget for displaying the exchange rate of cryptocurrencies on the desktop in real time.

![image](https://user-images.githubusercontent.com/123785508/230630082-d0c5a3b0-bf7c-459c-928e-faccf5047a45.png)

**List of supported cryptocurrency exchanges (API):**
- Binance Global
- Binance US *(available only from the USA)*
- Coinbase
- Kraken
- Poloniex

*Data is refreshed once every 60 seconds (15 seconds after a network error).*

## Installation

- Download latest version of the widget https://github.com/mxrcode/StockWidget/releases
- Unpack the **.zip** archive to any place convenient for you
- Run the app with **StockWidget.exe**

*An icon should appear in the tray by clicking on which a menu for interacting with the widget will appear. There you can configure it and add the program to autorun.*

The settings are stored in `config.cfg` next to the executable: one trading pair per line (`BTC-USDT`), options start with `$`. The log is written to `qInfo.log` in the same folder.

## Building

Requires Qt 6.5 or newer (developed with Qt 6.12, MinGW 64-bit) and CMake 3.16+.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build
windeployqt --release --no-translations build/StockWidget.exe
```

Or open `CMakeLists.txt` in Qt Creator.

# License
This software is released under the LGPL-3.0 License, see LICENSE.

The Roboto font (`resources/fonts`) is licensed under the Apache License 2.0, see `resources/fonts/LICENSE-Roboto.txt`.
