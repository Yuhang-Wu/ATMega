#include "system.h"
#include "pacer.h"
#include "tinygl.h"
#include "ir_uart.h"
#include "navswitch.h"
#include "../fonts/font5x7_1.h"

//#include "gameReady.h"
//#include "gameStart.h"

#define LOOP_RATE 10000
#define MESSAGE_RATE 10
#define NAVSWITCH_RATE 20
#define DROPING_RATE 20
#define LAUNCH_BARRIER 'L'

/* Identify the players */
uint8_t player1 = 0;
uint8_t player2 = 0;

uint8_t curNormal = 1;
uint8_t nextNormal = 1;


#define CONFORM 'Y'
#include "ledmat.h"

/**
 * Wait for players ready and define who is barriers sender
 */
void gameReady(void)
{
    /* Initialize environment */
    system_init();
    ir_uart_init();
    navswitch_init();
    pacer_init(LOOP_RATE);

    tinygl_init(LOOP_RATE);
    tinygl_font_set(&font5x7_1);
    tinygl_text_speed_set(MESSAGE_RATE);
    tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);
    tinygl_text("Ready? ");

    /* Players conformation on a NAVSWITCH_PUSH event. */
    while (!player1 && !player2) {
        pacer_wait();
        tinygl_update();
        navswitch_update();

        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
            ir_uart_putc(CONFORM);
            player1 = !player1;
        }

        if (ir_uart_read_ready_p()) {
            if (ir_uart_getc() == CONFORM) {
                player2 = !player2;
            }
        }
        ledmat_init();
    }
}


/**
 * If not gameover, run the game
 */

#include <stdio.h>
#include <stdlib.h>

/* Preset the shapes of the block */

const uint8_t normalShape[14][14] = {
    {4, 3, 3, 4, 3, 3, 0, 2, 1, 3, 2, 1, 3, 3}, // < shape
    {4, 3, 2, 4, 3, 2, 0, 1, 2, 1, 1, 2, 1, 2}, // ^ shape
    {3, 3, 2, 3, 3, 2, 0, 1, 3, 2, 1, 3, 2, 3}, // > shape
    {4, 3, 2, 4, 3, 2, 0, 2, 1, 2, 2, 1, 2, 2}, // v shape
    {3, 3, 3, 3, 3, 3, 0, 1, 2, 1, 2, 1, 2, 2}, // | shape
    {3, 2, 3, 2, 3, 2, 0, 1, 2, 1, 2, 1, 2, 2}, // / shape 2 dots
    {3, 4, 3, 4, 3, 4, 0, 1, 2, 1, 2, 1, 2, 2}, // \ shape 2 dots
    {3, 2, 3, 2, 3, 2, 0, 1, 1, 1, 1, 1, 1, 2}, // - shape 2 dots
	{4, 3, 3, 3, 2, 0, 0, 1, 1, 2, 3, 1, 0, 3}, // updown T shape
    {4, 3, 3, 3, 2, 0, 0, 3, 1, 2, 3, 3, 0, 3}, // T shape
    {4, 3, 2, 4, 3, 2, 0, 1, 2, 3, 1, 2, 3, 3}, // / shapw 3 dots
    {4, 3, 2, 4, 3, 2, 0, 3, 2, 1, 3, 2, 1, 3}, // \ shape 3 dots
    {4, 3, 3, 3, 2, 0, 0, 3, 1, 2, 3, 1, 0, 3}, // z shape
    {4, 3, 3, 3, 2, 0, 0, 1, 1, 2, 3, 3, 0, 3}, //inverse z shape
};

const uint8_t dots[2][14] = {
    {1, 2, 2, 3, 4, 4, 5, 2, 1, 3, 4, 2, 3, 4},
    {1, 2, 2, 3, 3, 4, 5, 2, 1, 4, 2, 3, 1, 4}
};

/*
 * Select the shape and fix location of barriers that sent to Player2
 * If the Player2 has already dodged normalShape barriers,
 * automaticly sends the complex dot barriers to Player2 
 */
void selectShape(tinygl_point_t pointSet[7], uint8_t randIndex, uint8_t curIndex)
{

    //if (curNormal) {
        curIndex = randIndex % 14;
        //curNormal = 1;
   // } else {
       // nextNormal = 1;
       // curNormal = 0;
       // curIndex = randIndex % 2;
    //}
    
    static uint8_t i = 0;
    for (i = 0; i < 7; i++) {
        pointSet[i].x = normalShape[curIndex][i];
        pointSet[i].y = normalShape[curIndex][i + 7];
    }
}

/* Make sure the whole parts of barrier is inside the LED matrix */
uint8_t insideMatrix(tinygl_point_t pointSet[7], tinygl_point_t move) 
{
    uint8_t isInside = 0;
    static uint8_t i = 0;
    for (i = 0; i < 7; i++) {
        if (pointSet[i].x + move.x < 6 && pointSet[i].x + move.x > 0) {
            isInside = 1;
        }
    }
    return isInside;
}


/* Configure whether the droping barrier is reach the bottom in Player2 */
uint8_t reachBottomLine(tinygl_point_t pointSet[7], tinygl_point_t drop)
{
	uint8_t reachBottom = 0;
	if (pointSet[6].y + drop.y < 1) {
		reachBottom = !reachBottom;
	}
	
	return reachBottom;
}

/* Display the barriers shape on Player1's LED matrix */
void desplayShape(tinygl_point_t pointSet[7], tinygl_point_t point,
                  tinygl_point_t move)
{
	static uint8_t i = 0;
    for (i = 0; i < 7; i++) {
        point.x = pointSet[i].x + move.x;
        point.y = pointSet[i].y + move.y;
        tinygl_draw_point(point, 1);
    }
}


/* Player1 launch barriers on NAVSWITCH_PUSH or NAVSWITCH_NORTH events */
void launching(uint16_t navswitch_tick, uint8_t isLaunched) 
{	
	// Get an other shape of barriers
	if(navswitch_tick >= LOOP_RATE / NAVSWITCH_RATE) {
		navswitch_tick = 0;
		navswitch_update();
		if(navswitch_push_event_p(NAVSWITCH_PUSH)) {
			selectShape(pointSet, rand(), curIndex);
			desplayShape(pointSet, point, move);
		}
	}
	
	// Shift barrier to the right by one column 
	if (navswitch_push_event_p(NAVSWITCH_WEST)) {
		if (insideMatrix(pointSet, move)) {
			move.x++;
			desplayShape(pointSet, point, move);
		}
	}
	
	// Shift barrier to the left by one column	
	else if (navswitch_push_event_p(NAVSWITCH_EAST)) {
		if (insideMatrix(pointSet, move)) {
			move.x--;
			desplayShape(pointSet, point, move);
		}
	}
	
	// Launching the barrier to the Player2
	else if (navswitch_push_event_p(NAVSWITCH_PUSH)
			|| navswitch_push_event_p(NAVSWITCH_NORTH)) {
	
		ir_uart_putc(LAUNCH_BARRIER);
	}
	
	// Player2 get the obstacles from Player1
	if (ir_uart_read_ready_p ()) {
		if (ir_uart_getc() == LAUNCH_BARRIER) {
			isLaunched = !isLaunched;
		}
	}
}


/* Main body */
int main(void)
{
    // Initialize Environment
    system_init();
    ir_uart_init();
    navswitch_init();
    pacer_init(LOOP_RATE);

    tinygl_init(LOOP_RATE);
    tinygl_font_set(&font5x7_1);
    tinygl_text_speed_set(MESSAGE_RATE);
    tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);
    
    //Prepare the game
    gameReady();
    
    /* Count to three seconds? */
    //timeForStart();
    
    
    // Game Start
    tinygl_point_t move; // check the navswitch of Player1
    tinygl_point_t drop; // barrier sliding on Player2 
	tinygl_point_t point; 
	tinygl_point_t player2Dot;  // Player2
	tinygl_point_t pointSet[7]; // temp save the selected barrier
	
	uint8_t curIndex = 0;
    uint8_t gameIsOver = 0;
    uint16_t navswitch_tick = 0;
    uint16_t barrier_tick = 0;
    uint8_t isEnd = 0;
    
    // Display a barrier once configure the Player1
    if(player1) {
		selectShape(pointSet, rand(), curIndex);
		desplayShape(pointSet, point, move);
	}
	
	// Display the predetermined Player2
	if(player2) {
		player2Dot.x = 3;
		player2Dot.y = 1;
	}
	
	// Run the game until gameover
    while (!gameIsOver) {

		uint8_t isLaunched = 0;
		
		// Player1 sends a selected barrier to the Player2
		if (player1 && !isLanched) {
			
			pacer_wait();
			tinygl_clear();
			navswitch_tick++;
			launching(navswitch_tick, isLaunched);
			tinygl_update();
		}
		
		// Player2 dodages the barrier from Player1
		if(player2 && isLaunched) {
			
			pacer_wait();
			tinygl_clear();
			barrier_tick++;
			
			// Barrier sliding and disparing
			if(barrier_tick >= LOOP_RATE / DROPING_RATE) {
				barrier_tick = 0;
				if (!reachBottomLine(pointSet)) {
					drop.y--;
					desplayShape(pointSet, player2Dot, drop);
				}
			}
			
			
			/*
			 * Control the dot to avoid the barriers and go head.
			 * If the dot gets the top line, gameover, player2 WIN
			 */
			 //dotControl(gameIsOver);
			
			
			/* Check whether the dot controled by Player2 
			 * touch the sliding barrier. 
			 * If yes, then gameover, Player1 WIN.
			 */
			// checkTouch(gameIsOver);
			
			tinygl_update();
		}
		
		/* Call gameover message and display the messagees */
		//gameover();
	}
    
    return 0;
}
