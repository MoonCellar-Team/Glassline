# GameNetworkingSockets — заголовки

Публичные заголовки [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets)
1.6.0 (Valve, BSD-3 — `LICENSE`), без изменений: `include/steam/*.h`. Нужны слою сети
(`port/src/net`) только при сборке — сама библиотека загружается во время работы (`dlopen`), её
нет ни в сборке, ни в репозитории. Вызовы — плоский C API (`steamnetworkingsockets_flat.h`).
