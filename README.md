  <div align="center">

  # 📡 STM32 无线遥控 Demo

  ![STM32](https://img.shields.io/badge/STM32-F103-blue)
  ![Language](https://img.shields.io/badge/Language-C-brightgreen)
  ![nRF24L01](https://img.shields.io/badge/无线-nRF24L01-orange)
  ![OLED](https://img.shields.io/badge/显示-OLED%20I2C-yellow)
  ![Encoder](https://img.shields.io/badge/输入-旋转编码器-green)

  *基于 STM32F103 的 nRF24L01 无线通信 + OLED 显示 + 旋转编码器输入工程*
  contact me:3458772695@qq.com(羡阳)

  </div>

  ---

  ## 📖 简介

  本项目基于 STM32F103，集成了三部分硬件：

  - **nRF24L01** 无线收发模块（SPI1 通信）
  - **OLED** 显示屏（I2C1 通信）
  - **旋转编码器**（TIM2 编码器模式）

  ---

  ## 🛠️ 硬件连接

  ### 1. nRF24L01 无线模块（SPI1 + 两个 GPIO）

  | 模块引脚 | STM32 引脚 | 说明 |
  |---------|-----------|------|
  | CSN（片选） | **PA3** | GPIO 输出，上电默认高电平（`main.h:60-61`） |
  | CE（使能） | **PA4** | GPIO 输出，上电默认低电平（`main.h:62-63`） |
  | SCK | **PA5** | SPI1 时钟 |
  | MISO | **PA6** | SPI1 主入从出 |
  | MOSI | **PA7** | SPI1 主出从入 |
  | IRQ | 未接 | 本工程用轮询 STATUS，不接中断脚 |
  | VCC / GND | 3.3V / GND | ⚠️ nRF 发射电流大，建议加 **10µF 去耦电容** |

  ### 2. OLED 显示屏（I2C1）

  | OLED 引脚 | STM32 引脚 | 说明 |
  |-----------|-----------|------|
  | SCL | **PB6** | I2C1 时钟 |
  | SDA | **PB7** | I2C1 数据 |
  | VCC / GND | 3.3V / GND | — |

  ### 3. 旋转编码器（TIM2 编码器模式）

  | 编码器引脚 | STM32 引脚 | 说明 |
  |-----------|-----------|------|
  | A 相 | **PA0** | TIM2_CH1 |
  | B 相 | **PA1** | TIM2_CH2 |
  | 公共端 / 电源 | 3.3V / GND | — |

  ---

  ## 📋 引脚总表

  | STM32 引脚 | 功能 | 所属外设 |
  |-----------|------|---------|
  | PA0 | 编码器 A 相 | TIM2_CH1 |
  | PA1 | 编码器 B 相 | TIM2_CH2 |
  | PA3 | CSN | nRF24L01 |
  | PA4 | CE | nRF24L01 |
  | PA5 | SCK | SPI1 → nRF24L01 |
  | PA6 | MISO | SPI1 → nRF24L01 |
  | PA7 | MOSI | SPI1 → nRF24L01 |
  | PB6 | SCL | I2C1 → OLED |
  | PB7 | SDA | I2C1 → OLED |

  > ✅ **无占用冲突**：PA0/PA1、PA3/PA4、PA5/6/7、PB6/7 全部独立，且避开了 PA8、PA13/PA14（SWD 调试口）。GPIOA 与 GPIOB
  的时钟均已使能（`main.c:332-333`）。

  ---

  ## ⚙️ 外设配置说明

  ### GPIO（CSN / CE）

  在 `MX_GPIO_Init()` 中初始化（`main.c:336-346`）：

  - `CSN_Pin | CE_Pin`：推挽输出、无上拉

  ### SPI1（`main.c:247-258`）

  - 主机模式、8 位、MSB 先行
  - **Mode 0**（POLARITY_LOW + PHASE_1EDGE）
  - 软件 NSS
  - 预分频 2（HSI 64MHz 下 SPI 时钟约 32MHz，稍高但可用）

  ### I2C1（`main.c:213-219`）

  - 100kHz、7 位地址
  - PB6/PB7 为 I2C1 硬件固定引脚

  ### TIM2（`main.c:288-299`）

  - 编码器模式 TI12（A、B 两相都计数）
  - PA0/PA1 为 TIM2_CH1/CH2 硬件固定引脚

  ---

  ## 🚀 使用方法

  1. 用 STM32CubeMX 打开 `DEMO.ioc`，确认配置后生成代码
  2. 编译并烧录到开发板
  3. 接线时注意 nRF24L01 供电加去耦电容，避免发射时掉电复位

  ---

  ## 📝 更新日志

  - `2026-10-01`：初始化工程，完成 nRF24L01 + OLED + 编码器接线与配置
