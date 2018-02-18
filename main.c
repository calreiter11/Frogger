//**************************************************
// 
// Authors: Caleb Reiter, Alex Fanner, Chris Chu, Kao Yang
// Description: 2-player Frogger Game
//
//**************************************************

#include <stdlib.h>

#include "ece210_api.h"
#include "lab_buttons.h"
#include "Images.h"

/***************************************************
 * Structures
 **************************************************/

//structure for each entity's image
struct Image {
	uint8_t width;								//width of the image [pixels]
	uint8_t height;								//height of the image [pixels]
	uint16_t foregroundColor;			//foreground color of the image
	uint16_t backgroundColor;			//background color of the image
	const uint8_t *bitmap;				//pointer to bitmap representing the image
};

//structure for each entity (both players and non-players)
struct Entity {
		bool isPlayer;							//if the entity is the player
		uint8_t xpos;								//x position of entity [pixels]
		uint16_t ypos;							//y position of entity [pixels]
		uint8_t move;								//move speed of entity [pixels]
		uint8_t direction;					//default direction of entity [one of PS2_RIGHT, PS2_UP, PS2_LEFT, PS2_DOWN]
		struct Image image;					//entity's image
};

/***************************************************
 * Function Prototypes
 **************************************************/

/* Updates an entity's position based on its current direction
 * Parameters:
 * 	-*entity: pointer to the entity to be updated
 */
void updateEntity(struct Entity *entity);

/* Updates player's position based on joystick and button input
 * Parameters:
 * -*player: pointer to the player's entity
 * -*canMove: pointer to the canMove variable
 */
void updatePlayer(struct Entity *player, bool *canMove);

/* Determines whether a move is valid based on entity's current position and planned move
 * Parameters:
 * 	-entity: the entity whose move is being validated (no pointer necessary because no entity variables are being modified)
 * 	-direction: the direction the entity is planning on moving
 * Returns:
 * 	-True if the entity is not the player or if the entity is a player and can move in the indicated direction
 * 	-False otherwise
 */
bool isValidMove(struct Entity entity, uint8_t direction);

/* Renders an entity on the screen
 * Parameters:
 * 	-entity: the entity to be rendered (no pointer necessary because no entity variables are being modified)
 */
void render(struct Entity entity);

/* Erases an entity's image from the screen
 * Parameters:
 * 	-: entity: the entity to be erased
 */
void erase(struct Entity entity);

/* Prints a string in red on the center of the screen (for debugging purposes)
 * Parameters:
 * 	-*string: the string to be printed
 */
void print(char *string, uint8_t row);

/* Clears the screen of any messages or images */
void clearScreen(void);

/* Initialize entity array with random entities
 * Parameters:
 * 	-a: the height of the array
 * 	-b: the width of the array
 * 	-entities[][]: array of entities to initialize
 */
void fillEntities(uint8_t a, uint8_t b, struct Entity entities[a][b]);

/* Initialize a single entity with the given variables
 * Parameters:
 * 	-*entity: the entity to initialize (pointer because variables are being modified)
 * 	-isPlayer: if the entity is a player
 * 	-xpos: x position of the entity [pixels]
 * 	-ypos: y position of the entity [pixels]
 * 	-move: move speed of the entity [pixels]
 * 	-direction: direction of the entity [one of PS2_RIGHT, PS2_UP, PS2_LEFT, PS2_DOWN]
 * 	-image: the entity's image
 */
void initEntity(struct Entity *entity, bool isPlayer, uint8_t xpos, uint16_t ypos, uint8_t move, uint8_t direction, struct Image image);

/* Generate a random image from the list of entity images
 * Returns:
 * 	-the random image
 */
struct Image randImage(void);

/* Determines whether a number is in a given range (inclusive)
 * Parameters:
 * 	-num: the number to compare
 * 	-start: the start of the range
 * 	-end: the end of the range
 * Returns:
 * 	-True if the number is within the given range (inclusive)
 * 	-False otherwise
 */
 bool inRange(uint8_t num, uint8_t start, uint8_t end);

/* Generate a random number in the given range (inclusive-exclusive)
 * Parameters:
 * 	-start: start of the range
 * 	-end: end of the range
 * Returns:
 * 	-a random number in the range
 */
uint8_t randRange(uint8_t start, uint8_t end);

/* Calculate the x offset to render the entity in the horizontal center of each grid location
 * Parameters:
 * 	-image: the image being offset
 * Returns:
 * 	-the x offset of the image [pixels]
 */
uint8_t xOffset(struct Image image);

/* Calculate the y offset to render the entity in the vertical center of each grid location
 * Parameters:
 * 	-image: the image being offset
 * Returns:
 * 	-the y offset of the image [pixels]
 */
uint8_t yOffset(struct Image image);

/* Draw water for each water row */
void drawWater(void);

/* Draw grass for the top and bottom rows */
void drawGrass(void);

/* Draw borders around the edge of the game */
void drawBorders(void);

/* Set all LEDs along the top of the board to the given color
 * Parameters:
 * 	-red: red value of the color to set
 * 	-green: green value of the color to set
 * 	-blue: blue value of the color to set
 */
void setTopLEDs(uint8_t red, uint8_t green, uint8_t blue);

/* Corrects the x and y offset of an entity based on its image
 * Parameters:
 * 	-*entity: the entity to correct
 */
void correctOffset(struct Entity *entity);

/* Compares two images to determine whether they are equal (deep comparison)
 * Parameters:
 * 	-image1: the first image to compare
 * 	-image2: the second image to compare
 * Returns:
 * 	-True if the images are exactly the same
 * 	-False otherwise
 */
bool compareImage(struct Image image1, struct Image image2);

/* Corrects an entity's background image based on its current position
 * Parameters:
 * 	-*entity: the entity to correct
 */
void correctBackgroundImage(struct Entity *entity);

/***************************************************
 * Global Constants and Variables
 **************************************************/

#define LCD_WIDTH 240			//LCD width [pixels]
#define LCD_HEIGHT 320		//LCD height [pixels]

/* Possible positions of the player are represented as a GRID_WIDTH by GRID_HEIGHT grid; each move
 * brings the player from one grid location to another, and the player will never be in more
 * than one grid location at any given time
 */
#define GRID_SIZE	30			//height/width of each grid location [pixels]
#define GRID_WIDTH 7			//width of grid [number of grid locations]
#define GRID_HEIGHT	10		//height of grid [number of grid locations]

#define RED_ON 0x08				//color corresponding to RED
#define GREEN_ON 0x08			//color corresponding to GREEN
#define BLUE_ON 0x04			//color corresponding to BLUE
#define ALL_ON 0xFF				//color corresponding to all LEDs on

#define TOP_BORDER (LCD_HEIGHT - GRID_HEIGHT * GRID_SIZE) / 2		//position of top border [pixels]
#define LEFT_BORDER (LCD_WIDTH - GRID_WIDTH * GRID_SIZE) / 2		//position of left border [pixels]
#define RIGHT_BORDER LEFT_BORDER + GRID_WIDTH * GRID_SIZE				//position of right border [pixels]
#define BOTTOM_BORDER TOP_BORDER + GRID_HEIGHT * GRID_SIZE			//position of bottom border [pixels]

#define GAME_WIDTH RIGHT_BORDER - LEFT_BORDER				//width of game screen [pixels]
#define GAME_HEIGHT TOP_BORDER - BOTTOM_BORDER			//height of game screen [pixels]

#define WATER_ROWS 3														//number of rows of logs [number of rows]
#define CAR_ROWS GRID_HEIGHT - WATER_ROWS - 2		//number of rows of cars [number of rows]
#define ENTITY_ROWS WATER_ROWS + CAR_ROWS				//number of rows containing entities (excludes top and bottom rows) [number of rows]

#define LOGS_PER_ROW 1							//number of logs per water row [number of logs]
#define CARS_PER_ROW 1							//number of cars per car row [number of cars]
#define MAX_ENTITIES_PER_ROW 3  		//maximum number of non-player entities per row [number of entities]

#define LOG_MIN_MOVE 1					//minimum move speed of logs [pixels]
#define LOG_MAX_MOVE 2					//maximum move speed of logs [pixels]
#define CAR_MIN_MOVE 1					//minimum move speed of cars [pixels]
#define CAR_MAX_MOVE 2					//maximum move speed of cars [pixels]

#define NUM_ENTITY_IMAGES 3			//number of different non-player images [number of images]

#define LOCAL_ID 0x11 			//ID of local board
#define REMOTE_ID 0x00			//ID of remote board

#define UP_BUTTON 0x01			//data corresponding to up button being pressed
#define WIN 0x00FF00FF			//data corresponding to a player winning

//constant array containing all possible non-player entity images
const struct Image ENTITY_IMAGES[] = {
	{CAR1_BITMAP_WIDTH, CAR1_BITMAP_HEIGHT, CAR1_FOREGROUND_COLOR, CAR1_BACKGROUND_COLOR, CAR1_BITMAP},										//car1 image
	{RACECAR_BITMAP_WIDTH, RACECAR_BITMAP_HEIGHT, RACECAR_FOREGROUND_COLOR, RACECAR_BACKGROUND_COLOR, RACECAR_BITMAP},		//racecar image
	{TRUCK_BITMAP_WIDTH, TRUCK_BITMAP_HEIGHT, TRUCK_FOREGROUND_COLOR, TRUCK_BACKGROUND_COLOR, TRUCK_BITMAP}								//truck image
};

//constant image for all logs
const struct Image LOG_IMAGE = {LOG_BITMAP_WIDTH, LOG_BITMAP_HEIGHT, LOG_FOREGROUND_COLOR, LOG_BACKGROUND_COLOR, LOG_BITMAP};

//frog images for each direction
const struct Image FROG_IMAGE_UP = {FROG_BITMAP_UP_WIDTH, FROG_BITMAP_UP_HEIGHT, FROG_FOREGROUND_COLOR, FROG_BACKGROUND_COLOR, FROG_BITMAP_UP};
const struct Image FROG_IMAGE_RIGHT = {FROG_BITMAP_RIGHT_WIDTH, FROG_BITMAP_RIGHT_HEIGHT, FROG_FOREGROUND_COLOR, FROG_BACKGROUND_COLOR, FROG_BITMAP_RIGHT};
const struct Image FROG_IMAGE_LEFT = {FROG_BITMAP_LEFT_WIDTH, FROG_BITMAP_LEFT_HEIGHT, FROG_FOREGROUND_COLOR, FROG_BACKGROUND_COLOR, FROG_BITMAP_LEFT};
const struct Image FROG_IMAGE_DOWN = {FROG_BITMAP_DOWN_WIDTH, FROG_BITMAP_DOWN_HEIGHT, FROG_FOREGROUND_COLOR, FROG_BACKGROUND_COLOR, FROG_BITMAP_DOWN};

//player 2 image
const struct Image FROG2_IMAGE = {FROG_BITMAP_UP_WIDTH, FROG_BITMAP_UP_HEIGHT, FROG2_FOREGROUND_COLOR, FROG2_BACKGROUND_COLOR, FROG_BITMAP_UP};

/**************************************************
 * Main
 *************************************************/
int main(void) {
	
	//initialize board
	ece210_initialize_board();
	ece210_lcd_add_msg("2-Player Frogger", TERMINAL_ALIGN_CENTER, LCD_COLOR_GREEN);
	for (uint8_t i = 0; i < 6; i++) { ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER, LCD_COLOR_BLACK); }
	ece210_lcd_add_msg("Initializing...", TERMINAL_ALIGN_CENTER, LCD_COLOR_CYAN);
	ece210_wireless_init(LOCAL_ID, REMOTE_ID);
	
	//display ID message
	char idmsg[20];
	sprintf(idmsg, "LOCAL ID: %d, REMOTE ID: %d", LOCAL_ID, REMOTE_ID); 
	ece210_lcd_add_msg(idmsg, TERMINAL_ALIGN_CENTER, LCD_COLOR_BLUE2);
	
	//set initial conditions
	bool playing = true;					//if the player is still playing the game
	bool alive = true;						//if the player is still alive (hasn't lost)
	bool localWin = false;				//if the local player has won
	bool remoteWin = false;				//if the remote player has won
	bool canMove = true;					//if the player is able to move (cannot move after moving until joystick is returned to center)
	bool localReady = false;			//if the local player is ready to play
	bool remoteReady = false;			//if the remote player is ready to play
	uint32_t ticks = 0;						//number of ticks until game starts - used to seed random number generator
	uint32_t data;								//data being transmitted wirelessly
	uint32_t player2data;					//data being received wirelessly
	uint16_t oldx, oldy;					//old x and y position of the frog
	
	//array of all entities to be updated
	struct Entity entities[ENTITY_ROWS][MAX_ENTITIES_PER_ROW] = {0, 0, 0, 0, 0, 0};
	
	//initialize frog entity
	struct Entity frog;
	frog.isPlayer = true;
	frog.move = GRID_SIZE;
	frog.image = FROG_IMAGE_UP;
	
	//initialize player 2 entity
	struct Entity frog2;
	frog2.image = FROG2_IMAGE;
	
	//overall game loop (spans multiple games)
	while(playing) {
	
		//initialize wireless
		ece210_wireless_init(LOCAL_ID, REMOTE_ID);
		//reset ready status of both players
		localReady = false;
		remoteReady = false;
		
		//prompt to ready up
		ece210_lcd_add_msg("Press up key to play.", TERMINAL_ALIGN_CENTER, LCD_COLOR_BLUE);
		//loop while both players are not ready
		while (!localReady || !remoteReady) {
			//wait for local player to be ready
			if (btn_up_pressed() && !localReady) {
				//set local player status to ready
				localReady = true;
				//send ready status to player 2
				ece210_wireless_send(UP_BUTTON);
				//display local ready message
				ece210_lcd_add_msg("You are ready!", TERMINAL_ALIGN_CENTER, LCD_COLOR_GREEN);
			}
			
			//wait for player 2 to be ready
			if (ece210_wireless_data_avaiable() && !remoteReady) {
				//if up button is received
				if (ece210_wireless_get() == UP_BUTTON) {
					//set player 2 status to ready
					remoteReady = true;
					//display player 2 ready message
					ece210_lcd_add_msg("Opponent is ready!", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
				}
			}
			
			//increment ticks
			ticks++;
		}
			
		//seed random number generator
		srand(ticks);
			
		//start game
		ece210_lcd_add_msg("All players are ready.", TERMINAL_ALIGN_CENTER, LCD_COLOR_WHITE);
		ece210_lcd_add_msg("Starting in:", TERMINAL_ALIGN_CENTER, LCD_COLOR_WHITE);
		ece210_lcd_add_msg("3", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
		setTopLEDs(100, 0, 0);
		ece210_wait_mSec(1000);
	
		ece210_lcd_add_msg("2", TERMINAL_ALIGN_CENTER, LCD_COLOR_ORANGE);
		setTopLEDs(100, 100, 0);
		ece210_wait_mSec(1000);
	
		ece210_lcd_add_msg("1", TERMINAL_ALIGN_CENTER, LCD_COLOR_YELLOW);
		setTopLEDs(0, 100, 0);
		ece210_wait_mSec(1000);
	
		ece210_lcd_add_msg("Begin!", TERMINAL_ALIGN_CENTER, LCD_COLOR_GREEN);
		setTopLEDs(0, 0, 0);
		
		//reset game conditions
		alive = true;
		localWin = false;
		remoteWin = false;
		frog.xpos = LEFT_BORDER + randRange(0, GRID_WIDTH) * GRID_SIZE + xOffset(frog.image);
		frog.ypos = TOP_BORDER + ((GRID_HEIGHT - 1) * GRID_SIZE) + yOffset(frog.image);
		frog2.xpos = LEFT_BORDER + randRange(0, GRID_WIDTH) * GRID_SIZE + xOffset(frog2.image);
		frog2.ypos = TOP_BORDER + ((GRID_HEIGHT - 1) * GRID_SIZE) + yOffset(frog2.image);
		fillEntities(ENTITY_ROWS, MAX_ENTITIES_PER_ROW, entities);
		clearScreen();
		drawBorders();
		drawWater();
		drawGrass();
		
		//individual game loop (runs for one game)
		while(alive && !localWin && !remoteWin) {
			
			//temporary variables to hold old frog position
			oldx = frog.xpos;
			oldy = frog.ypos;
			
			//update and render non-player entities
			for (uint8_t i = 0; i < ENTITY_ROWS; i++) {
				for (uint8_t j = 0; j < (i < WATER_ROWS ? LOGS_PER_ROW : CARS_PER_ROW); j++) {
					updateEntity(&entities[i][j]);
					render(entities[i][j]);
				}
			}
			
			//update player's position
			updatePlayer(&frog, &canMove);
			
			//calculate frog's current row, leftmost position, and rightmost position
			uint8_t row = (frog.ypos - TOP_BORDER) / GRID_SIZE;
			uint8_t f_left = frog.xpos;
			uint8_t f_right = frog.xpos + frog.image.width;	
			
			//only check for collisions if the player is not on the first or last row
			if (row != GRID_HEIGHT - 1 && row != 0) {
				//if the player is not in a water row
				if (row > WATER_ROWS) {
					//check for collisions between player and non-player entities on same row
					for (uint8_t i = 0; i < CARS_PER_ROW; i++) {
						uint8_t e_left = entities[row - 1][i].xpos;
						uint8_t e_right = entities[row - 1][i].xpos + entities[row - 1][i].image.width;		
						if (inRange(e_left, f_left, f_right) || inRange(e_right, f_left, f_right)) {
							alive = false;
						}
					}
				} else {
					//determine whether player is on a log
					bool onLog = false;
					for (uint8_t i = 0; i < LOGS_PER_ROW; i++) {
						uint8_t e_left = entities[row - 1][i].xpos;
						uint8_t e_right = entities[row - 1][i].xpos + entities[row - 1][i].image.width;
						if (f_left >= e_left && f_left <= e_right && f_right >= e_left && f_right <= e_right) {
							onLog = true;
							frog.xpos += entities[row - 1][i].move * (entities[row - 1][i].direction == PS2_LEFT ? -1 : 1);
						}
					}
					if (!onLog) alive = false;
				}
			}
			
			//send new position if it has changed from old position
			if (frog.xpos != oldx || frog.ypos != oldy) {
				//note: next if statement is necessary to prevent sending too many updates in a small period of time;
				//implementing a delay (e.g. only sending every 3rd update) still did not fix the issue of player 2 not
				//being rendered when on the water rows, so the condition is left to prevent freezing
				if (row > WATER_ROWS) {
					data = (frog.xpos << 16) + frog.ypos;
					ece210_wireless_send(data);
				}
			}
			
			//clear player 2's image
			erase(frog2);
			
			//update player 2's position if new data is available
			if (ece210_wireless_data_avaiable()) {
				player2data = ece210_wireless_get();
				if (player2data != WIN) {
					frog2.xpos = (player2data & 0xFFFF0000) >> 16;
					frog2.ypos = player2data & 0x0000FFFF;
					correctBackgroundImage(&frog2);
				} else {
					remoteWin = true;
				}
			}
			
			//correct offset for player 2's image
			correctOffset(&frog2);
			
			//render players
			render(frog2);
			render(frog);
			
			//check for a winner
			if (frog.ypos < TOP_BORDER + GRID_SIZE) {
				localWin = true;
				ece210_wireless_send(WIN);
			}

		} // end while (alive && !localWin && !remoteWin)
		
		//display endgame messages
		if (localWin) {
			//display winning message
			ece210_lcd_add_msg("CONGRATULATIONS!", TERMINAL_ALIGN_CENTER, LCD_COLOR_YELLOW);
			ece210_lcd_add_msg("YOU WIN!", TERMINAL_ALIGN_CENTER, LCD_COLOR_YELLOW);
			
			//display rainbow on LEDs
			for(uint8_t i = 0; i < 5; i++){
				setTopLEDs(RED_ON, GREEN_ON, BLUE_ON);
				ece210_ws2812b_write( 0, 50, GREEN_ON, BLUE_ON);
				ece210_wait_mSec(150);
				ece210_ws2812b_write( 1, RED_ON, 50, BLUE_ON);
				ece210_wait_mSec(150);
				ece210_ws2812b_write( 2, RED_ON, GREEN_ON, 50);
				ece210_wait_mSec (150);
				ece210_ws2812b_write( 3, 25, 50, 120);
				ece210_wait_mSec (150);
				ece210_ws2812b_write( 4, RED_ON, 100, 50);
				ece210_wait_mSec (150);
				ece210_ws2812b_write( 5, 50, 50, BLUE_ON);
				ece210_wait_mSec (150);
				ece210_ws2812b_write( 6, 50, GREEN_ON, 50);
				ece210_wait_mSec (150); 
				ece210_ws2812b_write( 7, RED_ON, 50, 50);
				ece210_wait_mSec (150); 
			}
		
		} else {
			//display game over message
			if (remoteWin) {
				ece210_lcd_add_msg("PLAYER 2 WINS!", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
			} else {
				ece210_lcd_add_msg("GAME OVER!", TERMINAL_ALIGN_CENTER, LCD_COLOR_ORANGE);
			}
			
			//display red LEDs
			for (uint8_t i = 0; i < 10; i++) {	
				setTopLEDs(100, 0, 0);
				ece210_wait_mSec (150);
				setTopLEDs(0, 0, 0);
			}
			
		}
		
		//prompt for new game
		ece210_lcd_add_msg("Press up to play again!", TERMINAL_ALIGN_CENTER, LCD_COLOR_WHITE);
		ece210_lcd_add_msg("Press any other button to quit.", TERMINAL_ALIGN_CENTER, LCD_COLOR_WHITE);
		
		//clear LEDs
		setTopLEDs(0, 0, 0);
		
		//wait until a button is pressed
		while(!btn_right_pressed() && !btn_up_pressed() && !btn_left_pressed() && !btn_down_pressed()
					&& ece210_ps2_read_position() == PS2_CENTER) {
		}
		
		//if any button is pressed other than the up button, stop playing the game
		if (!btn_up_pressed() && ece210_ps2_read_position() != PS2_UP) {
			playing = false;
		}
		
	} // end while(playing)
	
	//final message
	ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER, LCD_COLOR_BLACK);
	ece210_lcd_add_msg("THANKS FOR PLAYING!", TERMINAL_ALIGN_CENTER, LCD_COLOR_WHITE);
	clearScreen();
	
} // end main

/***************************************************
 * Functions
 **************************************************/

void updateEntity(struct Entity *entity) {
	//only move if the entity's move is valid (only applies to player)
	if (isValidMove(*entity, entity->direction)) {
		
		//erase entity's image from its current spot
		erase(*entity);
		
		//change player image to indicate direction
		if (entity->isPlayer) {
			switch(entity->direction) {
				case PS2_RIGHT:
					entity->image = FROG_IMAGE_RIGHT; break;
				case PS2_UP:
					entity->image = FROG_IMAGE_UP; break;
				case PS2_LEFT:
					entity->image = FROG_IMAGE_LEFT; break;
				case PS2_DOWN:
					entity->image = FROG_IMAGE_DOWN; break;
			}
			
			//correct offset for new image if not in the water or if moving left/right
			if (entity->ypos >= TOP_BORDER + GRID_SIZE * WATER_ROWS
					|| entity->direction == PS2_RIGHT
					|| entity->direction == PS2_LEFT) {
						correctOffset(entity);
			}
		}
		
		//move entity based on current direction
		switch(entity->direction) {
			case PS2_RIGHT:
				entity->xpos += entity->move; break;
			case PS2_UP:
				entity->ypos -= entity->move; break;
			case PS2_LEFT:
				entity->xpos -= entity->move; break;
			case PS2_DOWN:
				entity->ypos += entity->move;	break;
		}
	
		//correct entity when going off screen to the right
		if (entity->direction == PS2_RIGHT && entity->xpos + entity->image.width >= RIGHT_BORDER) {
			//if entity is a log, switch direction with new random speed
			if (compareImage(entity->image, LOG_IMAGE)) {
				entity->xpos = entity->xpos - 1;
				entity->direction = PS2_LEFT;
				entity->move = randRange(LOG_MIN_MOVE, LOG_MAX_MOVE + 1);
			} else {
				//if entity is not a log, loop entity from right to left
				entity->xpos = LEFT_BORDER + 1;
			}
		}
		
		//correct entity if going off screen to the left
		if (entity->direction == PS2_LEFT && entity->xpos <= LEFT_BORDER) {
			//if entity is a log, switch direction with new random speed
			if (compareImage(entity->image, LOG_IMAGE)) {
				entity->xpos = entity->xpos + 1;
				entity->direction = PS2_RIGHT;
				entity->move = randRange(LOG_MIN_MOVE, LOG_MAX_MOVE + 1);
			} else {
				//if entity is not a log, loop entity from left to right
				entity->xpos = RIGHT_BORDER - entity->image.width - 1;
			}
		}
		
		//correct background image if the entity is a player
		if (entity->isPlayer) {
			correctBackgroundImage(entity);
		}
		
	//if entity is a player and entity tries to make an invalid move
	} else if (entity->isPlayer) {
		//display flashing lights to indicate invalid move
		for (uint8_t i = 0; i < 2; i++) {
			//turn lights on
			setTopLEDs(100, 100, 0);
			//pause
			ece210_wait_mSec (25);
			//turn lights off
			setTopLEDs(0, 0, 0);
		}
	}
} //end updateEntity()

void updatePlayer(struct Entity *player, bool *canMove) {
	
	//read direction of joystick
	uint8_t direction = ece210_ps2_read_position();
	
	//override direction variable if buttons are being used instead
	if (AlertButtons) {
		AlertButtons = false;
		if (btn_right_pressed()) direction = PS2_RIGHT;
		if (btn_up_pressed()) direction = PS2_UP;
		if (btn_left_pressed()) direction = PS2_LEFT;
		if (btn_down_pressed()) direction = PS2_DOWN;
	}
	
	//if no directional input, reset move variable; else, if the player can move, move player
	if (direction == PS2_CENTER) {
		//allow player to move again
		*canMove = true;
	} else if (*canMove) {
		//prevent player from making further moves until no input is detected
		*canMove = false;
		//update player's direction variable
		player->direction = direction;
		//update player entity
		updateEntity(player);
	}
} //end updatePlayer();

bool isValidMove(struct Entity entity, uint8_t direction) {

	//return true if the entity is not a player
	if (!entity.isPlayer) return true;

	//return false if entity's move exceeds boundaries based on the move direction
	switch(direction) {
		case PS2_RIGHT:
			if (entity.xpos + entity.image.width + entity.move > RIGHT_BORDER) return false;
			break;
		case PS2_UP:
			if (entity.ypos - entity.move < TOP_BORDER) return false;
			break;
		case PS2_LEFT:
			if (entity.xpos - entity.move < LEFT_BORDER) return false;
			break;
		case PS2_DOWN:
			if (entity.ypos + entity.image.height + entity.move > BOTTOM_BORDER) return false;
			break;
	}
	
	//return true if the entity is a player and can make the desired move
	return true;
	
} //end isValidMove()

void render(struct Entity entity) {
	//draw the entity on the screen
	ece210_lcd_draw_image(entity.xpos, entity.image.width, entity.ypos, entity.image.height, entity.image.bitmap, 
												entity.image.foregroundColor, entity.image.backgroundColor);
} //end render()

void erase(struct Entity entity) {
	ece210_lcd_draw_image(entity.xpos, entity.image.width, entity.ypos, entity.image.height, entity.image.bitmap,
												entity.image.backgroundColor, entity.image.backgroundColor);
} //end erase()

void print(char *string, uint8_t row) {
	//print the string
	ece210_lcd_print_string(string, LCD_WIDTH - 10, 10, LCD_COLOR_RED, LCD_COLOR_BLACK);
} //end print()

void clearScreen() {
	//display 16 lines of empty text to clear the message list
	for (uint8_t i = 0; i < 16; i++) {
		ece210_lcd_add_msg("", TERMINAL_ALIGN_CENTER, LCD_COLOR_BLACK);
	}
	//clear any remaining images from the screen
	ece210_lcd_draw_rectangle(0, LCD_WIDTH, 0, LCD_HEIGHT, LCD_COLOR_BLACK);
} //end clearScreen()

void fillEntities(uint8_t a, uint8_t b, struct Entity entities[a][b]) {
	
	//initial variables
	uint8_t xpos;														//x position of entity being initialized [pixels]
	uint8_t car_mid = CARS_PER_ROW / 2;			//middle car in the row [number of entities]
	uint8_t log_mid = LOGS_PER_ROW / 2;			//middle log in the row [number of entities]
	
	//loop over each row of entities
	for (uint8_t i = 0; i < ENTITY_ROWS; i++) {
		
		//generate random x position for the middle entity in the row
		uint8_t xmid = randRange(LEFT_BORDER, RIGHT_BORDER);
		//generate random direction for all entities in the row
		uint8_t direction = (randRange(1,100) % 2 == 1 ? PS2_RIGHT : PS2_LEFT);
		
		uint8_t spacing;
		struct Image image;
		uint8_t move;
		
		if (i < WATER_ROWS) {
			image = LOG_IMAGE; 
			//calculate spacing based on number of entities in the row and the width of the entities' images
			spacing = (GAME_WIDTH - (LOGS_PER_ROW * image.width)) / LOGS_PER_ROW;
			//generate random move speed for all entities in the row
			move = randRange(LOG_MIN_MOVE, LOG_MAX_MOVE + 1);
		} else {
			//generate random image for all entities in the row
			image = randImage();
			//calculate spacing based on number of entities in the row and the width of the entities' images
			spacing = (GAME_WIDTH - (CARS_PER_ROW * image.width)) / CARS_PER_ROW;
			//generate random move speed for all entities in the row
			move = randRange(CAR_MIN_MOVE, CAR_MAX_MOVE + 1);
		}
		
		//loop over entities in the row
		for (uint8_t j = 0; j < (i < WATER_ROWS ? LOGS_PER_ROW : CARS_PER_ROW); j++) {
			
			//calculate x position of each entity based on previously generated x position of middle entity
			xpos = xmid + (j - (i < WATER_ROWS ? log_mid : car_mid)) * (spacing + image.width);
			
			//correct x positions if they are off screen either left or right
			if (xpos < LEFT_BORDER) xpos = RIGHT_BORDER + xpos;
			if (xpos > RIGHT_BORDER - image.width) xpos = LEFT_BORDER + xpos - (RIGHT_BORDER - image.width);
						
			//initialize the entity with all variables
			initEntity(	&entities[i][j],																			//pointer address
									false,																								//isPlayer
									xpos,																									//xpos
									(i + 1) * GRID_SIZE + TOP_BORDER + yOffset(image),		//ypos
									move,																									//move
									direction,																						//direction
									image);																								//image
		}
	}
	
} //end fillEntities()

void initEntity(struct Entity *entity, bool isPlayer, uint8_t xpos, uint16_t ypos, uint8_t move, uint8_t direction, struct Image image) {
	//assign all variables to the given entity
	entity->isPlayer = isPlayer;
	entity->xpos = xpos;
	entity->ypos = ypos;
	entity->move = move;
	entity->direction = direction;
	entity->image = image;
} //end initEntity()

struct Image randImage() {
	return ENTITY_IMAGES[randRange(1, 100) % NUM_ENTITY_IMAGES];
} //end randImage()

bool inRange(uint8_t num, uint8_t start, uint8_t end) {
	return (num >= start && num <= end);
} //end inRange()

uint8_t randRange(uint8_t start, uint8_t end) {
	uint8_t range = end - start;
	return rand() % range + start;
} //end randRange()
								
uint8_t xOffset(struct Image image) {
	return (GRID_SIZE - image.width) / 2;
} //end xOffset()

uint8_t yOffset(struct Image image) {
	return (GRID_SIZE - image.height) / 2;
} //end yOffset()

void drawWater() {
	ece210_lcd_draw_rectangle(LEFT_BORDER + 1, GAME_WIDTH - 2, TOP_BORDER + GRID_SIZE, WATER_ROWS * GRID_SIZE, LCD_COLOR_BLUE);
} //end drawWater()

void drawGrass() {
	ece210_lcd_draw_rectangle(LEFT_BORDER + 1, GAME_WIDTH - 2, TOP_BORDER + 1, GRID_SIZE - 1, LCD_COLOR_GREEN2);
	ece210_lcd_draw_rectangle(LEFT_BORDER + 1, GAME_WIDTH - 2, TOP_BORDER + GRID_SIZE * (GRID_HEIGHT - 1), GRID_SIZE - 1, LCD_COLOR_GREEN2);
} //end drawGrass()

void drawBorders() {
	//draw rectangle spanning entire game screen
	ece210_lcd_draw_rectangle(LEFT_BORDER, GAME_WIDTH, TOP_BORDER, GAME_HEIGHT, LCD_COLOR_GRAY);
	//clear inside of game screen, leaving a 1 pixel border around the edge
	ece210_lcd_draw_rectangle(LEFT_BORDER + 1, GAME_WIDTH - 2, TOP_BORDER + 1, GAME_HEIGHT - 2, LCD_COLOR_BLACK);
} //end drawBorders()

void setTopLEDs(uint8_t red, uint8_t green, uint8_t blue) {
	for (uint8_t i = 0; i < 8; i++) {
		ece210_ws2812b_write(i, red, green, blue);
	}
}

void correctOffset(struct Entity *entity) {
	//set position to top left of grid location
	entity->xpos = ((entity->xpos - LEFT_BORDER) / GRID_SIZE) * GRID_SIZE + LEFT_BORDER;
	entity->ypos = ((entity->ypos - TOP_BORDER) / GRID_SIZE) * GRID_SIZE + TOP_BORDER;
	
	//column the entity is in
	uint8_t gridx = (entity->xpos - LEFT_BORDER) / GRID_SIZE;
	//x position of the left side of the column
	uint8_t leftx = gridx * GRID_SIZE + LEFT_BORDER;
	//x position of the right side of the column
	uint8_t rightx = (gridx + 1) * GRID_SIZE + LEFT_BORDER;
	//if the entity is closer to the right column, set position to right side of column;
	//else, set position to left side of column
	if (entity->xpos - leftx > rightx - entity->xpos) {
		entity->xpos = rightx;
	} else {
		entity->xpos = leftx;
	}
	//offset position
	entity->xpos += xOffset(entity->image);
	entity->ypos += yOffset(entity->image);
}

bool compareImage(struct Image image1, struct Image image2) {
	return (image1.backgroundColor == image2.backgroundColor &&
					image1.bitmap == image2.bitmap &&
					image1.foregroundColor == image2.foregroundColor &&
					image1.height == image2.height &&
					image1.width == image2.width);
}

void correctBackgroundImage(struct Entity *entity) {
	//row the entity is in
	uint8_t row = (entity->ypos - TOP_BORDER) / GRID_SIZE;
	//if the row is the top or bottom row, set background color to GREEN2;
	//else, if the row is a water row, set background color to BROWN (logs);
	//else, set background color to BLACK
	if (row == 0 || row == GRID_HEIGHT - 1) {
		entity->image.backgroundColor = LCD_COLOR_GREEN2;
	} else if (row >= 1 && row <= WATER_ROWS) {
		entity->image.backgroundColor = LCD_COLOR_BROWN;
	} else {
		entity->image.backgroundColor = LCD_COLOR_BLACK;
	}
}
