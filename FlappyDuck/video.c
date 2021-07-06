/*
 * Program Name: "Intel FPGA Duck: The Game"
 * Program Author: Christopher Johnson, Florida State University
 * Course: Embedded Microprocessor Based System Design
 * Professor: Uwe Meyer-Baese
 * Date: 11/30/2020
 * Program Description: In this video game, a player controls a duck by pressing pushbutton 0 on the DE-10 Standard board.
 *                      The goal is to avoid the obstacles in your path, and achieve the greatest distance possible.
 * Technical Description: The only input for the user is to press KEY0 on the board. The only output used is the VGA video out port.
 *                        The program utilizes an ARM A9 HPS Core of the Cyclone V SoC. It uses no HAL, but borrows functions and 
 *                        it's template from the sample programs provided with the Intel University Program. 
 * Errata: 
 * Obstacle blocks rise up on the screen over the course of the game, inexplicably
 * Collision detection becomes unreliable as the game progresses past the first few obstacles
 * Any text written to the character buffer remains after system reset or binary reupload
 * 
 * To Do:
 * Modify obstacle algorithm to create procedurally generated random obstacle sizes.
 * Ensure that no function tries to write outside the span of the pixel buffer.
 * Clear character buffer before/after runnning program
 * Allow the user to play again without restarting the program
 * Utilize the double buffering method to prepare frames while they are being displayed
*/
#include "address_map_arm.h"
#include "stdlib.h"
#include "stdio.h"

/* template function prototypes */
void video_text(int, int, char *);
void video_box(int, int, int, int, short);
int  resample_rgb(int, int);
int  get_data_bits(int);

/* program function prototypes */
void draw_bird(short ypos, short xpos, short scale, short mode);


#define STANDARD_X 320
#define STANDARD_Y 240
#define INTEL_BLUE 0x0071C5
#define COLORBL 0x0071C5
#define BLACK 0x000000
#define WHITE 0xFFFFFF
#define BROWN 0x5F5F00
#define ORANGE 0xFF0000

#define PIXEL_BACKBUF_CTRL_BASE 0xFF203024
#define PIXEL_BUF_STATUS_BASE 0xFF20302C

#define BIRD_LEN 24  //Length of bird bitmap images
#define BIRD_HIG 28  //Height of bird bitmap images

/* global variables */
int screen_x;
int screen_y;
int res_offset;
int col_offset;

//Bitmap image of duck, with wings up.
short birdup[BIRD_HIG][BIRD_LEN]=  {{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//0
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//0
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//2
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//4
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,1,1,0, 0,0,0,0},//6
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,1,4, 1,0,0,0},//8
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,3,1, 1,1,1,1, 1,2,0,0},
                                    {0,0,0,0, 0,0,0,3, 3,3,3,3, 3,3,3,1, 1,1,1,1, 1,2,2,0},//10
                                    {0,0,0,0, 3,3,3,3, 3,3,3,3, 3,3,3,1, 1,1,1,1, 1,2,2,2},
                                    {0,0,0,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 1,1,1,0, 0,0,0,0},//12
                                    {3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,1, 0,0,0,0, 0,0,0,0},
                                    {3,3,3,3, 3,3,1,3, 3,3,3,3, 3,1,1,0, 0,0,0,0, 0,0,0,0},//14
                                    {0,1,1,3, 3,1,1,3, 3,3,3,3, 3,1,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,1,1, 1,1,1,3, 3,3,3,3, 4,4,0,0, 0,0,0,0, 0,0,0,0},//16
                                    {0,0,0,1, 1,1,1,3, 3,3,3,3, 4,4,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,1, 1,1,1,3, 3,3,3,3, 4,4,0,0, 0,0,0,0, 0,0,0,0},//18
                                    {0,0,4,1, 1,0,0,3, 3,3,3,3, 4,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,1,1,1, 0,0,0,3, 3,3,3,4, 4,0,0,0, 0,0,0,0, 0,0,0,0},//20
                                    {0,1,1,0, 0,0,0,0, 3,3,3,4, 4,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 3,3,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//22
                                    {0,0,0,0, 0,0,0,0, 3,3,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 3,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//24
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//25
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}};//0

//Bitmap image of duck, with wings down.
short birddown[BIRD_HIG][BIRD_LEN]={{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//0
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//0
                                    {0,0,0,0, 0,0,0,0, 0,0,0,4, 4,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,4,1,4, 4,0,0,0, 0,0,0,0, 0,0,0,0},//2
                                    {0,0,0,0, 0,0,0,0, 0,4,1,1, 4,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 4,1,1,1, 4,0,0,0, 0,0,0,0, 0,0,0,0},//4
                                    {0,0,0,0, 0,0,0,0, 1,1,1,1, 4,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,1, 1,1,1,1, 4,0,0,0, 0,1,1,0, 0,0,0,0},//6
                                    {0,0,0,0, 0,0,0,1, 1,1,1,1, 4,0,0,0, 1,1,1,1, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,1, 1,1,1,1, 4,0,0,1, 1,1,1,4, 1,0,0,0},//8
                                    {0,0,0,0, 0,0,0,1, 1,1,1,1, 4,0,3,1, 1,1,1,1, 1,2,0,0},
                                    {0,0,0,0, 0,0,0,1, 1,1,1,3, 3,3,3,1, 1,1,1,1, 1,2,2,0},//10
                                    {0,0,0,0, 3,3,3,1, 1,1,1,3, 3,3,3,1, 1,1,1,1, 1,2,2,2},
                                    {0,0,0,3, 3,3,3,1, 1,1,1,1, 1,3,3,3, 1,1,1,0, 0,0,0,0},//12
                                    {3,3,3,3, 3,3,3,1, 1,1,1,1, 1,3,3,1, 0,0,0,0, 0,0,0,0},
                                    {3,3,3,3, 3,3,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0, 0,0,0,0},//14
                                    {0,1,1,3, 3,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,1,1, 1,1,1,1, 1,1,1,1, 1,0,0,0, 0,0,0,0, 0,0,0,0},//16
                                    {0,0,0,1, 1,1,1,1, 1,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,1, 1,1,1,1, 1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//18
                                    {0,0,4,1, 1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,1,1,1, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//20
                                    {0,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//22
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//24
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},//25
                                    {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}};//0





/*******************************************************************************
 
 ******************************************************************************/
int main(void) {
    *(int *)PIXEL_BACKBUF_CTRL_BASE=0xC0000000;
    volatile int * video_resolution = (int *)(PIXEL_BUF_CTRL_BASE + 0x8);
    screen_x                        = *video_resolution & 0xFFFF;
    screen_y                        = (*video_resolution >> 16) & 0xFFFF;

    volatile int * rgb_status = (int *)(RGB_RESAMPLER_BASE);
    int            db         = get_data_bits(*rgb_status & 0x3F);

    /* check if resolution is smaller than the standard 320 x 240 */
    res_offset = (screen_x == 160) ? 1 : 0;

    /* check if number of data bits is less than the standard 16-bits */
    col_offset = (db == 8) ? 1 : 0;

    /* create a message to be displayed on the video and LCD displays */
    char text_top_row[40]    = "Intel FPGA Duck: The Game\0";
    char text_bottom_row[40] = "Christopher Johnson: EuPSD\0";

    /* update color */
    short bg = resample_rgb(db, INTEL_BLUE);   //Backgroud color 
    short c1 = resample_rgb(db, WHITE);
    short c2 = resample_rgb(db, ORANGE);
    short c3 = resample_rgb(db, BROWN);
    short c4 = resample_rgb(db, BLACK);
    int row;
    int col;
    for (row=0; row<BIRD_HIG; row++){
        for (col=0;col<BIRD_LEN; col++){
            switch (birdup[row][col]){  //Set the color of each pixel with the resampled colors mapped to the matricies
                case 0:
                    birdup[row][col]=bg;
                    break;
                case 1: 
                    birdup[row][col]=c1;
                    break;
                case 2:
                    birdup[row][col]=c2;
                    break;
                case 3:
                    birdup[row][col]=c3;
                    break;
                case 4:
                    birdup[row][col]=c4;
                    break;
                default:
                    birdup[row][col]=0x000000;
            }
            switch (birddown[row][col]){ //Do the same for the second bird image
                case 0:
                    birddown[row][col]=bg;
                    break;
                case 1: 
                    birddown[row][col]=c1;
                    break;
                case 2:
                    birddown[row][col]=c2;
                    break;
                case 3:
                    birddown[row][col]=c3;
                    break;
                case 4:
                    birddown[row][col]=c4;
                    break;
                default:
                    birddown[row][col]=0x000000;
            }
        }
    }

    video_box(0, 0, STANDARD_X, STANDARD_Y, bg); // clear the screen, set background color
    video_text(1,1,text_top_row); //Display the Name of the Game
    video_text(1,3,text_bottom_row); //Display Authors Name

    //Game Parameters
    float bird_height=50;
    float bird_speed=0;
    float bird_accel=0.025;
    const int box1_len=30;
    const int box1_h=40;
    float box1_pos_x=320;
    float scroll_speed=0.1;
    float scroll_accel=0.0001;
    int dist_traveled=0;
    volatile int running = 1;

    volatile int * MPcore_private_timer_ptr = (int *)MPCORE_PRIV_TIMER; //pointer to A9 core timer register
    int counter= 286000;
    *(MPcore_private_timer_ptr) = counter;
    *(MPcore_private_timer_ptr+2)=0b011; //mode=1, enable=1
    while(running){
        int key0= *(int *)KEY_BASE & 0x1;
        while(*(MPcore_private_timer_ptr + 3)==0) //Poll Timer, wait interval
            ;
            
        *(MPcore_private_timer_ptr + 3)=1; //Reset timer
        dist_traveled++;
        
        if (dist_traveled>3000){
            box1_pos_x=(box1_pos_x-scroll_speed);
            scroll_speed+=scroll_accel;
        }
        if (bird_height<20){
            //bird_speed=0.1;
        }
        else if(bird_height>210){
            bird_speed=-0.75;
        }
        else if(key0){
            bird_speed=-1;
        }
        bird_speed+=bird_accel;
        bird_height=(int)(bird_height+bird_speed);
        if(bird_speed>0)
            draw_bird((int)bird_height,100,2,1); 
        else
            draw_bird((int)bird_height,100,2,0);
        video_box((int)box1_pos_x, 0,(int)box1_pos_x+box1_len, box1_h, c4); //Draw top box
        video_box((int)box1_pos_x+box1_len+1, 0, (int)box1_pos_x+box1_len+10, box1_h, bg); //Clean up background behind

        video_box((int)box1_pos_x, 240-box1_h, (int)box1_pos_x+box1_len, 240, c4); //Draw Bottom Box
        video_box((int)box1_pos_x+box1_len+1, 240-box1_h, (int)box1_pos_x+box1_len+10, 240, bg); 
        
        if (box1_pos_x<148 && box1_pos_x>(100-box1_len)){ //If box is in same x position as bird
            if (bird_height<(box1_len+5) || (bird_height+45)>(240-box1_len)){ //If box is in same y position as bird
                running=0; //End Program Upon A Collision
            }
        }
    }
    char text_loss[40] = "Sorry, You Lost!\0"; //Readout final score and message
    char text_distance[40] = "Your Score:\0";
    char text_score[40];
    sprintf(text_score,"%d\0",dist_traveled);
    video_text(32, 29, text_loss);
    video_text(32, 30, text_distance);
    video_text(32, 31, text_score);
    video_box(31 * 4, 28 * 4, 49 * 4 - 1, 34 * 4 - 1, c4);
    return 0;
}

/*******************************************************************************
 * Subroutine to send a string of text to the video monitor
 ******************************************************************************/
void video_text(int x, int y, char * text_ptr) {
    int             offset;
    volatile char * character_buffer =
        (char *)FPGA_CHAR_BASE; // video character buffer

    /* assume that the text string fits on one line */
    offset = (y << 7) + x;
    while (*(text_ptr)) {
        *(character_buffer + offset) =
            *(text_ptr); // write to the character buffer
        ++text_ptr;
        ++offset;
    }
}
/*******************************************************************************
 * Subroutine to draw bird to the video monitor. The mode parameter specifies which bitmap bird is displayed
 ******************************************************************************/
void draw_bird(short ypos, short xpos, short scale, short mode){
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int pixel_ptr, row, col;

    if (mode==0){
        for (row = 0; row < BIRD_HIG*scale; row++){
            int y=ypos+row;
            for (col = 0; col < BIRD_LEN*scale; ++col) {
                int x=xpos+col;
                pixel_ptr = pixel_buf_ptr +
                            (y << (10 - res_offset - col_offset)) + (x << 1);
                *(short *)pixel_ptr = birdup[row/scale][col/scale]; // set pixel color
            }
        }
    }
    else{
        for (row = 0; row < BIRD_HIG*scale; row++){
            int y=ypos+row;
            for (col = 0; col < BIRD_LEN*scale; col++) {
                int x=xpos+col;
                pixel_ptr = pixel_buf_ptr +
                            (y << (10 - res_offset - col_offset)) + (x << 1);
                *(short *)pixel_ptr = birddown[row/scale][col/scale]; // set pixel color
            }
        }
    }
}

/*******************************************************************************
 * Draw a filled rectangle on the video monitor
 * Takes in points assuming 320x240 resolution and adjusts based on differences
 * in resolution and color bits.
 ******************************************************************************/
void video_box(int x1, int y1, int x2, int y2, short pixel_color) {
    int pixel_buf_ptr = *(int *)PIXEL_BUF_CTRL_BASE;
    int pixel_ptr, row, col;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << (res_offset);
    x1           = x1 / x_factor;
    x2           = x2 / x_factor;
    y1           = y1 / y_factor;
    y2           = y2 / y_factor;

    /* assume that the box coordinates are valid */
    for (row = y1; row <= y2; row++)
        for (col = x1; col <= x2; ++col) {
            pixel_ptr = pixel_buf_ptr +
                        (row << (10 - res_offset - col_offset)) + (col << 1);
            *(short *)pixel_ptr = pixel_color; // set pixel color
        }
}

/********************************************************************************
 * Resamples 24-bit color to 16-bit or 8-bit color
 *******************************************************************************/
int resample_rgb(int num_bits, int color) {
    if (num_bits == 8) {
        color = (((color >> 16) & 0x000000E0) | ((color >> 11) & 0x0000001C) |
                 ((color >> 6) & 0x00000003));
        color = (color << 8) | color;
    } else if (num_bits == 16) {
        color = (((color >> 8) & 0x0000F800) | ((color >> 5) & 0x000007E0) |
                 ((color >> 3) & 0x0000001F));
    }
    return color;
}

/********************************************************************************
 * Finds the number of data bits from the mode
 *******************************************************************************/
int get_data_bits(int mode) {
    switch (mode) {
    case 0x0:
        return 1;
    case 0x7:
        return 8;
    case 0x11:
        return 8;
    case 0x12:
        return 9;
    case 0x14:
        return 16;
    case 0x17:
        return 24;
    case 0x19:
        return 30;
    case 0x31:
        return 8;
    case 0x32:
        return 12;
    case 0x33:
        return 16;
    case 0x37:
        return 32;
    case 0x39:
        return 40;
    }
}

