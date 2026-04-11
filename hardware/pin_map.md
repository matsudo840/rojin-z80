# 老人Z80 GPIOピンアサイン定義書

このドキュメントは、本プロジェクトにおける ESP32-S3 と Z80 の接続ピン配置の **Single Source of Truth (唯一の正解)** です。
他のドキュメント (`concept.md`, `circuit-diagram.md`) と記述が食い違う場合は、**本書の記述を優先** します。

## 1. 方針と制約

1.  **USBポートの制限**: GPIOピン不足解消のため、`USB D-` (GPIO 19) を使用します。
    - **重要**: 開発および電源供給には **UARTポート** (USB-UARTブリッジ側) を使用してください。Native USBポートは使用できません。
2.  **RGB LED**: GPIO 48 (および一部のボードでは GPIO 38) は基板上の RGB LED (NeoPixel) に接続されています。これらは Z80 の制御信号（/RD 等）と競合するため、**起動時にソフトウェアで明示的に消灯** する必要があります。
3.  **Strapping Pins**: GPIO 0, 3, 45, 46 はブートモード決定ピンです。
    - ESP32の起動完了まで Z80 をリセット状態（ハイインピーダンス）に保つことで、ブート阻害を防ぎます。
14. **プルアップ構成**: 以下の信号線には、ノイズ対策およびフローティング防止のため、5Vへのプルアップ抵抗（例: $10 \text{k}\Omega$）を実装しています。
    - アドレスバス (A0-A15)
    - データバス (D0-D7)
    - 制御信号: INT, NMI, MREQ, IORQ, RD, WR, BUSACK, WAIT, BUSRQ, RESET, M1 (HALT, RFSHを除く主要信号)

## 2. ピンアサイン一覧

### A. アドレスバス (Z80 输出 → ESP32 入力)
| 信号名 | Z80 Pin | ESP32 GPIO | 備考 |
|:---|:---:|:---:|:---|
| **A0** | 30 | **GPIO 1** | |
| **A1** | 31 | **GPIO 2** | |
| **A2** | 32 | **GPIO 3** | Strap Pin (JTAG) |
| **A3** | 33 | **GPIO 4** | |
| **A4** | 34 | **GPIO 5** | |
| **A5** | 35 | **GPIO 6** | |
| **A6** | 36 | **GPIO 7** | |
| **A7** | 37 | **GPIO 8** | |
| **A8** | 38 | **GPIO 9** | |
| **A9** | 39 | **GPIO 10** | |
| **A10** | 40 | **GPIO 11** | |
| **A11** | 1 | **GPIO 12** | |
| **A12** | 2 | **GPIO 13** | |
| **A13** | 3 | **GPIO 0** | (← 14 から変更) Boot Button / Strap Pin |
| **A14** | 4 | **(N/C)** | GPIO不足のため廃止 |
| **A15** | 5 | **(N/C)** | GPIO不足のため廃止 |

### B. データバス (Z80 ⇔ ESP32 双方向)
| 信号名 | Z80 Pin | ESP32 GPIO | 備考 |
|:---|:---:|:---:|:---|
| **D0** | 14 | **GPIO 17** | |
| **D1** | 15 | **GPIO 18** | |
| **D2** | 12 | **GPIO 21** | |
| **D3** | 8 | **GPIO 38** | RGB LEDと競合の可能性あり |
| **D4** | 7 | **GPIO 39** | JTAG |
| **D5** | 9 | **GPIO 40** | JTAG |
| **D6** | 10 | **GPIO 41** | JTAG |
| **D7** | 13 | **GPIO 42** | JTAG |

### C. 制御信号
| 信号名 | 方向 | ESP32 GPIO | 役割・備考 |
|:---|:---:|:---:|:---|
| **CLK** | ESP→Z80 | **GPIO 14** | (← 0 から変更) コンデンサ回避のためクリーンなピンへ移動 |
| **WAIT** | ESP→Z80 | **GPIO 45** | Strap (Output) |
| **RESET** | ESP→Z80 | **GPIO 46** | Strap (Output) |
| **MREQ** | Z80→ESP | **GPIO 47** | Input |
| **RD** | Z80→ESP | **GPIO 48** | Input / **オンボードNeoPixelと共用 (要消灯)** |
| **WR** | Z80→ESP | **GPIO 16** | Input (A15から転用) |
| **IORQ** | Z80→ESP | **GPIO 35** | Input |
| **DIR** | ESP→LVC | **GPIO 15** | (A14から転用) |
| **SDA** | ESP⇔OLED | **GPIO 36** | I2C Data |
| **SCL** | ESP→OLED | **GPIO 37** | I2C Clock |

---

## 3. レベルシフタ (74LVC245) 構成

- **LVC #1**: A0-A7 (Direction: High 固定 / Z80→ESP)
- **LVC #2**: A8-A13 (Direction: High 固定 / Z80→ESP)
- **LVC #3**: D0-D7 (Direction: **GPIO 15** で制御)
    - Low: ESP → Z80 (Write to Z80)
    - High: Z80 → ESP (Read from Z80)
- **LVC #4**: CLK, WAIT, RESET (Direction: Low 固定 / ESP→Z80)
    - **注意**: LVC245の配線方向を **A=Z80 (5V), B=ESP (3.3V)** とします。
- **LVC #5**: , IORQ (Direction: High 固定 / Z80→ESP)
    - WR: Z80 Pin 22 -> LVC #5 -> **GPIO 16**
    - RD: Z80 Pin 21 -> LVC #5 -> **GPIO 48**
    - MREQ: Z80 Pin 19 -> LVC #5 -> **GPIO 47**
    - IORQ: Z80 Pin 20 -> LVC #5 -> **GPIO 35**
