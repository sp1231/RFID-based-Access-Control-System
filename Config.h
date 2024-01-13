#include "stm32f407xx.h"
void i2c_config (void);
void i2c_start (void);
void i2c_write (uint8_t data);
void i2c_address (uint8_t Address);
void i2c_stop (void);
void I2C_WriteMulti (uint8_t *data, uint8_t size);
void spi_config (void);
void spi_enable (void);
void spi_disable (void);
void cs_enable (void);
void cs_disable (void);
void SPI_Transmit (uint8_t *data, int size);
void spi_receive (uint8_t *data, int size);
void delay(void);