/*
 * File:        ece 4760 lab 3
 * Author:      Felipe Fortuna and Adam Macioszek
 * Target PIC:  PIC32MX250F128B
 */
// peripheral library includes
#include <plib.h>
#include <math.h>
#include <stdlib.h>
#include "config_1_2_3.h"
#include "pt_cornell_1_2_3.h"
#include "tft_gfx.h"
#include "tft_master.h"
// ============================================================================
// Global Variables

// Thread Structures
static struct pt pt_player, pt_game, pt_stats, pt_returnBalls;

// Fixed Point Macros
typedef signed int fix16;
#define multfix(a,b)   ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define divfix(a,b)    ((fix16)((((signed long long)(a)<<16)/(b)))) // divide two fix16
#define float2fix(a)   ((fix16)((a)*65536.0)) // 2^16
#define fix2float(a)   ((float)(a)/65536.0)
#define sqrtfix(a)     (float2fix(sqrt(fix2float(a)))) 
#define fix2int(a)     ((int)((a)>>16))
#define int2fix(a)     ((fix16)((a)<<16))
#define absfix(a)      abs(a)

// Ball Stuff
typedef struct {
    fix16 cx, cy, vx, vy;
    char collected; // If 1, ball needs to be respawned
    char hitcounter;
} ball_t;
#define NUM_BALLS 275 //190
static ball_t ball_arr[NUM_BALLS];
static int ball_dma[11*NUM_BALLS];
static int active_balls;

// Gameplay Values
#define paddle_len 20       // length of player paddle
#define game_length 30      // how long (in sec) the game runs for
static int score; // Player score
static short player; // top left pixel of paddle
static int leftover_ms; // How long the game thread yields for.
static int game_counter = 0;  // How long the game has been running for, in sec
fix16 div_table[20][20];
    
// DAC/DMA Variables
#define DAC_config 0b0011000000000000       // OR with DAC message, specifies DAC A output
#define dmaChn_win 0                            // Define DMA channel to use            
#define dmaChn_lose 2                            // Define DMA channel to use            
#define dmaChn_game 3                            // Define DMA channel to use            
#define sine_table_size 2048
static unsigned short sine_table_win[sine_table_size]; // the sine lookup table
static unsigned short sine_table_lose[sine_table_size]; // the sine lookup table
static unsigned short sine_table_game[sine_table_size]; // the sine lookup table

void tft_eraseBall(ball_t *b) {
    
    // Define boundaries
	int x0 = (fix2int(b->cx)-1 > 0) ? fix2int(b->cx)-1 : 0;
	int y0 = (fix2int(b->cy)-1 > 0) ? fix2int(b->cy)-1 : 0;
	int x1 = (x0+1 < 240) ? x0+1 : 239;
	int y1 = (y0+1 < 320) ? y0+1 : 319;
    
    // set AddrWindow
    tft_writecommand(ILI9340_CASET); // Column addr set
    tft_writedata16(x0);
    tft_writedata16(x1);
    tft_writecommand(ILI9340_PASET); // Row addr set
    tft_writedata16(y0);
    tft_writedata16(y1);
    tft_writecommand(ILI9340_RAMWR); // write to RAM
    
    // Write colors
    _dc_high();
    _cs_low(); 
	int i;
	for (i = 0; i < 4; i++)
		tft_spiwrite16(0x0000);
    _cs_high();
    
    /*
    tft_drawFastHLine(fix2int(b->cx) - 1, fix2int(b->cy) - 2, 2, 0x0000);
    tft_drawFastHLine(fix2int(b->cx) - 2, fix2int(b->cy) - 1, 4, 0x0000);
    tft_drawFastHLine(fix2int(b->cx) - 2, fix2int(b->cy)    , 4, 0x0000);
    tft_drawFastHLine(fix2int(b->cx) - 1, fix2int(b->cy) + 1, 2, 0x0000);
    */
}
    
void tft_drawBall(ball_t *b) {
    
    // Define boundaries
	int x0 = (fix2int(b->cx)-1 > 0) ? fix2int(b->cx)-1 : 0;
	int y0 = (fix2int(b->cy)-1 > 0) ? fix2int(b->cy)-1 : 0;
	int x1 = (x0+1 < 240) ? x0+1 : 239;
	int y1 = (y0+1 < 320) ? y0+1 : 319;
    
    // set AddrWindow
    tft_writecommand(ILI9340_CASET); // Column addr set
    tft_writedata16(x0);
    tft_writedata16(x1);
    tft_writecommand(ILI9340_PASET); // Row addr set
    tft_writedata16(y0);
    tft_writedata16(y1);
    tft_writecommand(ILI9340_RAMWR); // write to RAM
    
    // Write colors
    _dc_high();
    _cs_low(); 
	int i;
	for (i = 0; i < 4; i++)
		tft_spiwrite16(0xFFFF);
    _cs_high();
    /*
    tft_drawFastHLine(fix2int(b->cx) - 1, fix2int(b->cy) - 2, 2, b->color);
    tft_drawFastHLine(fix2int(b->cx) - 2, fix2int(b->cy) - 1, 4, b->color);
    tft_drawFastHLine(fix2int(b->cx) - 2, fix2int(b->cy)    , 4, b->color);
    tft_drawFastHLine(fix2int(b->cx) - 1, fix2int(b->cy) + 1, 2, b->color);
    */
}


// === Game Thread =============================================================
static PT_THREAD(protothread_game(struct pt *pt)) {

    PT_BEGIN(pt);

    // Initialize thread variables
    static fix16 dcx, dcy, dvx, dvy;
    static fix16 drag = float2fix(0.9999);


    // Initialize balls 
    srand(ReadADC10(0));
    AcquireADC10();
    active_balls = 0;
    int i;
    for (i = 0; i < NUM_BALLS; i++) {
        ball_arr[i].cy = int2fix((rand() % 100) + 5); // Initialize in top 100 pixels
        ball_arr[i].cx = int2fix((rand() % 230) + 5); // initialize somewhere on screen
        ball_arr[i].vx = int2fix((rand() % 7) - 3); // random between  -3 and 3
        ball_arr[i].vy = int2fix((rand() % 4) + 1); // random between  1 and  4 (down is +ve)
        ball_arr[i].collected = 0;
        ball_arr[i].hitcounter = 0;
        active_balls++;
    }
    
    while (1) {
        unsigned int begin_time = PT_GET_TIME();

        // Move and redraw all balls
        ball_t *ball;
        for (ball = ball_arr; ball < ball_arr + NUM_BALLS; ball++) {
            if (ball->collected == 0) {
                tft_eraseBall(ball);

                // Check if ball reached end condition
                if (ball->cy >= 20905984) { //int2fix(319) = 20840448
                    // bottom screen, point lost
                    ball->collected = 1;
                    active_balls--;
                    score--;
                    DmaChnEnable(dmaChn_lose);
                } else if (ball->cy >= 19595264 & ball->cy <= 19922944 & ball->cx >= int2fix(player - 1) & ball->cx <= int2fix(player + paddle_len + 1)) {
                    // int2fix(299), int2fix(304)
                    // player bar, point won
                    ball->collected = 1;
                    active_balls--;
                    score++;
                    DmaChnEnable(dmaChn_win);
                }
            }
            if (ball->collected == 0) {

                // Calculate wall collisions
                if (ball->cx <= 65536) { //int2fix(1)
                    // left of screen
                    ball->vx = -(ball->vx);
                    ball->cx = 131072; //int2fix(2)
                } else if (ball->cx >= 15663104) { //int2fix(239)
                    // right of screen
                    ball->vx = -(ball->vx);
                    ball->cx = 15597568; //int2fix(238)
                }
                if (ball->cy <= 65536) { //int2fix(1)
                    // top of screen
                    ball->vy = -(ball->vy);
                    ball->cy = 131072; //int2fix(2))
                } else if (ball->cy >= 15663104 & ball->cy <= 15859712 & (ball->cx <= 5242880 | ball->cx >= 10485760)) {
                    //int2fix(239), (242), (80), (160)
                    // barrier
                    ball->vy = -(ball->vy);
                    if (ball->vy > 0) ball->cy = 15925248; //int2fix(243))
                    else ball->cy = 15597568; //int2fix(238)
                }


                // Calculate ball-ball collisions
                if (ball->hitcounter <= 0) {
                    ball_t *otherBall;
                    for (otherBall = ball + 1; otherBall < ball_arr + NUM_BALLS; otherBall++) {
                        dcx = ball->cx - otherBall->cx;
                        dcy = ball->cy - otherBall->cy;

                        // If they're close together, recalculate velocities
                        if ((dcx <= 131072 & dcx >= int2fix(-2)) & (dcy <= 131072 & dcy >= int2fix(-2))) {
                            // int2fix(2), (-2), (2), (-2)
                            
                            dvx = ball->vx - otherBall->vx;
                            dvy = ball->vy - otherBall->vy;

                            
                            fix16 dotProd = multfix(dcx, dvx) + multfix(dcy, dvy);
                            fix16 dist_2 = absfix(multfix(dcx, dcx)) + absfix(multfix(dcy, dcy));
                            if (dist_2 == 0) dist_2 = int2fix(1); // Too close, distance set to 1...
                            fix16 factor = divfix(dotProd, dist_2);
                            //fix16 dotProd = multfix((dcx & (-16384)), (dvx & 0xffffc000)) + multfix((dcy & 0xffffc000), (dvy & 0xffffc000));
                            //fix16 factor = multfix(dotProd, div_table[abs(fix2int(dcx<<2))][abs(fix2int(dcy<<2))]);
                            fix16 delta_vx = multfix(-dcx, factor);
                            fix16 delta_vy = multfix(-dcy, factor);

                            ball->vx += delta_vx;
                            ball->vy += delta_vy;
                            ball->hitcounter = 2;
                            otherBall->vx -= delta_vx;
                            otherBall->vy -= delta_vy;
                            otherBall->hitcounter = 2;
                            
                        }

                    }
                } else ball->hitcounter--;

                // Move ball
                ball->vx = multfix(ball->vx, drag);
                ball->vy = multfix(ball->vy, drag);
                ball->cx += ball->vx;
                ball->cy += ball->vy;

                tft_drawBall(ball);
            }
        }

        // Draw two bars for field
        tft_drawFastHLine(0, 240, 80, ILI9340_WHITE);
        tft_drawFastHLine(160, 240, 80, ILI9340_WHITE);

        // Set frame rate to 20FPS (67ms/frame)
        leftover_ms = 67 - (PT_GET_TIME() - begin_time);
        if (leftover_ms < 0) {
            tft_fillScreen(ILI9340_GREEN);
            while (1);
        }
        else PT_YIELD_TIME_msec(leftover_ms);
    }
    PT_END(pt);
}

// === Player Thread =============================================================
static PT_THREAD(protothread_player(struct pt *pt)) {
    PT_BEGIN(pt);
    static int adc_reading;

    while (1) {
        unsigned int begin_time = PT_GET_TIME();

        // erase player paddle
        tft_drawFastHLine(player, 300, paddle_len, ILI9340_BLACK);

        // read the ADC
        adc_reading = ReadADC10(0);
        AcquireADC10(); // ADC has 10 bits of precision

        // convert to player position
        player = (float) (adc_reading * (240 - paddle_len)) / 1023.0; // Vref*adc/1023    

        // Draw player paddle
        tft_drawFastHLine(player, 300, paddle_len, ILI9340_WHITE);

        // Update player once per frame 
        PT_YIELD_TIME_msec(67 - (PT_GET_TIME() - begin_time));
    } // END WHILE(1)
    PT_END(pt);
} // player paddle thread

// === Return balls thread =====================================================
static PT_THREAD(protothread_returnBalls(struct pt *pt)) {
    PT_BEGIN(pt);
    
    while(1){
        
        unsigned int begin_time = PT_GET_TIME();
        // Resend collected balls
        int i;
        if (active_balls < NUM_BALLS){
            for (i = 0; i < NUM_BALLS; i++) {
                if (ball_arr[i].collected != 0){
                    ball_arr[i].cy = int2fix(10); // Initialize near top
                    ball_arr[i].cx = int2fix(120); // initialize on center axis
                    ball_arr[i].vx = int2fix((rand() % 7) - 3); // random between  -3 and 3
                    ball_arr[i].vy = int2fix((rand() % 4) + 1); // random between  1 and  4 (down is +ve)
                    ball_arr[i].collected = 0;
                    ball_arr[i].hitcounter = 0;
                    active_balls++;  
                }
            }
        }
        game_counter++;
        
        if (game_counter >= game_length){
            // Game over!
            
           
            tft_fillScreen(ILI9340_BLUE);
            tft_setTextColor(ILI9340_WHITE);
            tft_setTextSize(2);
            tft_setRotation(1);
            
            static char buffer[20];
            static char msg[20];
            if (score > 0) sprintf(msg, "You win!");
            else sprintf(msg, "You lose!");
            
            tft_setCursor(50, 100);
            sprintf(buffer, "score = %d", score);
            tft_writeString(buffer);
           
            tft_setTextSize(3);
            int i;
            while(1){
                DmaChnEnable(dmaChn_game);
                tft_setCursor(50, 50);
                tft_writeString(msg);
                for (i = 0; i < 3000000; i++);
                
                DmaChnDisable(dmaChn_game);
                tft_fillRect(50, 40, 180, 40, ILI9340_BLUE);
                for (i = 0; i < 3000000; i++);
            }
        }
        
        

        PT_YIELD_TIME_msec(1000 - (PT_GET_TIME() - begin_time));
    } // END WHILE(1)
    PT_END(pt);
} // return balls thread

// === Stats Thread =============================================================
static PT_THREAD(protothread_stats(struct pt *pt)) {
    PT_BEGIN(pt);

    static char buffer[60];

    while (1) {
        unsigned int begin_time = PT_GET_TIME();

        // Print stats:
        tft_setTextColor(ILI9340_WHITE);
        tft_setTextSize(1);

        tft_setCursor(0, 250);
        tft_fillRect(47, 250, 24, 7, ILI9340_BLACK); // x, y, w, h, erase last number
        sprintf(buffer, "score = %d", score);
        tft_writeString(buffer);

        tft_setCursor(0, 260);
        tft_fillRect(47, 260, 12, 7, ILI9340_BLACK); // x, y, w, h, erase last number
        sprintf(buffer, "lefts = %d", leftover_ms);
        tft_writeString(buffer);

        tft_setCursor(0, 270);
        tft_fillRect(47, 270, 18, 7, ILI9340_BLACK); // x, y, w, h, erase last number
        sprintf(buffer, "balls = %d", active_balls);
        tft_writeString(buffer);

        tft_setCursor(0, 280);
        tft_fillRect(47, 280, 18, 7, ILI9340_BLACK); // x, y, w, h, erase last number
        sprintf(buffer, "time  = %ds", game_counter);
        tft_writeString(buffer);

        // Update once per second
        // Set frame rate to 20FPS (67ms/frame)
        int wait_for = 10 - (PT_GET_TIME() - begin_time);
        if (wait_for < 0) {
            tft_fillScreen(ILI9340_BLUE);
            while (1);
        }
        PT_YIELD_TIME_msec(wait_for);
    } // END WHILE(1)
    PT_END(pt);
} // stats thread


int main(void) {
    // Setup SPI Connection
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
    PPSOutput(2, RPB5, SDO2); //MOSI is pin B5
    PPSOutput(4, RPB9, SS2); //CS is pin B10

    // Generate Sine Lookup Table
    int i;short temp;
    for (i = 0; i < sine_table_size; i++) {
            //(1-(((float) i)/sine_table_size)) * 
        temp = (short) (((float) 2047 * cos((float) i * 6.2831853 / (float) 256)) + 2047);
        sine_table_win[i] = (temp & 0xfff) | DAC_config;
        
        temp = (short) (((float) 2047 * cos((float) i * 6.2831853 / (float) 512)) + 2047);
        sine_table_lose[i] = (temp & 0xfff) | DAC_config;
        
        temp = (short) (((float) 2047 * cos((float) i * 6.2831853 / (float) 1024)) + 2047);
        sine_table_game[i] = (temp & 0xfff) | DAC_config;
    }

    // Set up timer2 and 3 (for DMA) 
    // Prescaler = 1, timer frequency = 40MHz, Interrupt period = timer_limit/time_frequency
    int timer_limit = 300; // Timer ticks until interrupt
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, timer_limit);


    // Setup DMA Transfers
    // We enable the AUTO option, to repeat same transfer
    DmaChnOpen(dmaChn_win, 2, DMA_OPEN_DEFAULT);
    // set the transfer parameters: ({dma channel}, {src addr}, {dest addr}, {src size}, {dest size}, {bytes/interrupt})
    DmaChnSetTxfer(dmaChn_win, sine_table_win, &SPI2BUF, 2 * sine_table_size, 2, 2); // 2*sine_table_size because each item is a short, made up of 2 bytes
    // set what event triggers the DMA transfer (here it's timer 2)
    DmaChnSetEventControl(dmaChn_win, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    // Setup DMA 2
    DmaChnOpen(dmaChn_lose, 2, DMA_OPEN_DEFAULT);
    DmaChnSetTxfer(dmaChn_lose, sine_table_lose, &SPI2BUF, 2 * sine_table_size, 2, 2); // 2*sine_table_size because each item is a short, made up of 2 bytes
    DmaChnSetEventControl(dmaChn_lose, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    // Setup DMA 3
    DmaChnOpen(dmaChn_game, 2, DMA_OPEN_AUTO);
    DmaChnSetTxfer(dmaChn_game, sine_table_game, &SPI2BUF, 2 * sine_table_size, 2, 2); // 2*sine_table_size because each item is a short, made up of 2 bytes
    DmaChnSetEventControl(dmaChn_game, DMA_EV_START_IRQ(_TIMER_2_IRQ));

    // Configure ADC
    CloseADC10(); // ensure the ADC is off before setting the configuration
#define PARAM1 ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //
#define PARAM2 ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
#define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2
#define PARAM4 ENABLE_AN11_ANA
#define PARAM5 SKIP_SCAN_ALL
    SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11); // configure to sample AN11 
    OpenADC10(PARAM1, PARAM2, PARAM3, PARAM4, PARAM5); // configure ADC using the parameters defined above
    EnableADC10();
    AcquireADC10();
    
    // Configure Threads
    PT_setup();
    INTEnableSystemMultiVectoredInt();
    PT_INIT(&pt_game);
    PT_INIT(&pt_player);
    PT_INIT(&pt_stats);
    PT_INIT(&pt_returnBalls);

    // Configure TFT Display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(0);

    // Schedule threads
    while (1) {
        PT_SCHEDULE(protothread_game(&pt_game));
        PT_SCHEDULE(protothread_player(&pt_player));
        PT_SCHEDULE(protothread_stats(&pt_stats));
        PT_SCHEDULE(protothread_returnBalls(&pt_returnBalls));
    }


    return 0;
}