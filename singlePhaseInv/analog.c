//
// analog.c
// 2012.04.12
// Eunwho Power Electronics
// by Cheoung Soon Gil
// revision :
#include	<header.h>
#include	<extern.h>


// included in F2806x_Adc.c
// InitAdc();
// AdcOffsetSelfCal();

void ADC_SOC_CNF( )
{

    extern void DSP28x_usDelay(Uint32 Count);

    EALLOW;
    AdcRegs.ADCCTL1.bit.ADCREFSEL   = 0;
    AdcRegs.ADCCTL1.bit.ADCBGPWD    = 1;    // Power up band gap
    AdcRegs.ADCCTL1.bit.ADCREFPWD   = 1;    // Power up reference
    AdcRegs.ADCCTL1.bit.ADCPWDN     = 1;    // Power up rest of ADC
    AdcRegs.ADCCTL1.bit.ADCENABLE   = 1;    // Enable ADC

    DSP28x_usDelay(1000);         // Delay before converting ADC channels



    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    AdcRegs.INTSEL1N2.bit.INT1E = 1;        // enable ADC interrupt 1

    AdcRegs.INTSEL1N2.bit.INT1CONT = 0; // set ADCInterrupt 1 to auto clr
    AdcRegs.INTSEL1N2.bit.INT1SEL = 1;      // IntChSel causes ADCInterrupt 1
/*
    AdcRegs.INTSEL1N2.bit.INT2CONT = 1; // set ADCInterrupt 1 to auto clr
    AdcRegs.INTSEL1N2.bit.INT2SEL = 1;      // IntChSel causes ADCInterrupt 1

    AdcRegs.INTSEL3N4.bit.INT3CONT = 1; // set ADCInterrupt 1 to auto clr
    AdcRegs.INTSEL3N4.bit.INT3SEL = 2;      // IntChSel causes ADCInterrupt 1

    AdcRegs.INTSEL3N4.bit.INT4CONT = 1; // set ADCInterrupt 1 to auto clr
    AdcRegs.INTSEL3N4.bit.INT4SEL = 3;      // IntChSel causes ADCInterrupt 1

    AdcRegs.INTSEL5N6.bit.INT5CONT = 1; // set ADCInterrupt 1 to auto clr
    AdcRegs.INTSEL5N6.bit.INT5SEL = 4;      // IntChSel causes ADCInterrupt 1

    AdcRegs.INTSEL5N6.bit.INT6CONT = 0; // set ADCInterrupt 1 to auto clr
    AdcRegs.INTSEL5N6.bit.INT6SEL = 5;      // IntChSel causes ADCInterrupt 1
*/
    AdcRegs.ADCINTFLG.bit.ADCINT1 = 0;  // clear interrupt flag for ADCINT1
    AdcRegs.ADCINTFLG.bit.ADCINT2 = 0;  // clear interrupt flag for ADCINT1
    AdcRegs.ADCINTFLG.bit.ADCINT3 = 0;  // clear interrupt flag for ADCINT1
    AdcRegs.ADCINTFLG.bit.ADCINT4 = 0;  // clear interrupt flag for ADCINT1
    AdcRegs.ADCINTFLG.bit.ADCINT5 = 0;  // clear interrupt flag for ADCINT1

    AdcRegs.ADCSOC0CTL.bit.CHSEL= 0;        // I_u
    AdcRegs.ADCSOC1CTL.bit.CHSEL= 8;        // I_v
    AdcRegs.ADCSOC2CTL.bit.CHSEL= 1;        // Vdc
    AdcRegs.ADCSOC3CTL.bit.CHSEL= 9;        // IGBT_Temp
    AdcRegs.ADCSOC4CTL.bit.CHSEL= 3;        // Ex_sensor

    AdcRegs.ADCSOC0CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC1CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC2CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC3CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC4CTL.bit.ACQPS = 6;
    AdcRegs.ADCSOC5CTL.bit.ACQPS = 6;
    AdcRegs.ADCSAMPLEMODE.all = 0;      // Simultaneous sample mode
    EDIS;
}

#define ADC_CONST         0.00048828125   // 1/2048

__interrupt void adcIsr(void)
{
    GpioDataRegs.GPASET.bit.GPIO8 = 1;

    adcIuPhase = AdcResult.ADCRESULT0;
    adcIvPhase = AdcResult.ADCRESULT1;
    adcVdc       = AdcResult.ADCRESULT2;
    adcIgbtTemperature = AdcResult.ADCRESULT3;
    adcExSensor = AdcResult.ADCRESULT4;

    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;       //Clear ADCINT1 flag reinitialize for next SOC
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;   // Acknowledge interrupt to PIE
    GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;
}

void analog_input_proc( double * referenc)
{
	* referenc = analog_ref_a * analog_cmd_in_span1 - analog_cmd_in_zero1;
}

void analog_out_proc( )
{
}

int check_sensor_trip()
{
	int TripCode;

	Nop();

	if( ( TripCode = CheckOverCurrent()) != 0 ) return TripCode ;	// debug
	if( ( TripCode = CheckOverVolt()   ) != 0 ) return TripCode ;
	if( ( TripCode = CheckUndeVolt()   ) != 0 ) return TripCode ;	// ���������� ������ �Ѵ�. 
	return 0;
}
	
// end of file
