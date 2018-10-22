/**
 * Ask players to conform the game.
 */

#include "system.h"
#include "led.h"
#include "timer.h"
#include "pacer.h"
#include "tinygl.h"
#include "ir_uart.h"
#include "navswitch.h"
#include "gameReady.h"
#include "../fonts/font5x7_1.h"

#define CONFORM 'Y'
#define LOOP_RATE 1000
#define MESSAGE_RATE 10


void gameReady(void)
{
	static uint8_t conform1 = 0;
	static uint8_t conform2 = 0;
	
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
    while (!conform1 && !conform2) {
		pacer_wait();
		tinygl_update();
		navswitch_update();
		
		// Player1 is ready
        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
			ir_uart_putc(CONFORM);
            conform1 = !conform1;
        } 

        if (ir_uart_read_ready_p()) {
			if (ir_uart_getc() == CONFORM) {
				conform2 = !conform2;
			}
		}
		
		
	}
	
	/* Count backward for 3 seconds */
	static uint8_t state = 0;
	static uint8_t counter = 0;
	led_init ();
	pacer_init (2);
	
	while (counter < 3) {
		 pacer_wait();
		 led_set(LED1, state);
		 state = !state;
		 counter++;
	}
}
