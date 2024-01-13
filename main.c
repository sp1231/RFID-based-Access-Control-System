#include "stm32f407xx.h"
#include "SysClockConfig.h"
#include "Config.h"

static uint8_t device_addr = 0x78; //slave address

/*

		OLED INTERFACING VIA I2C

*/


void i2c_config (void)
{	
	
	RCC->APB1ENR |= (1<<21); //enable i2c clock
	RCC->AHB1ENR |= (1<<1); //enable gpiob clock
	
	GPIOB->MODER |= (2<<16)|(2<<18); // alternate function for pb8 and pb9
	GPIOB->OTYPER |= (1<<8) | (1<<9); // open drain type output 
	GPIOB->OSPEEDR |= (1<<16) | (1<<18); //medium speed output
	GPIOB->PUPDR |= (1<<16) | (1<<18); // pull up output
	GPIOB->AFR[1] |= (4<<0) | (4<<4); // AF4 for I2C
	
	//reset I2C
	I2C1->CR1 |= (1<<15);
	I2C1->CR1 &= ~(1<<15);
	
	I2C1->CR2 |= (40<<0); // peripheral input clk 
	I2C1->CCR = (200<<0); // ((t_w+t_r)/t_clk)
	I2C1->TRISE = 41;
	I2C1->CR1 |= (1<<0); // enable i2c	

}

void i2c_start (void)
{
	I2C1->CR1 |= (1<<8); // set the start bit
	while(!(I2C1->SR1&(1<<0))); // wait for sb bit to set
}

void i2c_write (uint8_t data)
{
	while (!(I2C1->SR1 & (1<<7))); // wait for txe bit to set (checking if dr is empty)
	I2C1->DR = data; //writing data in dr reg
	while (!(I2C1->SR1 & (1<<2))); // wait for btf bit to set (checking for end of last data transmission )
}

void i2c_address (uint8_t Address)
{
		I2C1->DR = Address;	// sending slave address
	  while (!(I2C1->SR1 & (1<<1))); // wait for addr bit to set
	  uint8_t temp = I2C1->SR1 | I2C1->SR2; // read sr1 and sr2 to clear addr 
}

void i2c_stop (void)
{
	  I2C1->CR1 |=(1<<9);  //stop i2c
}
	
void I2C_WriteMulti (uint8_t *data, uint8_t size)
{
		while (!(I2C1->SR1 & (1<<7))); // wait fot txe bit to set
		while (size)
		{
			while (!(I2C1->SR1 & (1<<7)));  // wait for TXE bit to set 
			I2C1->DR = (uint32_t )*data;  // send data
			data++;
			size--;
		}
		while (!(I2C1->SR1 & (1<<2)));  // wait for btf bit to set
}

/*
	
		RFID INTERFACING VIA SPI 

*/


void spi_config (void)
{
	RCC->AHB1ENR |= (1<<0);  // Enable GPIO Clock
	GPIOA->MODER |= (2<<10)|(2<<12)|(2<<14)|(1<<18);  // Alternate functions for PA5, PA6, PA7 and Output for PA9	
	GPIOA->OSPEEDR |= (3<<10)|(3<<12)|(3<<14)|(3<<18);  // HIGH Speed for PA5, PA6, PA7, PA9	
	GPIOA->AFR[0] |= (5<<20)|(5<<24)|(5<<28);   // AF5(SPI1) for PA5, PA6, PA7
	
  RCC->APB2ENR |= (1<<12);  // Enable SPI1 CLock	
  SPI1->CR1 |= (1<<0)|(1<<1);   // CPOL=1, CPHA=1	
  SPI1->CR1 |= (1<<2);  // Master Mode
  SPI1->CR1 |= (3<<3);  // BR[2:0] = 011: fPCLK/16, PCLK2 = 80MHz, SPI clk = 5MHz
  SPI1->CR1 &= ~(1<<7);  // LSBFIRST = 0, MSB first
  SPI1->CR1 |= (1<<8) | (1<<9);  // SSM=1, SSi=1 -> Software Slave Management
  SPI1->CR1 &= ~(1<<10);  // RXONLY = 0, full-duplex
  SPI1->CR1 &= ~(1<<11);  // DFF=0, 8 bit data	
  SPI1->CR2 = 0;
} 

void spi_enable (void)
{
	SPI1->CR1 |= (1<<6);   // SPE=1, Peripheral enabled
}

void spi_disable (void)
{
	SPI1->CR1 &= ~(1<<6);   // SPE=0, Peripheral Disabled
}

void cs_enable (void)
{
	GPIOA->BSRR |= (1<<9)<<16;
}

void cs_disable (void)
{
	GPIOA->BSRR |= (1<<9);
}

void SPI_Transmit (uint8_t *data, int size)
{
	
	/************** STEPS TO FOLLOW *****************
	1. Wait for the TXE bit to set in the Status Register
	2. Write the data to the Data Register
	3. After the data has been transmitted, wait for the BSY bit to reset in Status Register
	4. Clear the Overrun flag by reading DR and SR
	************************************************/		
	
	int i=0;
	while (i<size)
	{
	   while (!((SPI1->SR)&(1<<1))) {};  // wait for TXE bit to set -> This will indicate that the buffer is empty
	   SPI1->DR = data[i];  // load the data into the Data Register
	   i++;
	}	
	
	
/*During discontinuous communications, there is a 2 APB clock period delay between the
write operation to the SPI_DR register and BSY bit setting. As a consequence it is
mandatory to wait first until TXE is set and then until BSY is cleared after writing the last
data.
*/
	while (!((SPI1->SR)&(1<<1))) {};  // wait for TXE bit to set -> This will indicate that the buffer is empty
	while (((SPI1->SR)&(1<<7))) {};  // wait for BSY bit to Reset -> This will indicate that SPI is not busy in communication	
	
	//  Clear the Overrun flag by reading DR and SR
	uint8_t temp = SPI1->DR;
					temp = SPI1->SR;
	
}

void spi_receive (uint8_t *data, int size)
{
	/************** STEPS TO FOLLOW *****************
	1. Wait for the BSY bit to reset in Status Register
	2. Send some Dummy data before reading the DATA
	3. Wait for the RXNE bit to Set in the status Register
	4. Read data from Data Register
	************************************************/		

	while (size)
	{
		while (((SPI1->SR)&(1<<7))) {};  // wait for BSY bit to Reset -> This will indicate that SPI is not busy in communication
		SPI1->DR = 0;  // send dummy data
		while (!((SPI1->SR) &(1<<0))){};  // Wait for RXNE to set -> This will indicate that the Rx buffer is not empty
	  *data++ = (SPI1->DR);
		size--;
	}	
}

/*
		KEYPAD INTERFACING VIA GPIO , REST INSIDE MAIN FUNCTION
*/

//noting down base address of GPIOA Peripherals:
//Range: 0x4002 0000 - 0x4002 03FF GPIOA
//noting down base address of RCC Unit also:
//Range: 0x4002 3800 - 0x4002 3BFF RCC (Reset and Clock Control)

void delay() //to avoid button re-bouncing time resulting in printing multiple times
{
	for(uint32_t i = 0; i<100;i++);
}






int main(void)
{
	SysClockConfig();
	
	//I2C
	
	i2c_config();
	i2c_address(device_addr);
	i2c_write(0b10000000); //	control byte
	i2c_write(0xA4); // command byte
	i2c_stop(); 
	
	// SPI
	
/*	
	spi_config();
	spi_enable();
	cs_enable();
	SPI_Transmit();
	cs_disable();
*/

	
	// KEYPAD USING GPIO

//Connected PA0 - PA3 to R1-R4
	//Connected PA6 - PA9 to C1-C4

	//type-casting to store addresses to the variable.
	//volatile keyword indicates that it can be changed at anytime.
	// Offset for RCC AHB1 peripheral clocl enable register is 0x30
	uint32_t volatile *pRCC_Base_add = (uint32_t*) 0x40023830;


	//GPIOx_MODER Register. Offset: 0x00
	uint32_t volatile *pGPIOModer = (uint32_t*) 0x40020000;


	//Output Data Register and Input Data Register:
	//using GPIOx_ODR. Offset = 0x14
	uint32_t volatile *pGPIOAODR = (uint32_t*) 0x40020014;

	//using GPIOx_IDR. Offset = 0x10.
	uint32_t volatile *pGPIOAIDR = (uint32_t*) 0x40020010;


	//Pull up and Pull down registers required.
	//Using GPIOx_PUPDR
	uint32_t volatile *pGPIOAPullup = (uint32_t*) 0x4002000C;

	//Enable the GPIOA Peripheral
	*pRCC_Base_add|=1; //setting the 0th bit of AHB1ENR Register.


	*pGPIOModer &= ~(0xFF<<12); //PA6 to PA9 are set to Input.

	*pGPIOModer &= ~(0xff<<0); //clearing the bits that we want to set.
	*pGPIOModer |= (0x55<<0);  //PA0 to PA3 set to OUTPUT.


	*pGPIOAPullup &= ~(0xff<<12);
	*pGPIOAPullup |= (0x55<<12);

	while(1)
	{
		*pGPIOAODR |= (0x0f<<0); //PA0 to PA3 all are HIGH.
		*pGPIOAODR &= ~(1<<0); //PA0 or R1 is LOW
		//scanning the columns:

		if(!(*pGPIOAIDR & (1<<6)))
				{
				  printf("1");
				  delay();

				}


		if(!(*pGPIOAIDR & (1<<7)))
				{
				  printf("2");
				  delay();

				}

		if(!(*pGPIOAIDR & (1<<8)))
				{
				  printf("3");
				  delay();

				}

		if(!(*pGPIOAIDR & (1<<9)))
				{
				  printf("A");
				  delay();

				}


		*pGPIOAODR |= (0x0f<<0); //PA0 to PA3 all are HIGH.
		*pGPIOAODR &= ~(1<<1); //PA1 or R2 is LOW.

		if(!(*pGPIOAIDR & (1<<6)))
				{
				  printf("4");
				  delay();

				}


		if(!(*pGPIOAIDR & (1<<7)))
				{
				  printf("5");
				  delay();

				}

		if(!(*pGPIOAIDR & (1<<8)))
				{
				  printf("6");
				  delay();

				}

		if(!(*pGPIOAIDR & (1<<9)))
				{
				  printf("B");
				  delay();

				}



		*pGPIOAODR |= (0x0f<<0); //PA0 to PA3 all are HIGH.
		*pGPIOAODR &= ~(1<<2); //PA2 or R3 is LOW.

				if(!(*pGPIOAIDR & (1<<6)))
						{
						  printf("7");
						  delay();

						}


				if(!(*pGPIOAIDR & (1<<7)))
						{
						  printf("8");
						  delay();

						}

				if(!(*pGPIOAIDR & (1<<8)))
						{
						  printf("9");
						  delay();

						}

				if(!(*pGPIOAIDR & (1<<9)))
						{
						  printf("C");
						  delay();

						}


		*pGPIOAODR |= (0x0f<<0); //PA0 to PA3 all are HIGH.
		*pGPIOAODR &= ~(1<<3); //PA3 or R4 is LOW.

						if(!(*pGPIOAIDR & (1<<6)))
								{
								  printf("0");
								  delay();

								}


						if(!(*pGPIOAIDR & (1<<7)))
								{
								  printf("F");
								  delay();

								}

						if(!(*pGPIOAIDR & (1<<8)))
								{
								  printf("E");
								  delay();

								}

						if(!(*pGPIOAIDR & (1<<9)))
								{
								  printf("D");
								  delay();

								}

	}

	while(1){
	}
	
}