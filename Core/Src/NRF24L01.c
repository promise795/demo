#include "main.h"
#include "NRF24L01.h"


#define DYNPD    0x1C
#define FEATURE  0x1D

/* 寄存器地址 */
#define CONFIG      0x00   // 配置寄存器（模式、CRC、上电）
#define EN_AA       0x01   // 使能自动应答
#define EN_RXADDR   0x02   // 使能接收通道
#define SETUP_AW    0x03   // 地址宽度
#define SETUP_RETR  0x04   // 自动重发设置
#define RF_CH       0x05   // 无线频道
#define RF_SETUP    0x06   // 速率和发射功率
#define STATUS      0x07   // 状态寄存器（中断标志）
#define RX_ADDR_P0  0x0A   // 接收通道0地址
#define TX_ADDR     0x10   // 发送地址
#define RX_PW_P0    0x11   // 接收通道0载荷宽度

/* 命令 */
#define R_REGISTER      0x00  // 读寄存器（再 | 寄存器地址）
#define W_REGISTER      0x20  // 写寄存器（再 | 寄存器地址）
#define R_RX_PAYLOAD    0x61  // 读接收载荷
#define W_TX_PAYLOAD    0xA0  // 写发送载荷
#define FLUSH_TX        0xE1  // 清空发送 FIFO
#define FLUSH_RX        0xE2  // 清空接收 FIFO
#define R_RX_PL_WID     0x60  // 读接收载荷长度
#define NOP             0xFF  // 空操作

/* STATUS 寄存器标志位 */
#define RX_DR   0x40   // 收到数据
#define TX_DS   0x20   // 发送完成
#define MAX_RT  0x10   // 达到最大重发次数


extern SPI_HandleTypeDef hspi1;/*cubeMX生成的SPI1句柄*/
 
static uint8_t NRF24L01_SPI_SwapByte(uint8_t byte){
    uint8_t rx=0;/* 同时收一个、发一个：SPI 是全双工，每发一个字节同时会收回来一个 */
    HAL_SPI_TransmitReceive(&hspi1,&byte,&rx,1,10);
    return rx;
}//交换一个字节

#define NRF24L01_CE_High()   HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_SET)
#define NRF24L01_CE_Low()    HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_RESET)
#define NRF24L01_CSN_High()  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET)
#define NRF24L01_CSN_Low()   HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_RESET)

static uint8_t NRF24L01_ReadReg(uint8_t reg){
    uint8_t val;
    NRF24L01_CSN_Low();
    NRF24L01_SPI_SwapByte(R_REGISTER | reg);  // 发"读寄存器"指令
    val = NRF24L01_SPI_SwapByte(NOP);      // 再发一字节，收回来的就是寄存器值
    NRF24L01_CSN_High();                   // 结束会话
    return val;
}//读寄存器

static void NRF24L01_WriteReg(uint8_t reg,uint8_t val){
    NRF24L01_CSN_Low();
    NRF24L01_SPI_SwapByte(W_REGISTER | reg);// 发"写寄存器"指令
    NRF24L01_SPI_SwapByte(val);// 发要写的值
    NRF24L01_CSN_High();
}//写寄存器

static void NRF24L01_WriteBuf(uint8_t reg,uint8_t *buf,uint8_t len){
    NRF24L01_CSN_Low();
    NRF24L01_SPI_SwapByte(reg);// 指令（注意：这里直接传完整指令字节）
    for(uint8_t i=0;i<len;i++)NRF24L01_SPI_SwapByte(buf[i]);
    NRF24L01_CSN_High();
}//写缓冲区

static void NRF24L01_ReadBuf(uint8_t reg,uint8_t *buf,uint8_t len){
    NRF24L01_CSN_Low();
    NRF24L01_SPI_SwapByte(reg);
    for(uint8_t i=0;i<len;i++)buf[i]=NRF24L01_SPI_SwapByte(NOP);
    NRF24L01_CSN_High();
}//读缓冲区

/*注意：
WriteBuf/ReadBuf 的第一个参数 reg 传的是完整指令字节（比如 W_REGISTER|TX_ADDR、W_TX_PAYLOAD），
而 ReadReg/WriteReg 传的是纯寄存器地址（内部再 |R_REGISTER）。
别搞混。*/

static void NRF24L01_FlushTx(void)
{
    NRF24L01_CSN_Low();
    NRF24L01_SPI_SwapByte(FLUSH_TX);
    NRF24L01_CSN_High();
}
static void NRF24L01_FlushRx(void)
{
    NRF24L01_CSN_Low();
    NRF24L01_SPI_SwapByte(FLUSH_RX);
    NRF24L01_CSN_High();
}



static const uint8_t NRF24L01_Address[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
/* 通信地址，收发两端必须完全一致 */
void NRF24L01_Init(void)
{
    NRF24L01_CE_Low();          // 初始化期间 CE 拉低，防止误发射
    NRF24L01_CSN_High();        // CSN 空闲必须高
    HAL_Delay(10);              // 等模块上电稳定

    NRF24L01_WriteReg(EN_AA, 0x01);        // 使能通道0自动应答
    NRF24L01_WriteReg(EN_RXADDR, 0x01);    // 使能接收通道0
    NRF24L01_WriteReg(SETUP_AW, 0x03);     // 地址宽度 5 字节
    NRF24L01_WriteReg(SETUP_RETR, 0x1A);   // 重发间隔500us、最多重发10次
    NRF24L01_WriteReg(RF_CH, 40);          // 频道 2.440GHz
    NRF24L01_WriteReg(RF_SETUP, 0x0F);   // 2Mbps、0dBm（原来是 0x06）
    NRF24L01_WriteReg(RX_PW_P0, 32);       // 通道0载荷32字节
    // NRF24L01_WriteReg(FEATURE, 0x04);   // 使能动态载荷长度
    // NRF24L01_WriteReg(DYNPD,   0x01);   // 通道0启用动态载荷

    NRF24L01_WriteBuf(W_REGISTER | TX_ADDR,   (uint8_t*)NRF24L01_Address, 5);
    NRF24L01_WriteBuf(W_REGISTER | RX_ADDR_P0, (uint8_t*)NRF24L01_Address, 5);

    NRF24L01_WriteReg(STATUS, RX_DR | TX_DS | MAX_RT);  // 清空所有中断标志
    NRF24L01_WriteBuf(FLUSH_TX, 0, 0);   // 这两行是清空 FIFO
    NRF24L01_WriteBuf(FLUSH_RX, 0, 0);
}

uint8_t NRF24L01_Check(void)
{
    uint8_t buf[5] = {0xA5, 0xA5, 0xA5, 0xA5, 0xA5};
    uint8_t i;

    NRF24L01_WriteBuf(W_REGISTER | TX_ADDR, buf, 5);  // 写测试值
    NRF24L01_ReadBuf (R_REGISTER | TX_ADDR, buf, 5);  // 读回来
    for (i = 0; i < 5; i++)
        if (buf[i] != 0xA5) break;
    return (i == 5) ? 1 : 0;   // 5个都对 → 模块在
}//检测模块是否存在

void NRF24L01_TXMode(void)
{
    NRF24L01_CE_Low();                                  // 切换模式先拉低 CE
    NRF24L01_WriteReg(CONFIG, 0x0E);                    // PRIM_RX=0 → 发送模式
    NRF24L01_WriteBuf(W_REGISTER | TX_ADDR,   (uint8_t*)NRF24L01_Address, 5);
    NRF24L01_WriteBuf(W_REGISTER | RX_ADDR_P0, (uint8_t*)NRF24L01_Address, 5); // 自动应答要收 ACK，必须一致
}//发模式切换

void NRF24L01_RXMode(void)
{
    NRF24L01_CE_Low();
    NRF24L01_WriteReg(CONFIG, 0x0F);                    // PRIM_RX=1 → 接收模式
    NRF24L01_WriteBuf(W_REGISTER | RX_ADDR_P0, (uint8_t*)NRF24L01_Address, 5);
    NRF24L01_FlushRx();
    NRF24L01_CE_High();                                 // 接收模式 CE 必须一直高
}//收模式切换

uint8_t NRF24L01_TxPacket(uint8_t *buf, uint8_t len)
{
    uint8_t status;
    uint32_t start;

    NRF24L01_CE_Low();                          // 发射前 CE 低
    NRF24L01_FlushTx();                         // 清空旧数据
    NRF24L01_WriteBuf(W_TX_PAYLOAD, buf, len);  // 把数据写进发送 FIFO
    NRF24L01_CE_High();                         // CE 拉高 = 触发发射

    start = HAL_GetTick();                      // 等待发送完成（轮询 STATUS）
    while (1) {
        status = NRF24L01_ReadReg(STATUS);
        if (status & (TX_DS | MAX_RT)) break;   // 发完了 / 重发到上限了
        if (HAL_GetTick() - start > 100) break; // 超时 100ms 兜底
    }
    NRF24L01_CE_Low();
    NRF24L01_WriteReg(STATUS, RX_DR | TX_DS | MAX_RT);  // 清中断标志（写1清0）
    return (status & TX_DS) ? 1 : 0;            // TX_DS=发送成功
}//发送一帧

uint8_t NRF24L01_RxPacket(uint8_t *buf)
{
    uint8_t status = NRF24L01_ReadReg(STATUS);
    if (!(status & RX_DR)) return 0;
    NRF24L01_ReadBuf(R_RX_PAYLOAD, buf, 32);   // 固定读 32 字节
    NRF24L01_WriteReg(STATUS, RX_DR);
    return 32;

}//接收一帧





