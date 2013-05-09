#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include "FreeRTOS.h"

#define ERROR_HANDLE 0xFF
#define ALL_GROUP 0x00

typedef uint8_t xSpriteHandle;
typedef uint8_t xGroupHandle;

void vPrint(const char *s);
void vWindowCreate(uint16_t width, uint16_t height);

xSpriteHandle xSpriteCreate(const char *filename, uint16_t xPos, uint16_t yPos,
 uint16_t rAngle, uint16_t width, uint16_t height, uint8_t order);
void vSpriteSetPosition(xSpriteHandle sprite, uint16_t x, uint16_t y);
void vSpriteSetRotation(xSpriteHandle sprite, uint16_t angle);
void vSpriteSetSize(xSpriteHandle sprite, uint16_t width, uint16_t height);
void vSpriteSetDepth(xSpriteHandle sprite, uint8_t depth);
void vSpriteDelete(xSpriteHandle sprite);

xGroupHandle xGroupCreate(void);
void vGroupAddSprite(xGroupHandle group, xSpriteHandle sprite);
void vGroupRemoveSprite(xGroupHandle group, xSpriteHandle sprite);
void vGroupDelete(xGroupHandle group);

uint8_t uCollide(xSpriteHandle sprite, xGroupHandle group,
 xSpriteHandle hits[], uint8_t hitsSize);

#endif /* GRAPHICS_H_ */