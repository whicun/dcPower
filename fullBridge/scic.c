
// file name : scic.c
// date 2012.0421


#include	<header.h>
#include	<extern.h>
// include <scic,h>

// #define UARTC_BAUD_RATE			60		// 38400 PLL 5

#if USE_SCI_C

#define UARTC_BAUD_RATE			120		// 38400

#define SCIC_RX_BUF_MAX		20
#define SCIC_TX_BUF_MAX		100

int scic_rx_start_addr=0;
int scic_rx_end_addr=0;

int scic_tx_start_addr=0;
int scic_tx_end_addr=0;

char scic_rx_msg_box[SCIC_RX_BUF_MAX] = {0};
char scic_tx_msg_box[SCIC_TX_BUF_MAX] = {0};

void scic_fifo_init()
{
	ScicRegs.SCICCR.all =0x0007;   			// 1 stop bit,  No loopback
	                              			// No parity,8 char bits,
	                              			// async mode, idle-line protocol
	ScicRegs.SCICTL1.all =0x0000;  			// Disable RX ERR, SLEEP, TXWAKE
	ScicRegs.SCICTL1.bit.RXENA = 1;  		// RX, internal SCICLK, 
	ScicRegs.SCICTL1.bit.TXENA = 1;  		// RX, internal SCICLK, 

	ScicRegs.SCICTL2.bit.TXINTENA 	=1;		// debug
	ScicRegs.SCICTL2.bit.RXBKINTENA =0;
	ScicRegs.SCIHBAUD = 0x0000;
	ScicRegs.SCILBAUD = UARTC_BAUD_RATE;
	ScicRegs.SCICCR.bit.LOOPBKENA =0; 		// Enable loop back

	ScicRegs.SCIFFTX.bit.TXFFIENA = 0;		// Clear SCI Interrupt flag
	ScicRegs.SCIFFTX.bit.SCIFFENA=1;
	ScicRegs.SCIFFTX.all=0xC020;

    ScicRegs.SCIFFRX.bit.RXFFIL 		= 0x0001;	// 5
    ScicRegs.SCIFFRX.bit.RXFFIENA 		= 1;	// 1
    ScicRegs.SCIFFRX.bit.RXFFINTCLR 	= 1;	// 1
    ScicRegs.SCIFFRX.bit.RXFFINT 		= 0;	// 1
    ScicRegs.SCIFFRX.bit.RXFFST			= 0;	// 5
    ScicRegs.SCIFFRX.bit.RXFIFORESET	= 1;	// 1
    ScicRegs.SCIFFRX.bit.RXFFOVRCLR		= 1;	// 1
    ScicRegs.SCIFFRX.bit.RXFFOVF		= 0;	// 1
	ScicRegs.SCIFFCT.all				= 0;    
	ScicRegs.SCICTL1.all 				= 0x0023;     // Relinquish SCI from Reset

	ScicRegs.SCIFFTX.bit.TXFIFOXRESET=1;
	ScicRegs.SCIFFRX.bit.RXFIFORESET=1;

	PieCtrlRegs.PIEIER8.bit.INTx5=1;     	// SCI_RX_INT_C --> PIE Group 8, INT5
	PieCtrlRegs.PIEIER8.bit.INTx6=1;     	// SCI_TX_INT_C --> PIE Group 8, INT6
	IER |= M_INT8;							// Scic irq 

//	PieCtrlRegs.PIEIER9.bit.INTx3=1;     // SCI_RX_INT_B --> PIE Group 9, INT1
//	PieCtrlRegs.PIEIER9.bit.INTx4=1;     // SCI_TX_INT_B --> PIE Group 9, INT1
//	IER |= M_INT9;		// Scic irq 
}

void scic_tx_msg( char * st)
{
	int i =0;
	char * str;

	str = st;

	while( *str !='\0'){		
		ScicRegs.SCITXBUF= * str++;     // Send data
		
		if(i < 16) i++;
		else 		break;
	}
}

int load_scic_tx_mail_box_char( char msg)
{
	if( msg == 0 ) return -1;

	scic_tx_msg_box[scic_tx_end_addr] = msg;

	if(scic_tx_end_addr < ( SCIC_TX_BUF_MAX-1)) scic_tx_end_addr ++;
	else										scic_tx_end_addr = 0;

	if(scic_tx_end_addr == scic_tx_start_addr){
		if(scic_tx_end_addr < (SCIC_TX_BUF_MAX-1)) scic_tx_start_addr++;
		else										scic_tx_start_addr = 0;
	}
	return 0;
}
void load_scic_tx_mail_box( char * st)
{
//	int loop_ctrl = 1;
	int loop_count; 
	char * str;

	str = st;

	ScicRegs.SCIFFTX.bit.TXFFIENA = 0;	// Clear SCI Interrupt flag
	loop_count = 0;

	while((*str != 0) && ( loop_count < 40)) {
		load_scic_tx_mail_box_char(*str++);
		loop_count ++;
	}
	ScicRegs.SCIFFTX.bit.TXFFIENA = 1;	// Clear SCI Interrupt flag
}
		
interrupt void scicTxFifoIsr(void)
{
    Uint16 i=0;

	while( scic_tx_end_addr != scic_tx_start_addr){

 		ScicRegs.SCITXBUF = scic_tx_msg_box[scic_tx_start_addr];

		if(scic_tx_start_addr < ( SCIC_TX_BUF_MAX-1)) scic_tx_start_addr ++;
		else											scic_tx_start_addr=0;

		if(scic_tx_end_addr == scic_tx_start_addr) break;

		i++;
		if( i > 15 ) break;
	}

	if(scic_tx_end_addr == scic_tx_start_addr) 

	ScicRegs.SCIFFTX.bit.TXFFIENA = 0;	// Clear SCI Interrupt flag
	ScicRegs.SCIFFTX.bit.TXFFINTCLR=1;	// Clear SCI Interrupt flag

	PieCtrlRegs.PIEACK.all|=0x0080;     // IN8 SCI-C 
//	PieCtrlRegs.PIEACK.all|=0x0100;     // IN9 SCI-B
}

// read data format   "9:4:123:x.xxxe-x"
// write data format  "9:6:123:1.234e-3"

interrupt void scicRxFifoIsr(void)
{
	static Uint32 modebus_start_time=0;
	static int scic_rx_count=0;
	static char msg_box[17]={0};

	// 5msec �̻� �̸� start�� ����Ѵ�. 
	if( ulGetTime_mSec(modebus_start_time) > 10 ){
		modebus_start_time = ulGetNow_mSec( );
		msg_box[0] = ScicRegs.SCIRXBUF.all;	 // Read data
		scic_rx_count = 0;
		scic_rx_count++;
	}
	else if( scic_rx_count < 15 ){ 
		msg_box[scic_rx_count] = ScicRegs.SCIRXBUF.all;	 // Read data
		scic_rx_count++;
	}
	else if( scic_rx_count == 15 ){ 
		msg_box[15] = ScicRegs.SCIRXBUF.all;	 // Read data
		scic_rx_count = 0;
		scic_rx_msg_flag =1;
		strncpy( scic_rx_msg_box,msg_box,16);
	}		
	else{
		msg_box[0] = ScicRegs.SCIRXBUF.all;	 // Read data	
		scic_rx_count++;
	}

	ScicRegs.SCIFFRX.bit.RXFFOVRCLR=1;   // Clear Overflow flag
	ScicRegs.SCIFFRX.bit.RXFFINTCLR=1;   // Clear Interrupt flag

	PieCtrlRegs.PIEACK.all|=0x0080;     // SCIC IN8  
//	PieCtrlRegs.PIEACK.all|=0x0100;     // SCIB IN9 
}

// read data format   "9:4:123:x.xxxe-x"
// write data format  "9:6:123:1.234e-3"
void scic_cmd_proc( int * sci_cmd, double * sci_ref)
{
	unsigned long sci_watchdog_count;
	static unsigned long start_count=0;
	static int onOff;
	double data,dbtemp;

	int addr,check,temp;
	char str[30]={0};

	TRIP_INFO * TripData;
	 
	* sci_cmd = CMD_NULL;
	* sci_ref = 0.0;

	if( scic_rx_msg_flag == 0){
		sci_watchdog_count = ulGetTime_mSec( start_count);		    
		if ( sci_watchdog_count > 5000){ 
			scic_fifo_init();
			start_count = ulGetNow_mSec( );
		}
		return;
	}

	scic_rx_msg_flag = 0;

	if ( scic_rx_msg_box[0] != '9') return;
	
	addr =  (scic_rx_msg_box[4]- '0')* 100 +(scic_rx_msg_box[5]- '0')*10 + (scic_rx_msg_box[6]- '0');
	scic_rx_msg_box[16]=0;
	data =  atof( & scic_rx_msg_box[8]);
	
	// regist write function decoding
	
	if( scic_rx_msg_box[2] == '6'){
		if( addr == 900 ){
			check = (int)data;
			if(check == 10){
				* sci_cmd = CMD_START;
				// * sci_ref = code_btn_start_ref;
				load_scic_tx_mail_box("UART CMD_START");		
			}
			else if( check == 20 ){
				* sci_cmd = CMD_STOP;  * sci_ref = 0.0;
				load_scic_tx_mail_box("UART CMD_STOP");		
			}
			else if( check == 30 ){
				* sci_cmd = CMD_RESET;  * sci_ref = 0.0;
				load_scic_tx_mail_box("UART CMD_RESET");		
			}
			else if( data == 40 ){
				* sci_cmd = CMD_SAVE;  * sci_ref = 0.0;
				load_scic_tx_mail_box("UART CMD_SAVE");		
			}
			else if( data == 50 ){
				* sci_cmd = CMD_READ_ALL;  * sci_ref = 0.0;
				load_scic_tx_mail_box("UART CMD_READ_ALL");		
			}
			else if( data == 80 ){
				* sci_cmd = CMD_NULL;  * sci_ref = 0.0;
				get_adc_offset();
			}
			else if( data == 90 ){
				* sci_cmd = CMD_NULL;  * sci_ref = 0.0;
				load_scic_tx_mail_box("EEPROM init Start");		
				check = init_eprom_data();		// 0�� �ƴϸ� address value
				if( check != 0) load_scic_tx_mail_box("EEPROM init Fail");		
				else		load_scic_tx_mail_box("EEPROM init OK");
			}
			else{
				load_scic_tx_mail_box("Illegal CMD data");		
			}
		}
		else{
			// registor_write_proc(addr,data);
			check = SaveDataProc(addr, data);
			Nop();
		}
	}

//==================
//   read routine
//====================
	else if(scic_rx_msg_box[2] == '4'){
	
		if(addr == 901){	//	monitor state
			check = (int)data;
			if(check == 0){
				switch(gMachineState){
					case STATE_POWER_ON:	load_scic_tx_mail_box("[POWE_ON] "); break;		
					case STATE_READY: 		load_scic_tx_mail_box("[READY]   "); break;		
					case STATE_RUN: 		load_scic_tx_mail_box("[RUN ]    "); break;		
					case STATE_TRIP: 		load_scic_tx_mail_box("[TRIP]    "); break;		
					case STATE_INIT_RUN: 	load_scic_tx_mail_box("[INIT]    "); break;		
					case STATE_GO_STOP: 	load_scic_tx_mail_box("[GO_STOP] "); break;		
					case STATE_BREAK_OFF: load_scic_tx_mail_box("STATE_WAIT_BREAK_OFF"); break;	
					default: 				load_scic_tx_mail_box("Unknown State"); break;
				}
			}
			return;
		}

//--- HMI monitor 

		else if(addr == 902){	//	���� ���� read
			check = (int)data;

			switch( check ){
			case 0 :
				monitor[0] = I_out;
				monitorPrint("Io=%d[A]",str,monitor[0]);
				load_scic_tx_mail_box(str);
				break;

			case 1 :
				monitor[1] = Power_out;
				monitorPrint("Po=%d kW",str,monitor[1]);
				load_scic_tx_mail_box(str);
				break;
			case 2 :
				monitor[2] = Vout;
				monitorPrint("Vo=%d[V]",str,monitor[2]);
				load_scic_tx_mail_box(str);
				break;
			case 3 :
				monitor[3] = Vdc;
				monitorPrint("Vp=%d[V]",str,monitor[3]);
				load_scic_tx_mail_box(str);
				break;

			case 4 :
				if( onOff ){ onOff = 0;	strncpy(str,"    Power ",10);}
				else{ onOff = 1; 	strncpy(str,"   Eunwho ",10);}
				load_scic_tx_mail_box(str);
				break;
			case 5 : // Reset;
				gMachineState = STATE_POWER_ON;
				Nop();
				asm (" .ref _c_int00"); // ;Branch to start of boot.asm in RTS library
				asm (" LB _c_int00"); // ;Branch to start of boot.asm in RTS library
				break;
			default:
				break;
			}
			// check �� �� ������ �����ͷ� �ξ ó�� �Ѵ�. 
		//	snprintf( str,30,"\n Vdc =%10.3e \n",Vdc);	load_scic_tx_mail_box(str); 
			return;
		}
		else if(addr == 903){	//	���� ���� read // EEPROM TRIP DATA
			check = (int)data;

			if( data == 0 ){
				snprintf( str,4,"%03d:",TripInfoNow.CODE);
				load_scic_tx_mail_box(str); delay_msecs(180);

				load_scic_tx_mail_box(TripInfoNow.MSG); delay_msecs(220);
				load_scic_tx_mail_box(TripInfoNow.TIME); delay_msecs(180);

				dbtemp = TripInfoNow.VOUT;
				temp = (int)(floor(dbtemp +0.5));				
				snprintf( str,20,"Vo=%3d[A]",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripInfoNow.VDC;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,20," VDC =%4d",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripInfoNow.CURRENT;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10,"I1  =%4d ",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripInfoNow.DATA;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10," DATA=%4d",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);
			}
			else{

		       TripData = (TRIP_INFO*)malloc(sizeof(TRIP_INFO));
		       GetTripInfo(check + 1,TripData);

				strncpy(gStr1,TripInfoNow.MSG,20);
			
				snprintf( str,4,"%03d:",TripData->CODE);
				load_scic_tx_mail_box(str); delay_msecs(180);

				load_scic_tx_mail_box(TripData->MSG); delay_msecs(220);
				load_scic_tx_mail_box(TripData->TIME); delay_msecs(180);

				dbtemp = TripData->VOUT;
				temp = (int)(floor(dbtemp +0.5));				
				snprintf( str,10,"Vo=%3d[A]",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripData->VDC;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10," VDC =%4d",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripData->CURRENT;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10,"I1  =%4d ",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripData->DATA;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10," DATA=%4d",temp);	
				load_scic_tx_mail_box(str);	delay_msecs(180);

				free(TripData);
			}
			return;
		}
		else if(addr == 904){	 // DATE & TIME SET
			check = (int)data;

			switch( check ){
			case 0:
				TimeInput[0] = scic_rx_msg_box[10];
				TimeInput[1] = scic_rx_msg_box[11];
				TimeInput[2] = scic_rx_msg_box[12];
				TimeInput[3] = scic_rx_msg_box[13];
				TimeInput[4] = scic_rx_msg_box[14];
				TimeInput[5] = scic_rx_msg_box[15];
				break;
			case 1:
				TimeInput[6] = scic_rx_msg_box[10];
				TimeInput[7] = scic_rx_msg_box[11];
				TimeInput[8] = scic_rx_msg_box[12];
				TimeInput[9] = scic_rx_msg_box[13];
				TimeInput[10] = scic_rx_msg_box[14];
				TimeInput[11] = scic_rx_msg_box[15];
				delay_msecs(50);
				WriteTimeToDS1307(TimeInput);
				load_scic_tx_mail_box("Date & Time Saved");
				break;
			case 2:
		//		load_scic_tx_mail_box("WAIT FOR CLEAR DATA!");
		//		delay_msecs(180);
				ClearTripDataToEeprom();
				break;
			}
			return;
		}
		else if(addr == 905){	// RUN & STOP
			check = (int)data;
				
			switch( check ){
			case 0:
				* sci_cmd = CMD_START;
				// * sci_ref = code_btn_start_ref;
				break;
			case 1:
				* sci_cmd = CMD_STOP;
				* sci_ref = 0.0;
				break;
			case 2:
				* sci_cmd = CMD_SPEED_UP;
				break;
			case 3:
				* sci_cmd = CMD_SPEED_DOWN;
				break;
			default:
				* sci_cmd = CMD_NULL;
				break;
			}
			return;
		}
		else if(addr == 906){
			GetTimeAndDateStr(str);
			load_scic_tx_mail_box(str);
			return;
		}
		else if (( addr > 979) && ( addr < 996)){
			check = addr - 980;
			snprintf( str,19,"adc =%4d",adc_result[check]);
			load_scic_tx_mail_box(str);
			delay_msecs(10);
			return;
		}

		check = get_code_information( addr, CMD_READ_DATA , & code_inform);
	
		if( check == 0 ){
			check = (int)data;

			switch(check)
			{
			case 0:
				snprintf( str,19,"CODE=%4d",addr);
				load_scic_tx_mail_box(str);
				 delay_msecs(10);
				break;
			case 1:
				load_scic_tx_mail_box(code_inform.disp);delay_msecs(10);
				break;
			case 2: 
				if( code_inform.type == TYPE_DOUBLE ){
					snprintf( str,20,"Data =%10.3e",code_inform.code_value.doubles);
				}
				else{
					snprintf( str,20,"Data =%10d",code_inform.code_value.ints);
				}
				load_scic_tx_mail_box(str);	delay_msecs(10);
				break;
			default:
				snprintf( str,19,"CODE=%4d",addr);
				load_scic_tx_mail_box(str);delay_msecs(10);
				load_scic_tx_mail_box(code_inform.disp);delay_msecs(10);

				if( code_inform.type == TYPE_DOUBLE )
					snprintf( str,20,"Data =%10.3e",code_inform.code_value.doubles);
				else
					snprintf( str,20,"Data =%10d",code_inform.code_value.ints);

				load_scic_tx_mail_box(str);	delay_msecs(10);				 
	
				break;
			}	
		}
		else{
			load_scic_tx_mail_box("Err Invalid Addr");delay_msecs(10);		
		}
		return;
	}
}
#else 
void load_scic_tx_mail_box( char * st){ }
void scic_cmd_proc( int * sci_cmd, double * sci_ref){ }
#endif

//==================================
// End of scic.c 
//==================================
