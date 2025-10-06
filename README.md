# mik32sd
*Драйвер карт SD для MIK32*

\[RU/[EN](README_EN.md)]

---
## Требуемые нестандартные модули
`mik32_transaction` (https://github.com/Barsy-Barsevich/mik32_transaction)
`mik32_stdio` (https://github.com/Barsy-Barsevich/mik32_stdio), только если методы `mik32sd_command.h` использованы

---
## Философия
Данный драйвер создавался для последующей реализации файловой системы на базе MIK32.
Данный драйвер:
- Поддерживает SDHC-карты информационной емкостью до 32ГБ;
- Реализует общение с картами SD через шину SPI;

---
## Заголовочные файлы

### ⚡️`mik32sd.h`
Содержит объявления основных методов драйвера.
##### `mik32_sd_init(MIK32SD_Descriptor_TypeDef *sd, MIK32SD_Config_TypeDef *cfg)`
Инициализация SD-карты и определение ее типа.
##### `mik32_sd_send_command(MIK32SD_Descriptor_TypeDef *sd, MIK32SD_CMD_TypeDef command, uint32_t operand, uint8_t crc, uint8_t* resp)`
Отправка команды на SD-карту.
##### `mik32_sd_single_read(MIK32SD_Descriptor_TypeDef *sd, uint32_t addr, uint8_t* buf)`
Чтение одного сектора SD-карты (512 байт).
##### `mik32_sd_single_write(MIK32SD_Descriptor_TypeDef *sd, uint32_t addr, uint8_t* buf)`
Запись одного сектора SD-карты (512 байт).
##### `mik32_sd_single_erase(MIK32SD_Descriptor_TypeDef *sd, uint32_t addr)`
Стирание одного сектора SD-карты (512 байт).

### ⚡️`mik32sd_spi.h`
Заголовок содержит объявление структуры-дескриптора модуля SPI для SD-карты и структуры-конфига для него; также содержит низовые методы для работы с SPI.

### ⚡️`mik32sd_param.h`
Заголовок содержит параметры драйвера.

### ⚡️`mik32sd_const.h`
Содержит объявления типов данных-констант (enums).
- `MIK32SD_CMD_TypeDef`;
- `MIK32SD_Type_TypeDef`;
- `MIK32SD_Voltage_TypeDef`;
- `MIK32SD_Tokens_TypeDef`;
- `MIK32SD_Status_TypeDef`.

### ⚡️`mik32sd_types.h`
Заголовок содержит объявления структур-дескрипторов и структур-конфигов, применяемых драйвером.

### ⚡️`mik32sd_diag.h`
Заголовок содержит объявления методов декодирования констант, возвращаемых методами драйвера.

### ⚡️`mik32sd_command.h`
Заголовок содержит объявления методов, которые могли бы быть применены в CLI-интерфейсе для отладки драйвера.


