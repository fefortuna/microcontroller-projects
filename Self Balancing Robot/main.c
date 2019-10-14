/*
 * File:        Lab 4
 * Author:      Adam Macioszek Felipe Fortuna
 * Target PIC:  PIC32MX250F128B
 * 
 */

////////////////////////////////////
// includes
#include "config_1_2_3.h"
#include "pt_cornell_1_2_3.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include "math.h"


// === global variables =======================================================
// thread structures
static struct pt pt_input, pt_auto, pt_time ;

static int sys_time;
// PWM parameters:
int generate_period = 5000 ;   // period = generate_period * 0.2us   (5000 -> 1msec)
int pwm_on_time = 1000;        // Duty cycle = pwm_on_time / generate_period
static int adc_reading, adc5;
// Dac Settings
#define DAC_config_chan_A 0b0011000000000000  // A-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000  // B-channel, 1x, active
static int desired_angle =750 , prev_angle;
static int angle[5]={0,0,0,0,0};
static int angle_idx = 0;
static float V_control = 0 ; 
static float Kp = 200.0 ; //control loop gain
static float Kd = 18000.0 ; //control loop derivitive gain
static float Ki = 0.115 ; //control loop integral gain
// 245, 16,000, .1
static int integral_cntl = 0; 
 static int last_error, error_angle;
 static int angle_read;
 static int motor_disp;
 int sign(int x) {
    return (x > 0) - (x < 0);
}
 
 //============================================
void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
   
    angle_idx++;
    if (angle_idx == 5) angle_idx = 0;
    last_error = error_angle;
    angle[angle_idx] = ReadADC10(1);
    if (angle_idx == 4) prev_angle = (angle[4] + angle[0]) >> 1;
    else prev_angle = (angle[angle_idx] + angle[angle_idx + 1]) >> 1;
    
    AcquireADC10();
    error_angle = desired_angle - prev_angle+7;
    
    
    float proportional_cntl = Kp * error_angle ;
    float differential_cntl = Kd * (error_angle - last_error);   
    //if (sign(error_angle) != sign(last_error))
      //  integral_cntl = 0;
    
    integral_cntl = integral_cntl + error_angle ;
    V_control =  proportional_cntl + differential_cntl + (Ki * integral_cntl) ;
    if (V_control < 0 )
        V_control = 0;
    if (V_control>39999)
        V_control= 39999 ;
    //V_control = ;
    int on_time = generate_period * (V_control/39999);
    SetDCOC3PWM(on_time); 

    

    // Run PID control loop using above angle
    
    // WRite DAC outputs
    if (prev_angle < 20) angle_read = (int) (prev_angle + 1024 - 760) * (90.0/271.0) + 2048; // wrap aroung
    else angle_read =  (int)(( prev_angle-760)*(2048.0/271.0)) + 2048;
    //angle_read = (int) (((float) angle_read/180.0) * 4096);
    motor_disp =(int)( motor_disp + (( (int) V_control - motor_disp>>4)));
    //motor_disp =(int) V_control + ((on_time - (int) V_control)>>4);
    //motor_disp = (int) (((float) motor_disp/4.5) * 4096);
    
    
    // Writes beam angle to channel A of SPI DAC to display angle as waveform
    mPORTBClearBits(BIT_4); // cs low
    while (TxBufFullSPI2());
    WriteSPI2(DAC_config_chan_A | (angle_read & 0xfff));
    while (SPI2STATbits.SPIBUSY);
    mPORTBSetBits(BIT_4); // cs high 
    
    // Writes motor signal to channel B of SPI DAC to display control signal as waveform
     
     mPORTBClearBits(BIT_4); // cs low
    while (TxBufFullSPI2());
    WriteSPI2(DAC_config_chan_B |  (((int) ( motor_disp* 4096.0/40000.0)) & 0xfff));
    while (SPI2STATbits.SPIBUSY);
    mPORTBSetBits(BIT_4); // cs high 
    
    // clear the timer interrupt flag
    mT2ClearIntFlag();
} // timer 2 ISR

// === One second Thread ======================================================
// update a 1 second tick counter
static PT_THREAD (protothread_time(struct pt *pt))
{
    PT_BEGIN(pt);

      while(1) {
            // yield time 1 second
            PT_YIELD_TIME_msec(1000) ;
            sys_time++ ;
            // NEVER exit while
      } // END WHILE(1)

  PT_END(pt);
} // thread 4
// === Auto Thread ======================================================
// 
static PT_THREAD (protothread_auto(struct pt *pt))
{
    static char buffer[60];
    PT_BEGIN(pt);
    while(1) {
        PT_YIELD_TIME_msec(100) ;
        if(sys_time >=0 && sys_time <5){
            desired_angle = 760;
        }
        else if(sys_time >=5 && sys_time <10)
            desired_angle = 852;
        else if (sys_time >=10 && sys_time <15)
            desired_angle = 667;
        else desired_angle = 760;
        
        // code
    } // END WHILE(1)

    PT_END(pt);
} // auto_run thread





// === User Input Thread ======================================================
// Takes user input to set up PID parameters
static PT_THREAD (protothread_input(struct pt *pt))
{
    static char buffer[60];
    static char set_this = 0; // [0 = kp, 1 = ki, 2 = kd, 3 = desired angle, 4 = none]
    static char decoup0 = 1;
    static char set_allowed = 0;
    static char decoup1 = 1;
    
    tft_setTextColor(ILI9340_WHITE);
    tft_setTextSize(2);
    
    PT_BEGIN(pt);
    while(1) {
        PT_YIELD_TIME_msec(100);
        
        // Switch which value is being set
        if (decoup0 && mPORTBReadBits(BIT_7)){
            set_this += 1;
            if (set_this == 5) set_this = 0;
            decoup0 = 0;
        }
        else if (!mPORTBReadBits(BIT_7))
            decoup0 = 1;
        if (decoup1 && mPORTBReadBits(BIT_8)){
            set_allowed = set_allowed ^ 1;
            decoup1 = 0;
        }
        else if (!mPORTBReadBits(BIT_8))
            decoup1 = 1;
        
        // Read ADC
        int potVal = ReadADC10(0);
        
        
        AcquireADC10();
        
        
        // Set change
        
        if (set_allowed){
            switch(set_this){
                case 0:
                    Kp = 500 * (potVal/1024.0);
                    break;
                case 1:
                    Kd = 20000 * (potVal/1024.0);
                    break;
                case 2:
                    Ki = (potVal/1024.0);
                    break;
                case 3:
                    desired_angle = 480 + (493 * (potVal/1023.0));
                    break;
                case 4:
                default:
                    break;
            }
        }     
        
               
        
        // Print stats:
        tft_setCursor(20, 30);
        tft_fillRect(90, 30, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        if (set_this == 0) tft_fillRect(0, 30, 10, 10, ILI9340_RED);
        else               tft_fillRect(0, 30, 10, 10, ILI9340_BLACK);
        sprintf(buffer, "kP   = %.2f", Kp);
        tft_writeString(buffer);
        
        tft_setCursor(20, 50);
        tft_fillRect(90, 50, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        if (set_this == 1) tft_fillRect(0, 50, 10, 10, ILI9340_RED);
        else               tft_fillRect(0, 50, 10, 10, ILI9340_BLACK);
        sprintf(buffer, "kD   = %.2f", Kd);
        tft_writeString(buffer);
        
        tft_setCursor(20, 70);
        tft_fillRect(90, 70, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        if (set_this == 2) tft_fillRect(0, 70, 10, 10, ILI9340_RED);
        else               tft_fillRect(0, 70, 10, 10, ILI9340_BLACK);
        sprintf(buffer, "kI   = %.2f", Ki);
        tft_writeString(buffer);
        
        tft_setCursor(20, 90);
        tft_fillRect(90, 90, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        
        
        if (set_this == 3) tft_fillRect(0, 90, 10, 10, ILI9340_RED);
        else               tft_fillRect(0, 90, 10, 10, ILI9340_BLACK);
        int desired_print;
        if (desired_angle < 20) desired_print = (int) (((float) desired_angle + 1024 - 760) * (90.0/271.0)); // wrap aroung
        else desired_print = (int) (((float) desired_angle - 760) * (90.0/271.0)); 
        sprintf(buffer, "goal = %d", desired_print);
        tft_writeString(buffer);
        
        if (set_allowed) tft_fillRect(100, 300, 10, 10, ILI9340_GREEN);
        else             tft_fillRect(100, 300, 10, 10, ILI9340_BLACK);
        
        
        tft_setCursor(0, 110);
        tft_fillRect(90, 110, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        int prev_print;
        if (prev_angle < 20) prev_print = (int) (((float) prev_angle + 1024 - 760) * (90.0/271.0)); // wrap aroung
        else prev_print = (int) (((float) prev_angle - 760) * (90.0/271.0)); 
        sprintf(buffer, "angle = %d", prev_print);
        tft_writeString(buffer);
        
        tft_setCursor(0, 150);
        tft_fillRect(70, 150, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        sprintf(buffer, "vcon = %.02f", V_control);
        tft_writeString(buffer);
        
        tft_setCursor(0, 190);
        tft_fillRect(70, 190, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        sprintf(buffer, "dacA = %d", angle_read);
        tft_writeString(buffer);
        tft_setCursor(0, 210);
        tft_fillRect(70, 210, 140, 14, ILI9340_BLUE); // x, y, w, h, erase last number
        sprintf(buffer, "dacB = %d", ((int) ( motor_disp* 4096.0/40000.0)) & 0xfff);
        tft_writeString(buffer);
        // code
    } // END WHILE(1)

    PT_END(pt);
} // user input thread


// === Main  ===================================================================
int main(void)
{
    // === Config PWM timer and output compare ========
    // set up timer2 at 1kHz to generate the wave period
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_8, generate_period);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();

    
  
    // set up compare3 for PWM mode
    OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time, pwm_on_time); //
    // OC3 is PPS group 4, map to RPB9 (pin 18)
    PPSOutput(4, RPB9, OC3);


  // === DAC SPI Setup =========================================================
    PPSOutput(2, RPB5, SDO2); // MOSI pin
    mPORTBSetPinsDigitalOut(BIT_4); // CS pin
    mPORTBSetBits(BIT_4);
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV, 2);
    
  // === Configure ADC =========================================================
    ANSELA = 0; ANSELB = 0; 
   CloseADC10(); // ensure the ADC is off before setting the configuration
   #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON //
   #define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_2 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_15 | ADC_CONV_CLK_Tcy 
    #define PARAM4	ENABLE_AN11_ANA | ENABLE_AN5_ANA // 
    #define PARAM5	SKIP_SCAN_AN0 | SKIP_SCAN_AN1 | SKIP_SCAN_AN2 | SKIP_SCAN_AN3 | SKIP_SCAN_AN4 | SKIP_SCAN_AN6 | SKIP_SCAN_AN7 | SKIP_SCAN_AN8 | SKIP_SCAN_AN9 | SKIP_SCAN_AN10 | SKIP_SCAN_AN12 | SKIP_SCAN_AN13 | SKIP_SCAN_AN14 | SKIP_SCAN_AN15
   SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF); // 
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above

	EnableADC10(); // Enable the ADC

  // === set up i/o port pin ===============================
  mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 );    // Two buttons

  // === configure threads =================================
  PT_setup();
  INTEnableSystemMultiVectoredInt();
  
  // Configure TFT Display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(0);
    
  // init threads
  PT_INIT(&pt_input);
  PT_INIT(&pt_auto);
  PT_INIT(&pt_time);
  while(1) {
      PT_SCHEDULE(protothread_input(&pt_input));
      PT_SCHEDULE(protothread_auto(&pt_auto));
      PT_SCHEDULE(protothread_time(&pt_time));
  }
} // main