#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f1xx_hal.h"

void NRF24L01_Init(void);
uint8_t NRF24L01_Check(void);
void NRF24L01_TXMode(void);
void NRF24L01_RXMode(void);
uint8_t NRF24L01_TxPacket(uint8_t *buf, uint8_t len);
uint8_t NRF24L01_RxPacket(uint8_t *buf);

#endif
