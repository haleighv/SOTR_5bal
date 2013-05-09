/*******************************************************************************
* File: asteroids.c
*
* Description: This FreeRTOS program uses the AVR graphics module to display
*  and track the game state of the classic arcade game "Asteroids." When
*  running on the AVR STK500, connect the switches to port B. SW7 turns left,
*  SW6 turns right, SW1 accelerates forward, and SW0 shoots a bullet. Initially,
*  five large asteroids are spawned around the player. As the player shoots the
*  asteroids with bullets, they decompose into three smaller asteroids. When
*  the player destroys all of the asteroids, they win the game. If the player
*  collides with an asteroid, they lose the game. In both the winning and losing
*  conditions, the game pauses for three seconds and displays an appropriate
*  message. It is recommended to compile the FreeRTOS portion of this project
*  with heap_2.c instead of heap_1.c since pvPortMalloc and vPortFree are used.
*
* Author(s): Doug Gallatin & Andrew Lehmer
*
* Revisions:
*
*******************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "graphics.h"
#include "usart.h"

const char *astImages[] = {
	"a1.png",
	"a2.png",
	"a3.png"
};

typedef struct {
	float x;
	float y;
} point;

typedef struct object_s {
	xSpriteHandle handle;
	point pos;
	point vel;
	float accel;
	int16_t angle;
	int8_t a_vel;
	uint8_t size;
	uint16_t life;
	struct object_s *next;
} object;

#define DEG_TO_RAD M_PI / 180.0

#define INITIAL_ASTEROIDS 5
#define SCREEN_W 800
#define SCREEN_H 600

#define DEAD_ZONE_OVER_2 120

#define FRAME_DELAY_MS  10
#define BULLET_DELAY_MS 500
#define BULLET_LIFE_MS  1000

#define SHIP_SIZE 24
#define BULLET_SIZE 6
#define AST_SIZE_3 100
#define AST_SIZE_2 40
#define AST_SIZE_1 15

#define BULLET_VEL 6.0

#define SHIP_MAX_VEL 8.0
#define SHIP_ACCEL 0.1
#define SHIP_AVEL  6.0

#define BACKGROUND_AVEL 0.01

#define AST_MAX_VEL_3 2.0
#define AST_MAX_VEL_2 3.0
#define AST_MAX_VEL_1 4.0
#define AST_MAX_AVEL_3 3.0
#define AST_MAX_AVEL_2 6.0
#define AST_MAX_AVEL_1 9.0

#define LEFT_BUTTON  _BV(PB7)
#define RIGHT_BUTTON _BV(PB6)
#define ACCEL_BUTTON _BV(PB1)
#define SHOOT_BUTTON _BV(PB0)

static xTaskHandle inputTaskHandle;
static xTaskHandle bulletTaskHandle;
static xTaskHandle updateTaskHandle;

static xSemaphoreHandle usartMutex;

static object ship;
static object *bullets = NULL;
static object *asteroids = NULL;

static xGroupHandle astGroup;
static xSpriteHandle background;

void init(void);
void reset(void);
int16_t getRandStartPosVal(int16_t dimOver2);
object *createAsteroid(float x, float y, float velx, float vely, int16_t angle, int8_t avel, int8_t size, object *nxt);
uint16_t sizeToPix(int8_t size);
object *createBullet(float x, float y, float velx, float vely, object *nxt);
void spawnAsteroid(point *pos, uint8_t size);

/*------------------------------------------------------------------------------
 * Function: inputTask
 *
 * Description: This task polls PINB for the current button state to determine
 *  if the player should turn, accelerate, or both. This task never blocks, so
 *  it should run at the lowest priority above the idle task priority.
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void inputTask(void *vParam) 
{
    /* Note:
     * ship.accel stores if the ship is moving
     * ship.a_vel stores which direction the ship is moving in
     */
	
    while (1)
	{
		if(LEFT_BUTTON)
			ship.vel-= ;
		if(RIGHT_BUTTON)
			ship.vel++;
		if(ACCEL_BUTTON)
			ship.accel+=SHIP_ACCEL;
	}
}

/*------------------------------------------------------------------------------
 * Function: bulletTask
 *
 * Description: This task polls the state of the button that fires a bullet
 *  every 10 milliseconds. If a bullet is fired, this task blocks for half a
 *  second to regulate the fire rate.  If a bullet is not fired, the task
 *  blocks for a frame delay (FRAME_DELAY_MS)
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void bulletTask(void *vParam) {
	 /* Note:
     * The ship heading is stored in ship.angle.
     * The ship's position is stored in ship.pos.x and ship.pos.y
     *
     * You will need to use the following code to add a new bullet:
     bullets = createBullet(x, y, vx, vy, bullets);
     */
	// variable to hold ticks value of last task run
	portTickType xLastWakeTime;

	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
    while (1)
	{
		if(SHOOT_BUTTON)
		{
			bullets = createBullet(ship.pos.x, ship.pos.y, ship.vel.x + BULLET_VEL, ship.vel.y + BULLET_VEL, bullets);
						
			vTaskDelay((BULLET_LIFE_MS/2)/portTICK_RATE_MS);
		}
			
		vTaskDelayUntil(&xLastWakeTime, FRAME_DELAY_MS / portTICK_RATE_MS);
	}
}

/*------------------------------------------------------------------------------
 * Function: updateTask
 *
 * Description: This task observes the currently stored velocities for every
 *  game object and updates their position and rotation accordingly. It also
 *  updates the ship's velocities based on its current acceleration and angle.
 *  If a bullet has been in flight for too long, this task will delete it. This
 *  task runs every 10 milliseconds.
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void updateTask(void *vParam) {
	float vel;
	object *objIter, *objPrev;
	for (;;) {
		
		// spin ship
		ship.angle += ship.a_vel;
		if (ship.angle >= 360)
		    ship.angle -= 360;
		else if (ship.angle < 0)
		    ship.angle += 360;
		
		// move ship
		ship.vel.x += ship.accel * -sin(ship.angle * DEG_TO_RAD);
		ship.vel.y += ship.accel * -cos(ship.angle * DEG_TO_RAD);
		vel = ship.vel.x * ship.vel.x + ship.vel.y * ship.vel.y;
		if (vel > SHIP_MAX_VEL) {
			ship.vel.x *= SHIP_MAX_VEL / vel;
			ship.vel.y *= SHIP_MAX_VEL / vel;
		}
		
		ship.pos.x += ship.vel.x;
		ship.pos.y += ship.vel.y;
		
		if (ship.pos.x < 0.0) {
			ship.pos.x += SCREEN_W;
		} else if (ship.pos.x > SCREEN_W) {
			ship.pos.x -= SCREEN_W;
		}
		
		if (ship.pos.y < 0.0) {
			ship.pos.y += SCREEN_H;
		} else if (ship.pos.y > SCREEN_H) {
			ship.pos.y -= SCREEN_H;
		}
		
		// move bullets
		objPrev = NULL;
		objIter = bullets;
		while (objIter != NULL) {
			// Kill bullet after a while
			objIter->life += FRAME_DELAY_MS;
			if (objIter->life >= BULLET_LIFE_MS) {
				
				xSemaphoreTake(usartMutex, portMAX_DELAY);
				vSpriteDelete(objIter->handle);
				
				if (objPrev != NULL) {
					objPrev->next = objIter->next;
					vPortFree(objIter);
					objIter = objPrev->next;
				} else {
					bullets = objIter->next;
					vPortFree(objIter);
					objIter = bullets;
				}
				xSemaphoreGive(usartMutex);
			} else {
			    objIter->pos.x += objIter->vel.x;
			    objIter->pos.y += objIter->vel.y;
			
			    if (objIter->pos.x < 0.0) {
				    objIter->pos.x += SCREEN_W;
			    } else if (objIter->pos.x > SCREEN_W) {
				    objIter->pos.x -= SCREEN_W;
			    }
			
			    if (objIter->pos.y < 0.0) {
				    objIter->pos.y += SCREEN_H;
			    } else if (objIter->pos.y > SCREEN_H) {
				    objIter->pos.y -= SCREEN_H;
			    }
			
			    objPrev = objIter;
			    objIter = objIter->next;
			}			
		}
		
		// move asteroids
        /* 
         * ToDo: Add code to move the asteroids
         */
		
		vTaskDelay(FRAME_DELAY_MS / portTICK_RATE_MS);
	}
}

/*------------------------------------------------------------------------------
 * Function: drawTask
 *
 * Description: This task sends the appropriate commands to update the game
 *  graphics every 10 milliseconds for a target frame rate of 100 FPS. It also
 *  checks collisions and performs the proper action based on the types of the
 *  colliding objects.
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void drawTask(void *vParam) {
	object *objIter, *objPrev, *astIter, *astPrev, *objTemp;
	xSpriteHandle hit, handle;
	point pos;
	uint8_t size;
	
	vTaskSuspend(updateTaskHandle);
	vTaskSuspend(bulletTaskHandle);
	vTaskSuspend(inputTaskHandle);
	init();
	vTaskResume(updateTaskHandle);
	vTaskResume(bulletTaskHandle);
	vTaskResume(inputTaskHandle);
	
	for (;;) {
		xSemaphoreTake(usartMutex, portMAX_DELAY);
		
		vSpriteSetRotation(ship.handle, (uint16_t)ship.angle);
		vSpriteSetPosition(ship.handle, (uint16_t)ship.pos.x, (uint16_t)ship.pos.y);

		objPrev = NULL;
		objIter = bullets;
		while (objIter != NULL) {
			vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
			if (uCollide(objIter->handle, astGroup, &hit, 1) > 0) {
				vSpriteDelete(objIter->handle);
				
				if (objPrev != NULL) {
					objPrev->next = objIter->next;
					vPortFree(objIter);
					objIter = objPrev->next;
				} else {
					bullets = objIter->next;
					vPortFree(objIter);
					objIter = bullets;
				}
				astPrev = NULL;
				astIter = asteroids;
				while (astIter != NULL) {
					if (astIter->handle == hit) {
						pos = astIter->pos;
						size = astIter->size;
						vSpriteDelete(astIter->handle);
						if (astPrev != NULL) {
					        astPrev->next = astIter->next;
					        vPortFree(astIter);
					        astIter = astPrev->next;
				        } else {
					        asteroids = astIter->next;
					        vPortFree(astIter);
				        }
						spawnAsteroid(&pos, size);
						break;					
						
					} else {
						astPrev = astIter;
						astIter = astIter->next;
					}
				}
				
			} else {
				objPrev = objIter;
			    objIter = objIter->next;
			}			
		}
		
		objIter = asteroids;
		while (objIter != NULL) {
			vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
			vSpriteSetRotation(objIter->handle, objIter->angle);
			objIter = objIter->next;
		}			
				
		if (uCollide(ship.handle, astGroup, &hit, 1) > 0 || asteroids == NULL) {
			vTaskSuspend(updateTaskHandle);
			vTaskSuspend(bulletTaskHandle);
			vTaskSuspend(inputTaskHandle);
			
			if (asteroids == NULL)
			    handle = xSpriteCreate("win.png", SCREEN_W>>1, SCREEN_H>>1, 20, SCREEN_W>>1, SCREEN_H>>1, 100);
			else
			    handle = xSpriteCreate("lose.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
				
			vTaskDelay(3000 / portTICK_RATE_MS);
			vSpriteDelete(handle);
			
			reset();
			init();
			
			vTaskResume(updateTaskHandle);
			vTaskResume(bulletTaskHandle);
			vTaskResume(inputTaskHandle);
		}
		
		xSemaphoreGive(usartMutex);
		
		vTaskDelay(FRAME_DELAY_MS / portTICK_RATE_MS);
	}
}

int main(void) {
	DDRB = 0x00;
	TCCR2A = _BV(CS00); 
	
	usartMutex = xSemaphoreCreateMutex();
	
	vWindowCreate(SCREEN_W, SCREEN_H);
	
	sei();
	
	xTaskCreate(inputTask, (signed char *) "i", 80, NULL, 1, &inputTaskHandle);
	xTaskCreate(bulletTask, (signed char *) "b", 130, NULL, 2, &bulletTaskHandle);
	xTaskCreate(updateTask, (signed char *) "u", 200, NULL, 4, &updateTaskHandle);
	xTaskCreate(drawTask, (signed char *) "d", 230, NULL, 3, NULL);
	xTaskCreate(USART_Write_Task, (signed char *) "w", 150, NULL, 5, NULL);
	
	vTaskStartScheduler();
	
	for (;;)
	    ;
	return 0;
}

/*------------------------------------------------------------------------------
 * Function: init
 *
 * Description: This function initializes a new game of asteroids. A window
 *  must be created before this function may be called.
 *----------------------------------------------------------------------------*/
void init(void) {
	int i;
	
	bullets = NULL;
	asteroids = NULL;
	astGroup = ERROR_HANDLE;
	
	background = xSpriteCreate("stars.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
	
	srand(TCNT0);
	
	astGroup = xGroupCreate();
	
	for (i = 0; i < INITIAL_ASTEROIDS; i++) {
		asteroids = createAsteroid(getRandStartPosVal(SCREEN_W >> 1),
		                           getRandStartPosVal(SCREEN_H >> 1),
		                           (rand() % (int8_t)(AST_MAX_VEL_3 * 10)) / 5.0 - AST_MAX_VEL_3,
								   (rand() % (int8_t)(AST_MAX_VEL_3 * 10)) / 5.0 - AST_MAX_VEL_3,
								   rand() % 360,
								   (rand() % (int8_t)(AST_MAX_AVEL_3 * 10)) / 5.0 - AST_MAX_AVEL_3,
								   3,
								   asteroids);
	}
	
	ship.handle = xSpriteCreate("ship.png", SCREEN_W >> 1, SCREEN_H >> 1, 0, SHIP_SIZE, SHIP_SIZE, 1);
	ship.pos.x = SCREEN_W >> 1;
	ship.pos.y = SCREEN_H >> 1;
	ship.vel.x = 0;
	ship.vel.y = 0;
	ship.accel = 0;
	ship.angle = 0;
	ship.a_vel = 0;
}

/*------------------------------------------------------------------------------
 * Function: reset
 *
 * Description: This function destroys all game objects in the heap and clears
 *  their respective sprites from the window.
 *----------------------------------------------------------------------------*/
void reset(void) {
	/* Note:
     * You need to free all resources here using a reentrant function provided by
     * the freeRTOS API and clear all sprites from the game window.
     * Use vGroupDelete for the asteroid group.
     *
     * Remember bullets and asteroids are object lists so you should traverse the list
     * using something like:  
     *	while (thisObject != NULL) {
     *		vSpriteDelete(thisObject)
     *		nextObject = thisObject->next.
     *		delete thisObject using a reentrant function
     *		thisObject = nextObject
     *	}
     */
	 
}

/*------------------------------------------------------------------------------
 * Function: getRandStartPosVal
 *
 * Description: This function calculates a safe starting coordinate for an
 *  asteroid given the half extent of the current window.
 *
 * param dimOver2: Half of the dimension of the window for which a random
 *  coordinate value is desired.
 * return: A safe, pseudorandom coordinate value.
 *----------------------------------------------------------------------------*/
int16_t getRandStartPosVal(int16_t dimOver2) {
	return rand() % (dimOver2 - DEAD_ZONE_OVER_2) + (rand() % 2) * (dimOver2 + DEAD_ZONE_OVER_2);
}

/*------------------------------------------------------------------------------
 * Function: createAsteroid
 *
 * Description: This function creates a new asteroid object with a random
 *  sprite image.
 *
 * param x: The starting x position of the asteroid in window coordinates.
 * param y: The starting y position of the asteroid in window coordinates.
 * param velx: The starting x velocity of the asteroid in pixels per frame.
 * param vely: The starting y velocity of the asteroid in pixels per frame.
 * param angle: The starting angle of the asteroid in degrees.
 * param avel: The starting angular velocity of the asteroid in degrees per
 *  frame.
 * param size: The starting size of the asteroid. Must be in the range [1,3].
 * param nxt: A pointer to the next asteroid object in a linked list.
 * return: A pointer to a malloc'd asteroid object. Must be freed by the calling
 *  process.
 *----------------------------------------------------------------------------*/
object *createAsteroid(float x, float y, float velx, float vely, int16_t angle, int8_t avel, int8_t size, object *nxt) {
	/* ToDo:
     * Create a new asteroid object using a reentrant malloc() function
     * Setup the pointers in the linked list using:
     asteroid->next = nxt;
     * Create a new sprite using xSpriteCreate()
     * Add new asteroid to the group "astGroup" using:
     *	vGroupAddSprite() 
     */
}

/*------------------------------------------------------------------------------
 * Function: sizeToPix
 *
 * Description: This function converts a size enumeration value in the range
 *  [1,3] to its corresponding pixel dimension.
 *
 * param size: A number in the range [1-3]
 * return: A pixel size which may be used to appropriately scale an asteroid
 *  sprite.
 *----------------------------------------------------------------------------*/
uint16_t sizeToPix(int8_t size) {
	switch (size) {
		case 3:
		    return AST_SIZE_3;
		case 2:
		    return AST_SIZE_2;
		case 1:
		    return AST_SIZE_1;
		default:
		    return AST_SIZE_3 << 2;
	}
	return AST_SIZE_3 << 2;
}

/*------------------------------------------------------------------------------
 * Function: createBullet
 *
 * Description: This function creates a new bullet object.
 *
 * param x: The starting x position of the new bullet sprite.
 * param y: The starting y position of the new bullet sprite.
 * param velx: The new bullet's x velocity.
 * param vely: The new bullet's y velocity.
 * param nxt: A pointer to the next bullet object in a linked list of bullets.
 * return: A pointer to a malloc'd bullet object. This pointer must be freed by
 *  the caller.
 *----------------------------------------------------------------------------*/
object *createBullet(float x, float y, float velx, float vely, object *nxt) 
{
	/* ToDo:
     * Create a new bullet object using a reentrant malloc() function
     * Setup the pointers in the linked list using:
     bullet->next = nxt;
     * Create a new sprite using xSpriteCreate()
     */
	 
	 if(pvPortMalloc(sizeof(object)) != NULL)
	 {
		bullet.pos.x = x;
		bullet.pos.y = y;
		bullet.vel.x = velx;
		bullet.vel.y = vely;
		bullet -> next = nxt;
		
		xSpriteCreate("bullet.png",(uint16_t)bullet.pos.x ,(uint16_t)bullet.pos.y, 0, BULLET_SIZE, BULLET_SIZE, 1);
	 }
}

/*------------------------------------------------------------------------------
 * Function: spawnAsteroid
 *
 * Description: This function decomposes a larger asteroid into three smaller
 *  ones with random velocities and appends them to the global list of
 *  asteroids.
 *
 * param pos: A pointer to the position at which the new asteroids will be
 *  created.
 * param size: The size of the asteroid being destroyed.
 *----------------------------------------------------------------------------*/
void spawnAsteroid(point *pos, uint8_t size) {
	/* ToDo:
     * Spawn 3 smaller asteroids, or no asteroids depending on the size of this asteroid
     * Use createAsteroid()
     */
}
