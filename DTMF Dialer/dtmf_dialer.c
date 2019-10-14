/*
 * Author:      Felipe Fortuna and Adam Macioszek
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for sin function
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

////////////////////////////////////
// Thread Structures
static struct pt pt_key;
static struct pt pt_debug;

////////////////////////////////////
// Keypad

char buffer[12]; // Stores phone number
int buff_index; // Number of items in buffer
char test_mode; // Indicates test mode 
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits; // Keypad pulldowns
// Debouncing State Machine
#define STATE_RELEASE 0
#define STATE_TPRESS 1
#define STATE_PRESS 2
#define STATE_TRELEASE 3
// some precise, fixed, short delays
#define NOP asm("nop"); // to use for extending pulse durations on the keypad if behavior is erratic
#define wait20 NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;  // 1/2 microsec
#define wait40 wait20;wait20; // one microsec

////////////////////////////////////
// DAC and DDS

// Dac Settings
#define DAC_config_chan_A 0b0011000000000000  // A-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000  // B-channel, 1x, active
volatile int spiClkDiv = 2;
// Dac Modes
#define MODE_OFF 0
#define MODE_TEST 1
#define MODE_BURST 2
#define MODE_END_BURST 3

// Sine Table and DDS variables
#define sin_table_size 256
#define Fs 17 // Sample frequency = 2^15
volatile unsigned int sin_table[sin_table_size];
volatile unsigned short dac_msg, dac_mode;
volatile short ramp;
volatile unsigned int Fout_0, phase_acc_0, phase_incr_0;
volatile unsigned int Fout_1, phase_acc_1, phase_incr_1;
volatile int keyCode, isr_time; // Global purely for use in debug
int freq_table[12][2] = {
    {941, 1336}, {697, 1209}, {697, 1336}, 
    {697, 1477}, {770, 1209}, {770, 1336},
    {770, 1477}, {852, 1209}, {852, 1336},
    {852, 1477}, {941, 1209}, {941, 1477}};
int sfreq_table[7] = {697, 770, 852, 941, 1209, 1336, 1477};

/////////////////////////////////////////////////////////////////

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    if (dac_mode == MODE_OFF)
        dac_msg = 0;
    else if (dac_mode == MODE_TEST) {
        phase_incr_0 = (Fout_0 << (32 - Fs)); // phase_increment = Fout * (2^32)/(2^Fs)
        phase_acc_0 += phase_incr_0; // DDS phase
        dac_msg = (2047 + (sin_table[phase_acc_0 >> 24]));
        
    } else {
        if (dac_mode == MODE_BURST) {
            // Handle ramp up
            if (ramp < 1024) ramp++;
                
        } else if (dac_mode == MODE_END_BURST) {
            // Handle ramp down, eventually switch to MODE_OFF
            if (ramp > 0) ramp--;
            else dac_mode = MODE_OFF;
        }   
        phase_incr_0 = (Fout_0 << (32 - Fs)); // phase_increment = Fout * (2^32)/(2^Fs)
        phase_acc_0 += phase_incr_0; // DDS phase
        phase_incr_1 = (Fout_1 << (32 - Fs));
        phase_acc_1 += phase_incr_1;
        
        dac_msg = (2047 + (sin_table[phase_acc_0 >> 24]217 >> 1) + (sin_table[phase_acc_1 >> 24] >> 1));
        dac_msg = (ramp * dac_msg)>>10; // Use top 8 bits as index, offset by 2047 so all are positive
    }   
    
    
    mPORTBClearBits(BIT_4); // cs low, start transaction
    while (TxBufFullSPI2()); // test for ready
    WriteSPI2(DAC_config_chan_A | (dac_msg & 0xfff)); // write to spi2
    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
    mPORTBSetBits(BIT_4); // cs high, end transaction
    isr_time = ReadTimer2();
    mT2ClearIntFlag();
}

// === Key Thread ===============================================
static PT_THREAD(protothread_key(struct pt *pt)){
    PT_BEGIN(pt);
    static int pattern, keypad, i, PushState, lastPush, burst_start;
    static int keytable[12] = {0x108, 0x81, 0x101, 0x201, 0x82, 0x102, 0x202, 0x84, 0x104, 0x204, 0x88, 0x208};

    // init the keypad pins A0-A3 and B7-B9
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3); // Rows are outputs
    mPORTBSetPinsDigitalIn(BIT_6 | BIT_7 | BIT_8 | BIT_9 | BIT_10); // Columns are inputs
    EnablePullDownB(BIT_7 | BIT_8 | BIT_9 | BIT_10);
    
    while(1){
        PT_YIELD_TIME_msec(30);
        
        // set test mode
        if (mPORTBReadBits(BIT_10))
            test_mode = 1;
        else
            test_mode = 0;
        
        // read each row sequentially
        mPORTAClearBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);
        pattern = 1;
        mPORTASetBits(pattern);
        for (i = 0; i < 4; i++) {
            wait40;
            keypad = mPORTBReadBits(BIT_7 | BIT_8 | BIT_9);
            if (keypad != 0) {
                keypad |= pattern;
                break;
            }
            mPORTAClearBits(pattern);
            pattern <<= 1;
            mPORTASetBits(pattern);
        }

        // search for keycode
        if (keypad > 0) { // then button is pushed
            for (i = 0; i < 12; i++) {
                if (keytable[i] == keypad) break;
            }
            // if invalid, two button push, set to -1
            if (i == 12) i = -1;
        }
        else i = -1; // no button pushed
        keyCode = i;
        
        burst_start = 0;
        switch (PushState) {
            case STATE_RELEASE:
                if (keyCode != -1) {
                    PushState = STATE_TPRESS;
                    lastPush = keyCode;
                } else PushState = STATE_RELEASE;
                break;
            case STATE_TPRESS:
                if (keyCode == lastPush) {
                    PushState = STATE_PRESS;
                    if (keyCode == 10) {
                        memset(buffer, NULL, 12);
                        buff_index = 0;
                    } 
                    else if (keyCode == 11 & !test_mode) {                      
                        burst_start = 1;
                    } 
                    else if (buff_index < 12) {
                        buffer[buff_index] = keyCode;
                        buff_index++;
                        if (!test_mode & keyCode < 10) {
                            Fout_0 = freq_table[keyCode][0];
                            Fout_1 = freq_table[keyCode][1];
                            ramp = 0;
                            dac_mode = MODE_BURST;
                            PT_YIELD_TIME_msec(75);
                            dac_mode = MODE_END_BURST;
                            PT_YIELD_TIME_msec(75);
                        }
                    }
                } else PushState = 0;
                break;
            case STATE_PRESS:
                if (keyCode != lastPush)
                    PushState = STATE_TRELEASE;
                break;
            case STATE_TRELEASE:
                if (keyCode == lastPush)
                    PushState = STATE_PRESS;
                else
                    PushState = STATE_TPRESS;
                break;
            default:
                PushState = STATE_RELEASE;
                break;
        } // end case
        
        //  Handle test mode
        if (test_mode) {
            if (keyCode > 0 & keyCode <= 7) {
                Fout_0 = sfreq_table[keyCode - 1];
                Fout_1 = 0;
                dac_mode = MODE_TEST;
            } else {
                Fout_0 = 0;
                Fout_1 = 0;
                dac_mode = MODE_OFF;
            }
        }
        
        // Handle burst mode
        if (burst_start){
            for(i=0; i < buff_index; i++){
                Fout_0 = freq_table[buffer[i]][0];
                Fout_1 = freq_table[buffer[i]][1];
                ramp = 0;
                dac_mode = MODE_BURST;
                PT_YIELD_TIME_msec(75);
                dac_mode = MODE_END_BURST;
                PT_YIELD_TIME_msec(75);
            }
        }
    }
    PT_END(pt);
}

void printplz(char* buf, int line_num, int title_len){
    tft_setCursor(20, line_num*20);
    tft_fillRoundRect(20+(title_len+3)*12, line_num*20, 220, 15, 1, ILI9340_BLACK);
    tft_writeString(buf);
}

// === Debug Thread =============================================
static PT_THREAD(protothread_debug(struct pt *pt)) {
    PT_BEGIN(pt);
    char tmp[60];
    static int j;
    tft_setTextColor(ILI9340_YELLOW);
    tft_setTextSize(2);
    while(1){
        PT_YIELD_TIME_msec(10);
        sprintf(tmp, "keyCode = %d", keyCode);
        if (keyCode == 10)sprintf(tmp, "keyCode = *");
        if (keyCode == 11)sprintf(tmp, "keyCode = #");
        printplz(tmp, 0, 7);

        sprintf(tmp, "test_mode = %d", test_mode);
        printplz(tmp, 3, 9);

        PT_YIELD_TIME_msec(10);

        sprintf(tmp, "freq = %d + %d", Fout_0, Fout_1);
        printplz(tmp, 10, 4);
             
        PT_YIELD_TIME_msec(10);
    }
    PT_END(pt);
}

void main(void) {

    // DAC Setup ===============================================================
    // Set up timer2, prescalar = 1, f_t2 = 40MHz (T_t2 = 25ns)
    // N = f_t2/Fs, how many timer ticks per interrupt thrown
    float f_t2 = 40000000.0;
    float f_sample = pow(2.0, Fs);
    int N = (int) (f_t2 / f_sample);
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, N);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();

    // SPI setup
    PPSOutput(2, RPB5, SDO2); // MOSI pin
    mPORTBSetPinsDigitalOut(BIT_4); // CS pin
    mPORTBSetBits(BIT_4);
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV, spiClkDiv);

    // Sine Lookup Table =======================================================
    // scaled to produce values between 0 and 2047
    int i;
    for (i = 0; i < sin_table_size; i++) {
        sin_table[i] = (int) ((float) 2047 * cos((float) i * 6.2831853 / (float) sin_table_size));
    }

    // Configure Threads =======================================================
    PT_setup();
    INTEnableSystemMultiVectoredInt();

    // init the threads
    PT_INIT(&pt_key);
    PT_INIT(&pt_debug);

    // init the display
    // NOTE that this init assumes SPI channel 1 connections
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK); //240x320 vertical display
    tft_setRotation(0); // Use tft_setRotation(1) for 320x240

    // round-robin scheduler for threads
    while (1) {
        PT_SCHEDULE(protothread_key(&pt_key));
        PT_SCHEDULE(protothread_debug(&pt_debug));
    }
}
