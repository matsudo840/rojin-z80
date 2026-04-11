# 老人Z80 回路図 (Mermaid版)

## 1. システム全体構成図

```mermaid
graph TB
    subgraph Power["電源供給系統"]
        USB[USB 5V電源]
        USB --> Z80_VCC[Z80 VCC]
        USB --> HC574_VCC[74HC574 VCC]
        USB --> LVC_A[レベルシフタ A側 5V]

        ESP_3V3[ESP32 3.3V出力]
        ESP_3V3 --> LVC_VCC[レベルシフタ VCC 3.3V]
        ESP_3V3 --> LVC_B[レベルシフタ B側 3.3V]

        GND[共通GND]
    end

    subgraph CPU["Z80 CPU (老人)"]
        Z80[Z84C0008<br/>8MHz CMOS<br/>5V駆動]
    end

    subgraph LevelShift["神経系 (電圧変換)"]
        LVC1[LVC245 #1<br/>A0-A7]
        LVC2[LVC245 #2<br/>A8-A15]
        LVC3[LVC245 #3<br/>D0-D7<br/>双方向]
        LVC4[LVC245 #4<br/>制御信号<br/>ESP→Z80]
        LVC5[LVC245 #5<br/>制御信号<br/>Z80→ESP]
    end

    subgraph MCU["ESP32 (介護士)"]
        ESP[ESP32-S3<br/>DevKitC-1<br/>N16R8<br/>3.3V駆動]
    end

    subgraph Output["出力系 (脈動)"]
        HC574[74HC574<br/>ラッチ]
        LED[LED×8<br/>バイタルサイン]
    end

    subgraph Cloud["AI (意識)"]
        ChatGPT[ChatGPT API<br/>プログラム生成]
    end

    Z80 -->|アドレスバス<br/>A0-A7| LVC1
    Z80 -->|アドレスバス<br/>A8-A15| LVC2
    Z80 <-->|データバス<br/>D0-D7| LVC3
    Z80 <-->|制御信号| LVC4
    Z80 -->|MREQ, RD| LVC5

    LVC1 -->|GPIO 1-8| ESP
    LVC2 -->|GPIO 9-16| ESP
    LVC3 <-->|GPIO 17-42| ESP
    LVC4 <-->|CLK(0), WAIT, RESET| ESP
    LVC5 -->|GPIO 47-48| ESP

    Z80 -->|データ出力| HC574
    HC574 --> LED

    ESP -->|Wi-Fi| ChatGPT

    style Z80 fill:#ff9999
    style ESP fill:#99ccff
    style LVC1 fill:#ffeb99
    style LVC2 fill:#ffeb99
    style LVC3 fill:#ffeb99
    style LVC4 fill:#ffeb99
    style LVC5 fill:#ffeb99
    style ChatGPT fill:#99ff99
```

## 2. データフロー詳細図

```mermaid
flowchart LR
    subgraph Z80_Side["Z80側 (5V)"]
        A_BUS[アドレスバス<br/>A0-A15<br/>16本]
        D_BUS[データバス<br/>D0-D7<br/>8本]
        CTRL_OUT[制御出力<br/>MREQ, RD]
        CTRL_IN[制御入力<br/>CLK, WAIT, RESET]
    end

    subgraph Converter["レベル変換 (5V ↔ 3.3V)"]
        LS_ADDR[LVC245 #1,#2<br/>Dir=Low<br/>一方向]
        LS_DATA[LVC245 #3<br/>Dir制御<br/>双方向]
        LS_OUT[LVC245 #5<br/>Dir=Low<br/>一方向]
        LS_IN[LVC245 #4<br/>Dir制御<br/>一方向]
    end

    subgraph ESP_Side["ESP32側 (3.3V)"]
        GPIO_ADDR[GPIO 1-16<br/>アドレス受信]
        GPIO_DATA[GPIO 17-42<br/>データ送受信]
        GPIO_MREQ[GPIO 47-48<br/>制御受信]
        GPIO_CTRL[GPIO 0,45,46<br/>制御送信]
        GPIO_DIR[GPIO 19<br/>方向制御]
    end

    A_BUS -->|5V| LS_ADDR
    LS_ADDR -->|3.3V| GPIO_ADDR

    D_BUS <-->|5V| LS_DATA
    LS_DATA <-->|3.3V| GPIO_DATA
    GPIO_DIR -.->|制御| LS_DATA

    CTRL_OUT -->|5V| LS_OUT
    LS_OUT -->|3.3V| GPIO_MREQ

    GPIO_CTRL -->|3.3V| LS_IN
    LS_IN -->|5V| CTRL_IN

    style A_BUS fill:#ffffcc
    style D_BUS fill:#ccffcc
    style CTRL_OUT fill:#ccccff
    style CTRL_IN fill:#ffccff
```

## 3. ピンアサイン詳細マップ

```mermaid
graph LR
    subgraph Z80_Pins["Z80 ピン配置"]
        direction TB
        A0_A7["A0-A7<br/>pin 30-37"]
        A8_A15["A8-A15<br/>pin 38-40, 1-5"]
        D0_D7["D0-D7<br/>pin 14,15,12,8,7,9,10,13"]
        CLK_PIN["CLK pin 6"]
        WAIT_PIN["WAIT pin 24"]
        RESET_PIN["RESET pin 26"]
        MREQ_PIN["MREQ pin 19"]
        RD_PIN["RD pin 21"]
    end

    subgraph LVC1_IC["LVC245 #1"]
        LVC1_A["A1-A8<br/>(5V側/Z80)"]
        LVC1_B["B1-B8<br/>(3.3V側/ESP)"]
        LVC1_A --> LVC1_B
    end

    subgraph LVC2_IC["LVC245 #2"]
        LVC2_A["A1-A8<br/>(5V側/Z80)"]
        LVC2_B["B1-B8<br/>(3.3V側/ESP)"]
        LVC2_A --> LVC2_B
    end

    subgraph LVC3_IC["LVC245 #3"]
        LVC3_A["A1-A8<br/>(5V側/Z80)"]
        LVC3_B["B1-B8<br/>(3.3V側/ESP)"]
        LVC3_DIR["DIR<br/>GPIO 19"]
        LVC3_B <--> LVC3_A
    end

    subgraph ESP_GPIO["ESP32 GPIO"]
        direction TB
        GPIO_1_8["GPIO 1-8"]
        GPIO_9_16["GPIO 9-16"]
        GPIO_17_42["GPIO 17,18,21,38-42"]
        GPIO_0["GPIO 0"]
        GPIO_45["GPIO 45"]
        GPIO_46["GPIO 46"]
        GPIO_47["GPIO 47"]
        GPIO_48["GPIO 48"]
        GPIO_19["GPIO 19"]
    end

    A0_A7 --> LVC1_A
    LVC1_B --> GPIO_1_8

    A8_A15 --> LVC2_A
    LVC2_B --> GPIO_9_16

    D0_D7 <--> LVC3_A
    LVC3_B <--> GPIO_17_42
    GPIO_19 --> LVC3_DIR

    GPIO_0 --> CLK_PIN
    GPIO_45 --> WAIT_PIN
    GPIO_46 --> RESET_PIN

    MREQ_PIN --> GPIO_47
    RD_PIN --> GPIO_48

    style A0_A7 fill:#ffe6cc
    style A8_A15 fill:#ffe6cc
    style D0_D7 fill:#ccffe6
    style CLK_PIN fill:#cce6ff
    style WAIT_PIN fill:#cce6ff
    style RESET_PIN fill:#cce6ff
```

## 4. 仮想メモリ動作シーケンス

```mermaid
sequenceDiagram
    participant Z80 as Z80 CPU
    participant CLK as CLK信号
    participant MREQ as MREQ信号
    participant WAIT as WAIT信号
    participant ESP as ESP32
    participant AI as ChatGPT

    Note over ESP,AI: 初期化: AIからプログラムを取得
    AI->>ESP: Z80マシン語コード送信
    ESP->>ESP: 仮想メモリに展開

    Note over Z80,ESP: 実行開始
    ESP->>CLK: クロックHigh
    activate Z80
    CLK->>Z80: 1クロック進行

    Z80->>MREQ: Low (メモリ要求)
    activate MREQ
    MREQ->>ESP: メモリアクセス検知

    ESP->>WAIT: Low (Z80を停止)
    activate WAIT
    WAIT->>Z80: 時間停止

    ESP->>ESP: アドレスバス読み取り<br/>(GPIO 1-16)
    ESP->>ESP: 仮想メモリから<br/>該当命令を取得

    ESP->>Z80: データバス経由で<br/>命令を供給<br/>(GPIO 17-42)

    ESP->>WAIT: High (停止解除)
    deactivate WAIT
    WAIT->>Z80: 時間再開

    deactivate MREQ
    Z80->>Z80: 命令実行
    deactivate Z80

    ESP->>CLK: クロックLow

    Note over Z80,ESP: 次のサイクルへ
```

## 5. タイミングチャート

```mermaid
%%{init: {'theme':'base'}}%%
gantt
    title Z80 仮想メモリ読み出しタイミング
    dateFormat X
    axisFormat %L

    section CLK (GPIO 0)
    High   :a1, 0, 100
    Low    :a2, 100, 100
    High   :a3, 200, 100

    section MREQ (Z80→47)
    High   :b1, 0, 50
    Low    :b2, 50, 150
    High   :b3, 200, 100

    section WAIT (45→Z80)
    High   :c1, 0, 60
    Low    :c2, 60, 100
    High   :c3, 160, 140

    section Address (Z80→1-16)
    Invalid:d1, 0, 50
    Valid  :crit, d2, 50, 150
    Invalid:d3, 200, 100

    section Data (17-42)
    HiZ    :e1, 0, 100
    Valid  :crit, e2, 100, 60
    HiZ    :e3, 160, 140

    section DIR (GPIO 19)
    Low    :f1, 0, 100
    High   :f2, 100, 60
    Low    :f3, 160, 140
```

## 6. 電源配線図

```mermaid
graph TD
    subgraph PowerSource["電源源"]
        USB[USB 5V]
        ESP_REG[ESP32 3.3V<br/>レギュレータ出力]
    end

    subgraph V5_Rail["5V電源レール"]
        Z80_V[Z80 VCC pin11]
        HC_V[74HC574 VCC pin20]
        LVC_A_V[LVC245 A側<br/>入力基準]
    end

    subgraph V3_Rail["3.3V電源レール"]
        LVC_VCC[LVC245 ×5<br/>VCC pin20]
        LVC_B_V[LVC245 B側<br/>入力基準]
    end

    subgraph GroundRail["共通GND"]
        Z80_G[Z80 GND pin29]
        ESP_G[ESP32 GND]
        LVC_G[LVC245 ×5<br/>GND pin10]
        HC_G[74HC574 GND pin10]
        LED_G[LED カソード]
        USB_G[USB GND]
    end

    subgraph Bypass["バイパスコンデンサ"]
        CAP_Z80[0.1μF<br/>Z80 VCC-GND]
        CAP_ESP[0.1μF ×複数<br/>ESP32]
        CAP_LVC[0.1μF ×5<br/>各LVC245]
        CAP_HC[0.1μF<br/>74HC574]
    end

    USB --> V5_Rail
    USB --> GroundRail
    ESP_REG --> V3_Rail
    ESP_REG --> GroundRail

    V5_Rail -.->|接続| CAP_Z80
    V5_Rail -.->|接続| CAP_HC
    V3_Rail -.->|接続| CAP_ESP
    V3_Rail -.->|接続| CAP_LVC

    CAP_Z80 -.-> GroundRail
    CAP_ESP -.-> GroundRail
    CAP_LVC -.-> GroundRail
    CAP_HC -.-> GroundRail

    style USB fill:#ff9999
    style ESP_REG fill:#99ccff
    style GroundRail fill:#333333,color:#ffffff
```

## 7. LED出力回路

```mermaid
graph LR
    subgraph Z80_Data["Z80 データバス"]
        D0[D0 pin14]
        D1[D1 pin15]
        D2[D2 pin12]
        D3[D3 pin8]
        D4[D4 pin7]
        D5[D5 pin9]
        D6[D6 pin10]
        D7[D7 pin13]
    end

    subgraph Control["制御信号"]
        IORQ[IORQ pin20]
        WR[WR pin22]
        AND[AND Gate]
        IORQ --> AND
        WR --> AND
    end

    subgraph Latch["74HC574 ラッチ"]
        direction TB
        IN[D1-D8入力]
        CLK_L[CLK pin11]
        OUT[Q1-Q8出力]
        IN --> OUT
    end

    subgraph LED_Array["LED表示"]
        LED0[LED0 + 330Ω]
        LED1[LED1 + 330Ω]
        LED2[LED2 + 330Ω]
        LED3[LED3 + 330Ω]
        LED4[LED4 + 330Ω]
        LED5[LED5 + 330Ω]
        LED6[LED6 + 330Ω]
        LED7[LED7 + 330Ω]
    end

    D0 --> IN
    D1 --> IN
    D2 --> IN
    D3 --> IN
    D4 --> IN
    D5 --> IN
    D6 --> IN
    D7 --> IN

    AND --> CLK_L

    OUT --> LED0
    OUT --> LED1
    OUT --> LED2
    OUT --> LED3
    OUT --> LED4
    OUT --> LED5
    OUT --> LED6
    OUT --> LED7

    LED0 --> GND[GND]
    LED1 --> GND
    LED2 --> GND
    LED3 --> GND
    LED4 --> GND
    LED5 --> GND
    LED6 --> GND
    LED7 --> GND

    style LED0 fill:#ffff99
    style LED1 fill:#ffff99
    style LED2 fill:#ffff99
    style LED3 fill:#ffff99
    style LED4 fill:#ffff99
    style LED5 fill:#ffff99
    style LED6 fill:#ffff99
    style LED7 fill:#ffff99
```

## 8. ブレッドボード配置図

```mermaid
graph TB
    subgraph BB1["ブレッドボード #1"]
        direction LR
        ESP32[ESP32-S3 DevKitC-1<br/>幅広40ピン構成<br/>両側に電源レール]
        POWER1[5V/3.3V/GND<br/>電源分配]
    end

    subgraph BB2["ブレッドボード #2"]
        direction LR
        LVC1[LVC245 #1<br/>A0-A7]
        LVC2[LVC245 #2<br/>A8-A15]
        LVC3[LVC245 #3<br/>D0-D7]
        LVC1 --- LVC2
        LVC2 --- LVC3
    end

    subgraph BB3["ブレッドボード #3"]
        direction LR
        Z80_IC[Z80 CPU<br/>DIP40ソケット]
        LVC4[LVC245 #4<br/>制御ESP→Z]
        LVC5[LVC245 #5<br/>制御Z→ESP]
        HC[74HC574<br/>+ LED×8]
        Z80_IC --- LVC4
        LVC4 --- LVC5
        LVC5 --- HC
    end

    BB1 -.->|フラットケーブル<br/>40本程度| BB2
    BB2 -.->|フラットケーブル<br/>20本程度| BB3

    style ESP32 fill:#99ccff
    style Z80_IC fill:#ff9999
    style LVC1 fill:#ffeb99
    style LVC2 fill:#ffeb99
    style LVC3 fill:#ffeb99
    style LVC4 fill:#ffeb99
    style LVC5 fill:#ffeb99
```

## 9. パスコン（バイパスコンデンサ）配置

各ICのVCC-GND間に0.1μFセラミックコンデンサを**できるだけ近く**に配置する。

| IC | 電源電圧 | パスコン容量 | 数量 | 配置位置 |
|----|---------|------------|------|---------|
| Z80 CPU | 5V | 0.1μF | 1個 | pin11-pin29間 |
| 74HC574 | 5V | 0.1μF | 1個 | pin20-pin10間 |
| LVC245 #1 | 3.3V | 0.1μF | 1個 | pin20-pin10間 |
| LVC245 #2 | 3.3V | 0.1μF | 1個 | pin20-pin10間 |
| LVC245 #3 | 3.3V | 0.1μF | 1個 | pin20-pin10間 |
| LVC245 #4 | 3.3V | 0.1μF | 1個 | pin20-pin10間 |
| LVC245 #5 | 3.3V | 0.1μF | 1個 | pin20-pin10間 |
| ESP32-S3 | 3.3V | 0.1μF | 2-3個 | 各3.3V-GND間（複数箇所） |
| ESP32-S3 | 3.3V | 10μF | 1個 | 3.3V-GND間（バルク用） |

**合計必要数: 0.1μF × 9個、10μF × 1個**

## 10. 配線色コーディング

```mermaid
graph LR
    subgraph Colors["推奨配線色"]
        direction TB
        RED[赤: 5V電源]
        ORANGE[オレンジ: 3.3V電源]
        BLACK[黒: GND]
        YELLOW[黄色系: アドレスバス]
        GREEN[緑色系: データバス]
        BLUE[青色系: 制御信号]
        WHITE[白: クロック]
    end

    style RED fill:#ff0000,color:#ffffff
    style ORANGE fill:#ff9900,color:#000000
    style BLACK fill:#000000,color:#ffffff
    style YELLOW fill:#ffff00,color:#000000
    style GREEN fill:#00ff00,color:#000000
    style BLUE fill:#0099ff,color:#ffffff
    style WHITE fill:#ffffff,color:#000000
```
