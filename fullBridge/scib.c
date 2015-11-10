
// file name : scib.c
// date 2012.0421


#include	<header.h>
#include	<extern.h>
// include <scib,h>

// #define UARTC_BAUD_RATE			60		// 38400 PLL 5

#if USE_SCI_B

#define UARTB_BAUD_RATE			120		// 38400

#define SCIB_RX_BUF_MAX		20
#define SCIB_TX_BUF_MAX		100

int scib_rx_start_addr=0;
int scib_rx_end_addr=0;

int scib_tx_start_addr=0;
int scib_tx_end_addr=0;

char scib_rx_msg_box[SCIB_RX_BUF_MAX] = {0};
char scib_tx_msg_box[SCIB_TX_BUF_MAX] = {0};

void scib_fifo_init()
{
	ScibRegs.SCICCR.all =0x0007;   			// 1 stop bit,  No loopback
	                              			// No parity,8 char bits,
	                              			// async mode, idle-line protocol
	ScibRegs.SCICTL1.all =0x0000;  			// Disable RX ERR, SLEEP, TXWAKE
	ScibRegs.SCICTL1.bit.RXENA = 1;  		// RX, internal SCICLK, 
	ScibRegs.SCICTL1.bit.TXENA = 1;  		// RX, internal SCICLK, 

	ScibRegs.SCICTL2.bit.TXINTENA 	=1;		// debug
	ScibRegs.SCICTL2.bit.RXBKINTENA =0;
	ScibRegs.SCIHBAUD = 0x0000;
	ScibRegs.SCILBAUD = UARTB_BAUD_RATE;
	ScibRegs.SCICCR.bit.LOOPBKENA =0; 		// Enable loop back

	ScibRegs.SCIFFTX.bit.TXFFIENA = 0;		// Clear SCI Interrupt flag
	ScibRegs.SCIFFTX.bit.SCIFFENA=1;
	ScibRegs.SCIFFTX.all=0xC020;

    ScibRegs.SCIFFRX.bit.RXFFIL 		= 0x0001;	// 5
    ScibRegs.SCIFFRX.bit.RXFFIENA 		= 1;	// 1
    ScibRegs.SCIFFRX.bit.RXFFINTCLR 	= 1;	// 1
    ScibRegs.SCIFFRX.bit.RXFFINT 		= 0;	// 1
    ScibRegs.SCIFFRX.bit.RXFFST			= 0;	// 5
    ScibRegs.SCIFFRX.bit.RXFIFORESET	= 1;	// 1
    ScibRegs.SCIFFRX.bit.RXFFOVRCLR		= 1;	// 1
    ScibRegs.SCIFFRX.bit.RXFFOVF		= 0;	// 1
	ScibRegs.SCIFFCT.all				= 0;    
	ScibRegs.SCICTL1.all 				= 0x0023;     // Relinquish SCI from Reset

	ScibRegs.SCIFFTX.bit.TXFIFOXRESET=1;
	ScibRegs.SCIFFRX.bit.RXFIFORESET=1;

//	PieCtrlRegs.PIEIER8.bit.INTx5=1;     	// SCI_RX_INT_C --> PIE Group 8, INT5
//	PieCtrlRegs.PIEIER8.bit.INTx6=1;     	// SCI_TX_INT_C --> PIE Group 8, INT6
//	IER |= M_INT8;							// scib irq 

	PieCtrlRegs.PIEIER9.bit.INTx3=1;     // SCI_RX_INT_B --> PIE Group 9, INT1
	PieCtrlRegs.PIEIER9.bit.INTx4=1;     // SCI_TX_INT_B --> PIE Group 9, INT1
	IER |= M_INT9;		// scib irq 
}

void scib_tx_msg( char * st)
{
	int i =0;
	char * str;

	str = st;

	while( *str !='\0' ){		
		ScibRegs.SCITXBUF= * str++;     // Send data
		
		if(i < 16) i++;
		else 		break;
	}
}

int load_scib_tx_mail_box_char( char msg)
{
	if( msg == 0 ) return -1;

	scib_tx_msg_box[scib_tx_end_addr] = msg;

	if(scib_tx_end_addr < ( SCIB_TX_BUF_MAX-1)) scib_tx_end_addr ++;
	else										scib_tx_end_addr = 0;

	if(scib_tx_end_addr == scib_tx_start_addr){
		if(scib_tx_end_addr < (SCIB_TX_BUF_MAX-1)) scib_tx_start_addr++;
		else										scib_tx_start_addr = 0;
	}
	return 0;
}
void load_scib_tx_mail_box( char * st)
{
//	int loop_ctrl = 1;
	int loop_count; 
	char * str;

	str = st;

	ScibRegs.SCIFFTX.bit.TXFFIENA = 0;	// Clear SCI Interrupt flag
	loop_count = 0;

	while((*str != 0) && ( loop_count < 41)) {
		load_scib_tx_mail_box_char(*str++);
		loop_count ++;
	}

	ScibRegs.SCIFFTX.bit.TXFFIENA = 1;	// Clear SCI Interrupt flag
}
		
interrupt void scibTxFifoIsr(void)
{
    Uint16 i=0;

	while( scib_tx_end_addr != scib_tx_start_addr){

 		ScibRegs.SCITXBUF = scib_tx_msg_box[scib_tx_start_addr];

		if(scib_tx_start_addr < ( SCIB_TX_BUF_MAX-1)) scib_tx_start_addr ++;
		else											scib_tx_start_addr=0;

		if(scib_tx_end_addr == scib_tx_start_addr) break;

		i++;
		if( i > 15 ) break;
	}

	if(scib_tx_end_addr == scib_tx_start_addr) 

	ScibRegs.SCIFFTX.bit.TXFFIENA = 0;	// Clear SCI Interrupt flag

	ScibRegs.SCIFFTX.bit.TXFFINTCLR=1;	// Clear SCI Interrupt flag
//	PieCtrlRegs.PIEACK.all|=0x0080;     // IN8 
	PieCtrlRegs.PIEACK.all|=0x0100;     // IN9 
}

// read data format   "9:4:123:x.xxxe-x"
// write data format  "9:6:123:1.234e-3"

interrupt void scibRxFifoIsr(void)
{
	static Uint32 modebus_start_time=0;
	static int scib_rx_count=0;
	static char msg_box[17]={0};

	// 5msec �̻� �̸� start�� ����Ѵ�. 
	if( ulGetTime_mSec(modebus_start_time) > 10 ){
		modebus_start_time = ulGetNow_mSec( );
		msg_box[0] = ScibRegs.SCIRXBUF.all;	 // Read data
		scib_rx_count = 0;
		scib_rx_count++;
	}
	else if( scib_rx_count < 15 ){ 
		msg_box[scib_rx_count] = ScibRegs.SCIRXBUF.all;	 // Read data
		scib_rx_count++;
	}
	else if( scib_rx_count == 15 ){ 
		msg_box[15] = ScibRegs.SCIRXBUF.all;	 // Read data
		scib_rx_count = 0;
		scib_rx_msg_flag =1;
		strncpy( scib_rx_msg_box,msg_box,16);
	}		
	else{
		msg_box[0] = ScibRegs.SCIRXBUF.all;	 // Read data	
		scib_rx_count++;
	}

	ScibRegs.SCIFFRX.bit.RXFFOVRCLR=1;   // Clear Overflow flag
	ScibRegs.SCIFFRX.bit.RXFFINTCLR=1;   // Clear Interrupt flag

//	PieCtrlRegs.PIEACK.all|=0x0080;     // IN8 
	PieCtrlRegs.PIEACK.all|=0x0100;     // IN9 
}






// read data format   "9:4:123:x.xxxe-x"
// write data format  "9:6:123:1.234e-3"
void scib_cmd_proc( int * sci_cmd, double * sci_ref)
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

	if( scib_rx_msg_flag == 0){
		sci_watchdog_count = ulGetTime_mSec( start_count);		    
		if ( sci_watchdog_count > 5000){ 
			scib_fifo_init();
			start_count = ulGetNow_mSec( );
		}
		return;
	}

	scib_rx_msg_flag = 0;
	
	if ( scib_rx_msg_box[0] != '9') return;
	
	addr =  (scib_rx_msg_box[4]- '0')* 100 +(scib_rx_msg_box[5]- '0')*10 + (scib_rx_msg_box[6]- '0');
	scib_rx_msg_box[16]=0;
	data =  atof( & scib_rx_msg_box[8]);
	
	// regist write function decoding
	
	if( scib_rx_msg_box[2] == '6'){
		if( addr == 900 ){
			check = (int)data;
			if(check == 10){
				* sci_cmd = CMD_START;
				//* sci_ref = code_btn_start_ref;
				load_scib_tx_mail_box("UART CMD_START");		
			}
			else if( check == 20 ){
				* sci_cmd = CMD_STOP;  * sci_ref = 0.0;
				load_scib_tx_mail_box("UART CMD_STOP");		
			}
			else if( check == 30 ){
				* sci_cmd = CMD_RESET;  * sci_ref = 0.0;
				load_scib_tx_mail_box("UART CMD_RESET");		
			}
			else if( data == 40 ){
				* sci_cmd = CMD_SAVE;  * sci_ref = 0.0;
				load_scib_tx_mail_box("UART CMD_SAVE");		
			}
			else if( data == 80 ){
				* sci_cmd = CMD_NULL;  * sci_ref = 0.0;
				get_adc_offset();
			}
			else if( data == 90 ){
				* sci_cmd = CMD_NULL;  * sci_ref = 0.0;
				load_scib_tx_mail_box("EPROM initStart");		
				check = init_eprom_data();		// 0�� �ƴϸ� address value
				if( check != 0) load_scib_tx_mail_box("EEPROM init Fail");		
				else		load_scib_tx_mail_box("EPROM init OK");
			}
			else{
				load_scib_tx_mail_box("Illegal CMD data");		
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
	else if(scib_rx_msg_box[2] == '4'){
	
		if(addr == 901){	//	monitor state
			check = (int)data;
			if(check == 0){
				switch(gMachineState){
					case STATE_POWER_ON:	load_scib_tx_mail_box("[POWE_ON] "); break;		
					case STATE_READY: 		load_scib_tx_mail_box("[READY]   "); break;		
					case STATE_RUN: 		load_scib_tx_mail_box("[RUN ]    "); break;		
					case STATE_TRIP: 		load_scib_tx_mail_box("[TRIP]    "); break;		
					case STATE_INIT_RUN: 	load_scib_tx_mail_box("[INIT]    "); break;		
					case STATE_GO_STOP: 	load_scib_tx_mail_box("[GO_STOP] "); break;		
					case STATE_BREAK_OFF: load_scib_tx_mail_box("STATE_WAIT_BREAK_OFF"); break;	
					default: 				load_scib_tx_mail_box("Unknown State"); break;
				}
			}
			return;
		}
		else if(addr == 902){	//	���� ���� read
			check = (int)data;

			switch( check ){
			case 0 :
				monitor[0] = I_out;
				monitorPrint("Io=%d[A]",str,monitor[0]);
				load_scib_tx_mail_box(str);
				break;

			case 1 :
				monitor[1] = Power_out * 0.001;
				monitorPrint("Po=%d kW",str,monitor[1]);
				load_scib_tx_mail_box(str);
				break;
			case 2 :
				monitor[2] = Vout;
				monitorPrint("Vo=%d[V]",str,monitor[2]);
				load_scib_tx_mail_box(str);
				break;
			case 3 :
				monitor[3] = Vdc;
				monitorPrint("Vp=%d[V]",str,monitor[3]);
				load_scib_tx_mail_box(str);
				break;

			case 4 :
				if( onOff ){ onOff = 0;	strncpy(str,"    Power ",10);}
				else{ onOff = 1; 	strncpy(str,"   Eunwho ",10);}
				load_scib_tx_mail_box(str);
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
		//	snprintf( str,30,"\n Vdc =%10.3e \n",Vdc);	load_scib_tx_mail_box(str); 
			return;
		}
		else if(addr == 903){	//	���� ���� read // EEPROM TRIP DATA
			check = (int)data;

			if( check == 0 ){
				snprintf( str,4,"%03d:",TripInfoNow.CODE);
				load_scib_tx_mail_box(str); delay_msecs(180);

				load_scib_tx_mail_box(TripInfoNow.MSG); delay_msecs(220);
				load_scib_tx_mail_box(TripInfoNow.TIME); delay_msecs(180);

				dbtemp = TripInfoNow.VOUT;
				temp = (int)(floor(dbtemp +0.5));				
				snprintf( str,10,"Vo=%3d[A]",temp);	
				load_scib_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripInfoNow.VDC;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10," VDC =%4d",temp);	
				load_scib_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripInfoNow.CURRENT;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10,"I1  =%4d ",temp);	
				load_scib_tx_mail_box(str);	delay_msecs(180);

				dbtemp = TripInfoNow.DATA;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10," DATA=%4d",temp);	
				load_scib_tx_mail_box(str);	delay_msecs(180);
			}
			else if( check < 20){
				
		       TripData = (TRIP_INFO*)malloc(sizeof(TRIP_INFO));
		       GetTripInfo((check-10),TripData);

				snprintf( str,4,"%03d:",TripData->CODE);
				load_scib_tx_mail_box(str); delay_msecs(20);

				load_scib_tx_mail_box(TripData->MSG); delay_msecs(20);

				dbtemp = TripData->DATA;
				temp = (int)(floor(dbtemp +0.5));
				snprintf( str,10," DATA=%4d",temp);	
				load_scib_tx_mail_box(str);	delay_msecs(20);

				free(TripData);
			}
			else if( check < 30){				
		       TripData = (TRIP_INFO*)malloc(sizeof(TRIP_INFO));
		       GetTripInfo((check-20),TripData);

				load_scib_tx_mail_box(TripData->TIME); delay_msecs(20);
				load_scib_tx_mail_box(TripData->START_TIME); delay_msecs(20);

				free(TripData);
			}
			return;
		}
		else if(addr == 904){	 // DATE & TIME SET
			check = (int)data;

			switch( check ){
			case 0:
				TimeInput[0] = scib_rx_msg_box[10];
				TimeInput[1] = scib_rx_msg_box[11];
				TimeInput[2] = scib_rx_msg_box[12];
				TimeInput[3] = scib_rx_msg_box[13];
				TimeInput[4] = scib_rx_msg_box[14];
				TimeInput[5] = scib_rx_msg_box[15];
				break;
			case 1:
				TimeInput[6] = scib_rx_msg_box[10];
				TimeInput[7] = scib_rx_msg_box[11];
				TimeInput[8] = scib_rx_msg_box[12];
				TimeInput[9] = scib_rx_msg_box[13];
				TimeInput[10] = scib_rx_msg_box[14];
				TimeInput[11] = scib_rx_msg_box[15];
				delay_msecs(50);
				WriteTimeToDS1307(TimeInput);
				load_scib_tx_mail_box("Date & Time Saved");
				break;
			case 2:
		//		load_scib_tx_mail_box("WAIT FOR CLEAR DATA!");
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
			load_scib_tx_mail_box(str);
			return;
		}
		else if (( addr > 979) && ( addr < 996)){
			check = addr - 980;
			snprintf( str,19,"adc =%4d",adc_result[check]);
			load_scib_tx_mail_box(str);
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
				load_scib_tx_mail_box(str);
				 delay_msecs(10);
				break;
			case 1:
				load_scib_tx_mail_box(code_inform.disp);delay_msecs(10);
				break;
			case 2: 
				if( code_inform.type == TYPE_DOUBLE ){
					snprintf( str,20,"Data =%10.3e",code_inform.code_value.doubles);
				}
				else{
					snprintf( str,20,"Data =%10d",code_inform.code_value.ints);
				}
				load_scib_tx_mail_box(str);	delay_msecs(10);
				break;
			default:
				snprintf( str,19,"CODE=%4d",addr);
				load_scib_tx_mail_box(str);delay_msecs(10);
				load_scib_tx_mail_box(code_inform.disp);delay_msecs(10);

				if( code_inform.type == TYPE_DOUBLE )
					snprintf( str,20,"Data =%10.3e",code_inform.code_value.doubles);
				else
					snprintf( str,20,"Data =%10d",code_inform.code_value.ints);

				load_scib_tx_mail_box(str);	delay_msecs(10);				 
	
				break;
			}	
		}
		else{
			load_scib_tx_mail_box("Err Invalid Addr");delay_msecs(20);		
		}
		return;
	}
}
#else 
void load_scib_tx_mail_box( char * st){ }
void scib_cmd_proc( int * sci_cmd, double * sci_ref){ }
#endif

//==================================
// End of scib.c 
//==================================
