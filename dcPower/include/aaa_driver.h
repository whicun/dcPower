#ifndef		__UDD_OPTION_H_
#define		__UDD_OPTION_H_

#define   RPM2WRM            (2.*PI/60.) 
#define   WRM2RPM            (60./(2.*PI)) 
#define SWITCHING_FREQ      10000
#define ADC_CONST         0.00048828125   // 1/2048
//#define DUAL_PWM_INTERRUPT      (0)          //   PWM Top & Bottom Interrupt

//*******************************************
// Program Control 
//*******************************************
#define FLASH_RUN               1
#define MAIN_POWER_OFF			0			// Vdc == 300.0
#define CONVERTER_ON			0
#define USE_UART_A              1
#define TEST_ADC_CENTER			1

#define USE_GRAPH               0
#define GRAPH_NUMBER			300

#define TYPE_INTEGER			0
#define TYPE_FLOAT				1

#define EEPROM_RW_DELAY			10	

#define DEAD_TIME_COUNT			137			// 90Mhz

#define TRIP_BACKUP_ADDR		3000		
#define EPROM_ADDR_TRIP_POINT	4010
#define EPROM_ADDR_CHECKSUM		4020

typedef union
{
	float	dword;			/* float word : 32bit */
	struct
	{
		int word0;		/* ���� 16��Ʈ(least siginificant word) */
		int word1;		/* ���� 16��Ʈ(most siginificant word) */
	} word;	
	struct
	{
		unsigned byte0	:8;	/* LSB */
		unsigned byte1	:8;
		unsigned byte2	:8;	/* sign + MSB */
		unsigned byte3	:8;	/* exponent */
	} byte;
} UNION32;

#define	STATE_POWER_ON			0
#define	STATE_READY				1
#define	STATE_RUN				2
#define STATE_TRIP              3
#define STATE_TO_RESET          4
#define	STATE_INIT_RUN			5
#define	STATE_GO_STOP			6
#define	STATE_WAIT_BREAK_OFF	7

//--- I2C
#define ADDR_24LC32	0x0050		// address of 24ls32 serial eeprom
#define I2C_EEPROM_SLAVE_ADDR 	0x50

#define I2C_NUMBYTES          	4
#define I2C_EEPROM_HIGH_ADDR  	0x00
#define I2C_EEPROM_LOW_ADDR   	0x30

//-- uart 

#define SCIA_RX_BUF_MAX     30
#define SCIA_TX_BUF_MAX     80

#define CMD_NULL			  	0
#define CMD_START				0x0041	//'A'
#define CMD_STOP				0x0042	//'B'
#define CMD_RESET				0x0043	//'C'
#define CMD_SPEED_UP			0x0044	//'D'	
#define CMD_SPEED_DOWN			0x0045	//'E'
#define CMD_READ_DATA			0x0046	//'F'
#define CMD_WRITE_RAM			0x0047	//'G'
#define CMD_WRITE_ROM			0x0048	//'H'
#define CMD_READ_ALL			0x0049	//'I'
#define CMD_WRITE_ALL			0x004A	//'J'
#define CMD_CODE_MSG			0x004B	//'K'
#define CMD_WRITE_CHECK			0x004C	//'L'
#define CMD_MONITOR				0x004D	//'M'
#define CMD_SAVE				0x004D	//'Q'

// TripProc.c
#endif

