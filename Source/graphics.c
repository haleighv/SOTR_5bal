#include "graphics.h"
#include "usart.h"

/* Sprite functions */
#define CREATE_SPRITE       0x01
#define SET_POS             0x02
#define SET_ROT             0x03
#define SET_ORDER           0x0C
#define SET_SIZE            0x0D
#define DELETE_SPRITE       0x04

/* Collision functions */
#define CREATE_GROUP        0x05
#define ADD_TO_GROUP        0x06
#define REMOVE_FROM_GROUP   0x07
#define DELETE_GROUP        0x08
#define COLLIDE             0x09

#define CREATE_WINDOW       0x0A
#define PYTHON_PRINT        0x0B

#define BAUD_RATE			38400

/*******************************************************************************
* Function: vPrint
*
* Description: Prints the supplied string to the python terminal.  Useful for 
*  debugging.
*
* param s: The string to print out.
*******************************************************************************/
void vPrint(const char *s) {
	USART_Write(PYTHON_PRINT);
	while (*s != '\0') {
		USART_Write((uint8_t)*s++);
	}
	USART_Write(0x00);  /* string is null-terminated */
}

/*******************************************************************************
* Function: vWindowCreate
*
* Description: Initializes the communication line to the external graphics
*  context and opens a window for drawing with the specified dimensions.
*
* param width: Desired width of the window in pixels
* param height: Desired height of the window in pixels
*******************************************************************************/
void vWindowCreate(uint16_t width, uint16_t height) {
	USART_Init(BAUD_RATE, configCPU_CLOCK_HZ);

	USART_Read();
	USART_Write_Unprotected(0xFF);
	
	USART_Write_Unprotected(CREATE_WINDOW);
	USART_Write_Unprotected(width >> 8);
	USART_Write_Unprotected(width & 0x00FF);
	USART_Write_Unprotected(height >> 8);
	USART_Write_Unprotected(height & 0x00FF);
}

/*******************************************************************************
* Function: xSpriteCreate
*
* Description: Instantiates a sprite in the external graphics context using the
*  contents of the given external file with the given position, angle, size, and
*  depth in the window. The window origin is in the upper-left corner.
*
* param filename: Null-terminated string containing the name of the sprite image
*  file in the external graphics context.
* param xPos: Initial x-position of the center of the sprite in window coords
* param yPos: Initial y-position of the center of the sprite in window coords
* param rAngle: Initial CCW rotation of the sprite about its center in degrees
* param width: Initial, unrotated width of the sprite in pixels
* param height: Initial, unrotated height of the sprite in pixels
* param depth: Initial draw depth of the sprite (larger numbers are in front)
* return: A valid handle to the new sprite on success; ERROR_HANDLE otherwise
*******************************************************************************/
xSpriteHandle xSpriteCreate(const char *filename, uint16_t xPos, uint16_t yPos,
 uint16_t rAngle, uint16_t width, uint16_t height, uint8_t depth) {
	
	USART_Write(CREATE_SPRITE);

	while (*filename != '\0') {
		USART_Write((uint8_t)*filename++);
	}
	USART_Write(0x00);  /* Filename is null-terminated */

	USART_Write(xPos >> 8);
	USART_Write(xPos & 0x00FF);
	USART_Write(yPos >> 8);
	USART_Write(yPos & 0x00FF);
	USART_Write(rAngle >> 8);
	USART_Write(rAngle & 0x00FF);
	USART_Write(width >> 8);
	USART_Write(width & 0x00FF);
	USART_Write(height >> 8);
	USART_Write(height & 0x00FF);
	USART_Write(depth);
	
	PORTA = 0xAA;
	xSpriteHandle result = (xSpriteHandle)USART_Read();
	PORTA = 0xFF;
	
	return result;
}

/*******************************************************************************
* Function: vSpriteSetPosition
*
* Description: Sets the given sprite's position in the window. The window origin
*  is in the upper-left corner.
*
* param sprite: The handle to the sprite
* param x: New x-position of the sprite's center in window coordinates
* param y: New y-position of the sprite's center in window coordinates
*******************************************************************************/
void vSpriteSetPosition(xSpriteHandle sprite, uint16_t x, uint16_t y) {
	USART_Write(SET_POS);
	USART_Write(sprite);
	USART_Write(x >> 8);
	USART_Write(x & 0x00FF);
	USART_Write(y >> 8);
	USART_Write(y & 0x00FF);
}

/*******************************************************************************
* Function: vSpriteSetRotation
*
* Description: Sets the given sprite's rotation.
*
* param sprite: The handle to the sprite
* param angle: Angle in degrees to rotate the sprite CCW about its center
*******************************************************************************/
void vSpriteSetRotation(xSpriteHandle sprite, uint16_t angle) {
	USART_Write(SET_ROT);
	USART_Write(sprite);
	USART_Write(angle >> 8);
	USART_Write(angle & 0x00FF);
}

/*******************************************************************************
* Function: vSpriteSetSize
*
* Description: Sets the given sprite's unrotated extents.
*
* param sprite: The handle to the sprite
* param width: New width of the sprite in pixels before applying rotation
* param height: New height of the sprite in pixels before applying rotation
*******************************************************************************/
void vSpriteSetSize(xSpriteHandle sprite, uint16_t width, uint16_t height) {
	USART_Write(SET_SIZE);
	USART_Write(sprite);
	USART_Write(width >> 8);
	USART_Write(width & 0x00FF);
	USART_Write(height >> 8);
	USART_Write(height & 0x00FF);
}

/*******************************************************************************
* Function: vSpriteSetDepth
*
* Description: Sets the draw depth of the given sprite.
*
* param sprite: The handle to the sprite
* param depth: New draw depth (larger depths are in front of smaller depths)
*******************************************************************************/
void vSpriteSetDepth(xSpriteHandle sprite, uint8_t depth) {
	USART_Write(SET_ORDER);
	USART_Write(sprite);
	USART_Write(depth);
}

/*******************************************************************************
* Function: vSpriteDelete
*
* Description: Removes the sprite from the window and invalidates the given
*  handle.
*
* param sprite: The handle to the sprite to be deleted
*******************************************************************************/
void vSpriteDelete(xSpriteHandle sprite) {
	USART_Write(DELETE_SPRITE);
	USART_Write(sprite);
}

/*******************************************************************************
* Function: xGroupCreate
*
* Description: Instantiates an empty sprite group. Sprite groups are useful for
*  collision tests (see bCollide).
*
* return: A valid handle to the new group on success; ERROR_HANDLE otherwise
*******************************************************************************/
xGroupHandle xGroupCreate(void) {
	USART_Write(CREATE_GROUP);
	xGroupHandle result = (xGroupHandle)USART_Read();
	
	return result;
}

/*******************************************************************************
* Function: vGroupAddSprite
*
* Description: Adds the given sprite to the given group.
*
* param group: The handle to the group to add the sprite to
* param sprite: The handle to the sprite to add to the group
*******************************************************************************/
void vGroupAddSprite(xGroupHandle group, xSpriteHandle sprite) {
	USART_Write(ADD_TO_GROUP);
	USART_Write(group);
	USART_Write(sprite);
}

/*******************************************************************************
* Function: vGroupRemoveSprite
*
* Description: Removes the given sprite from the given group.
*
* param group: The handle to the group to remove the sprite from
* param sprite: The handle to the sprite to remove from the group
*******************************************************************************/
void vGroupRemoveSprite(xGroupHandle group, xSpriteHandle sprite) {
	USART_Write(REMOVE_FROM_GROUP);
	USART_Write(group);
	USART_Write(sprite);
}

/*******************************************************************************
* Function: vGroupDelete
*
* Description: Invalidates the given group handle and removes all the sprites
*  it contains from it.
*
* param group: The handle to the group to be deleted
*******************************************************************************/
void vGroupDelete(xGroupHandle group) {
	USART_Write(DELETE_GROUP);
	USART_Write(group);
}

/*******************************************************************************
* Function: uCollide
*
* Description: Tests if the given sprite collided with any members of the given
*  group and populates the provided hits array with sprite handles the sprite
*  collided with.
*
* param sprite: The handle to the sprite to test for collisions
* param group: The handle to the group of sprites to test for collisions
*  against sprite
* param hits: An array in which to store the sprite handles from group that
*  sprite collided with
* param hitsSize: The size of the hits array
* return: The number of hits stored in the hits array
*******************************************************************************/
uint8_t uCollide(xSpriteHandle sprite, xGroupHandle group,
 xSpriteHandle hits[], uint8_t hitsSize) {
	uint8_t hitCount = 0;
	
	USART_Write(COLLIDE);
	USART_Write(sprite);
	USART_Write(group);
	
	while (hitCount < hitsSize) {
		hits[hitCount] = USART_Read();
		if (hits[hitCount] == ERROR_HANDLE) {
			return hitCount;
		}
		hitCount++;
	}
	
	while (USART_Read() != ERROR_HANDLE)
	    hitCount++;
		
	return hitCount;
}	