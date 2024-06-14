//
// Created by cbmeeks on 11/27/23.
//

#ifndef VGAPICO_SPRITE_H
#define VGAPICO_SPRITE_H

#include <stdint-gcc.h>
#include <stdbool.h>

#define NUM_SPRITES 128
#define MAX_SPRITES_PER_LINE 16

typedef struct {
    uint16_t *data_ptr;
    unsigned int width;
    unsigned int height;
    int x;
    int y;
    bool enabled;
} sprite_info_t;


sprite_info_t sprites[NUM_SPRITES];

int initSprites();

#endif //VGAPICO_SPRITE_H
