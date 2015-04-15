/*
 $Id:$
 
 ST7565 LCD library!
 
 Copyright (C) 2010 Limor Fried, Adafruit Industries
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 
 // some of this code was written by <cstone@pobox.com> originally; it is in the public domain.
 */

//#include <Wire.h>
//#include <avr/pgmspace.h>
//#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bitmap_image.hpp"
#include "ST7565.h"

#define ST7565_STARTBYTES 1

uint8_t is_reversed = 0;

// a handy reference to where the pages are on the screen
const uint8_t pagemap[] = { 3, 2, 1, 0, 7, 6, 5, 4 };

// a 5x7 font table
// 5x7 LCD font 'flipped' for the ST7565 - public domain
uint8_t font[] = {
    0x0, 0x0, 0x0, 0x0, 0x0,       // Ascii 0
    0x7C, 0xDA, 0xF2, 0xDA, 0x7C,  //ASC(01)
    0x7C, 0xD6, 0xF2, 0xD6, 0x7C,  //ASC(02)
    0x38, 0x7C, 0x3E, 0x7C, 0x38,
    0x18, 0x3C, 0x7E, 0x3C, 0x18,
    0x38, 0xEA, 0xBE, 0xEA, 0x38,
    0x38, 0x7A, 0xFE, 0x7A, 0x38,
    0x0, 0x18, 0x3C, 0x18, 0x0,
    0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
    0x0, 0x18, 0x24, 0x18, 0x0,
    0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
    0xC, 0x12, 0x5C, 0x60, 0x70,
    0x64, 0x94, 0x9E, 0x94, 0x64,
    0x2, 0xFE, 0xA0, 0xA0, 0xE0,
    0x2, 0xFE, 0xA0, 0xA4, 0xFC,
    0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
    0xFE, 0x7C, 0x38, 0x38, 0x10,
    0x10, 0x38, 0x38, 0x7C, 0xFE,
    0x28, 0x44, 0xFE, 0x44, 0x28,
    0xFA, 0xFA, 0x0, 0xFA, 0xFA,
    0x60, 0x90, 0xFE, 0x80, 0xFE,
    0x0, 0x66, 0x91, 0xA9, 0x56,
    0x6, 0x6, 0x6, 0x6, 0x6,
    0x29, 0x45, 0xFF, 0x45, 0x29,
    0x10, 0x20, 0x7E, 0x20, 0x10,
    0x8, 0x4, 0x7E, 0x4, 0x8,
    0x10, 0x10, 0x54, 0x38, 0x10,
    0x10, 0x38, 0x54, 0x10, 0x10,
    0x78, 0x8, 0x8, 0x8, 0x8,
    0x30, 0x78, 0x30, 0x78, 0x30,
    0xC, 0x1C, 0x7C, 0x1C, 0xC,
    0x60, 0x70, 0x7C, 0x70, 0x60,
    0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xFA, 0x0, 0x0,
    0x0, 0xE0, 0x0, 0xE0, 0x0,
    0x28, 0xFE, 0x28, 0xFE, 0x28,
    0x24, 0x54, 0xFE, 0x54, 0x48,
    0xC4, 0xC8, 0x10, 0x26, 0x46,
    0x6C, 0x92, 0x6A, 0x4, 0xA,
    0x0, 0x10, 0xE0, 0xC0, 0x0,
    0x0, 0x38, 0x44, 0x82, 0x0,
    0x0, 0x82, 0x44, 0x38, 0x0,
    0x54, 0x38, 0xFE, 0x38, 0x54,
    0x10, 0x10, 0x7C, 0x10, 0x10,
    0x0, 0x1, 0xE, 0xC, 0x0,
    0x10, 0x10, 0x10, 0x10, 0x10,
    0x0, 0x0, 0x6, 0x6, 0x0,
    0x4, 0x8, 0x10, 0x20, 0x40,
    0x7C, 0x8A, 0x92, 0xA2, 0x7C,
    0x0, 0x42, 0xFE, 0x2, 0x0,
    0x4E, 0x92, 0x92, 0x92, 0x62,
    0x84, 0x82, 0x92, 0xB2, 0xCC,
    0x18, 0x28, 0x48, 0xFE, 0x8,
    0xE4, 0xA2, 0xA2, 0xA2, 0x9C,
    0x3C, 0x52, 0x92, 0x92, 0x8C,
    0x82, 0x84, 0x88, 0x90, 0xE0,
    0x6C, 0x92, 0x92, 0x92, 0x6C,
    0x62, 0x92, 0x92, 0x94, 0x78,
    0x0, 0x0, 0x28, 0x0, 0x0,
    0x0, 0x2, 0x2C, 0x0, 0x0,
    0x0, 0x10, 0x28, 0x44, 0x82,
    0x28, 0x28, 0x28, 0x28, 0x28,
    0x0, 0x82, 0x44, 0x28, 0x10,
    0x40, 0x80, 0x9A, 0x90, 0x60,
    0x7C, 0x82, 0xBA, 0x9A, 0x72,
    0x3E, 0x48, 0x88, 0x48, 0x3E,
    0xFE, 0x92, 0x92, 0x92, 0x6C,
    0x7C, 0x82, 0x82, 0x82, 0x44,
    0xFE, 0x82, 0x82, 0x82, 0x7C,
    0xFE, 0x92, 0x92, 0x92, 0x82,
    0xFE, 0x90, 0x90, 0x90, 0x80,
    0x7C, 0x82, 0x82, 0x8A, 0xCE,
    0xFE, 0x10, 0x10, 0x10, 0xFE,
    0x0, 0x82, 0xFE, 0x82, 0x0,
    0x4, 0x2, 0x82, 0xFC, 0x80,
    0xFE, 0x10, 0x28, 0x44, 0x82,
    0xFE, 0x2, 0x2, 0x2, 0x2,
    0xFE, 0x40, 0x38, 0x40, 0xFE,
    0xFE, 0x20, 0x10, 0x8, 0xFE,
    0x7C, 0x82, 0x82, 0x82, 0x7C,
    0xFE, 0x90, 0x90, 0x90, 0x60,
    0x7C, 0x82, 0x8A, 0x84, 0x7A,
    0xFE, 0x90, 0x98, 0x94, 0x62,
    0x64, 0x92, 0x92, 0x92, 0x4C,
    0xC0, 0x80, 0xFE, 0x80, 0xC0,
    0xFC, 0x2, 0x2, 0x2, 0xFC,
    0xF8, 0x4, 0x2, 0x4, 0xF8,
    0xFC, 0x2, 0x1C, 0x2, 0xFC,
    0xC6, 0x28, 0x10, 0x28, 0xC6,
    0xC0, 0x20, 0x1E, 0x20, 0xC0,
    0x86, 0x9A, 0x92, 0xB2, 0xC2,
    0x0, 0xFE, 0x82, 0x82, 0x82,
    0x40, 0x20, 0x10, 0x8, 0x4,
    0x0, 0x82, 0x82, 0x82, 0xFE,
    0x20, 0x40, 0x80, 0x40, 0x20,
    0x2, 0x2, 0x2, 0x2, 0x2,
    0x0, 0xC0, 0xE0, 0x10, 0x0,
    0x4, 0x2A, 0x2A, 0x1E, 0x2,
    0xFE, 0x14, 0x22, 0x22, 0x1C,
    0x1C, 0x22, 0x22, 0x22, 0x14,
    0x1C, 0x22, 0x22, 0x14, 0xFE,
    0x1C, 0x2A, 0x2A, 0x2A, 0x18,
    0x0, 0x10, 0x7E, 0x90, 0x40,
    0x18, 0x25, 0x25, 0x39, 0x1E,
    0xFE, 0x10, 0x20, 0x20, 0x1E,
    0x0, 0x22, 0xBE, 0x2, 0x0,
    0x4, 0x2, 0x2, 0xBC, 0x0,
    0xFE, 0x8, 0x14, 0x22, 0x0,
    0x0, 0x82, 0xFE, 0x2, 0x0,
    0x3E, 0x20, 0x1E, 0x20, 0x1E,
    0x3E, 0x10, 0x20, 0x20, 0x1E,
    0x1C, 0x22, 0x22, 0x22, 0x1C,
    0x3F, 0x18, 0x24, 0x24, 0x18,
    0x18, 0x24, 0x24, 0x18, 0x3F,
    0x3E, 0x10, 0x20, 0x20, 0x10,
    0x12, 0x2A, 0x2A, 0x2A, 0x24,
    0x20, 0x20, 0xFC, 0x22, 0x24,
    0x3C, 0x2, 0x2, 0x4, 0x3E,
    0x38, 0x4, 0x2, 0x4, 0x38,
    0x3C, 0x2, 0xC, 0x2, 0x3C,
    0x22, 0x14, 0x8, 0x14, 0x22,
    0x32, 0x9, 0x9, 0x9, 0x3E,
    0x22, 0x26, 0x2A, 0x32, 0x22,
    0x0, 0x10, 0x6C, 0x82, 0x0,
    0x0, 0x0, 0xEE, 0x0, 0x0,
    0x0, 0x82, 0x6C, 0x10, 0x0,
    0x40, 0x80, 0x40, 0x20, 0x40,
    0x3C, 0x64, 0xC4, 0x64, 0x3C,
    0x78, 0x85, 0x85, 0x86, 0x48,
    0x5C, 0x2, 0x2, 0x4, 0x5E,
    0x1C, 0x2A, 0x2A, 0xAA, 0x9A,
    0x84, 0xAA, 0xAA, 0x9E, 0x82,
    0x84, 0x2A, 0x2A, 0x1E, 0x82,
    0x84, 0xAA, 0x2A, 0x1E, 0x2,
    0x4, 0x2A, 0xAA, 0x9E, 0x2,
    0x30, 0x78, 0x4A, 0x4E, 0x48,
    0x9C, 0xAA, 0xAA, 0xAA, 0x9A,
    0x9C, 0x2A, 0x2A, 0x2A, 0x9A,
    0x9C, 0xAA, 0x2A, 0x2A, 0x1A,
    0x0, 0x0, 0xA2, 0x3E, 0x82,
    0x0, 0x40, 0xA2, 0xBE, 0x42,
    0x0, 0x80, 0xA2, 0x3E, 0x2,
    0xF, 0x94, 0x24, 0x94, 0xF,
    0xF, 0x14, 0xA4, 0x14, 0xF,
    0x3E, 0x2A, 0xAA, 0xA2, 0x0,
    0x4, 0x2A, 0x2A, 0x3E, 0x2A,
    0x3E, 0x50, 0x90, 0xFE, 0x92,
    0x4C, 0x92, 0x92, 0x92, 0x4C,
    0x4C, 0x12, 0x12, 0x12, 0x4C,
    0x4C, 0x52, 0x12, 0x12, 0xC,
    0x5C, 0x82, 0x82, 0x84, 0x5E,
    0x5C, 0x42, 0x2, 0x4, 0x1E,
    0x0, 0xB9, 0x5, 0x5, 0xBE,
    0x9C, 0x22, 0x22, 0x22, 0x9C,
    0xBC, 0x2, 0x2, 0x2, 0xBC,
    0x3C, 0x24, 0xFF, 0x24, 0x24,
    0x12, 0x7E, 0x92, 0xC2, 0x66,
    0xD4, 0xF4, 0x3F, 0xF4, 0xD4,
    0xFF, 0x90, 0x94, 0x6F, 0x4,
    0x3, 0x11, 0x7E, 0x90, 0xC0,
    0x4, 0x2A, 0x2A, 0x9E, 0x82,
    0x0, 0x0, 0x22, 0xBE, 0x82,
    0xC, 0x12, 0x12, 0x52, 0x4C,
    0x1C, 0x2, 0x2, 0x44, 0x5E,
    0x0, 0x5E, 0x50, 0x50, 0x4E,
    0xBE, 0xB0, 0x98, 0x8C, 0xBE,
    0x64, 0x94, 0x94, 0xF4, 0x14,
    0x64, 0x94, 0x94, 0x94, 0x64,
    0xC, 0x12, 0xB2, 0x2, 0x4,
    0x1C, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x1C,
    0xF4, 0x8, 0x13, 0x35, 0x5D,
    0xF4, 0x8, 0x14, 0x2C, 0x5F,
    0x0, 0x0, 0xDE, 0x0, 0x0,
    0x10, 0x28, 0x54, 0x28, 0x44,
    0x44, 0x28, 0x54, 0x28, 0x10,
    0x55, 0x0, 0xAA, 0x0, 0x55,
    0x55, 0xAA, 0x55, 0xAA, 0x55,
    0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0x0, 0x0, 0x0, 0xFF, 0x0,
    0x8, 0x8, 0x8, 0xFF, 0x0,
    0x28, 0x28, 0x28, 0xFF, 0x0,
    0x8, 0x8, 0xFF, 0x0, 0xFF,
    0x8, 0x8, 0xF, 0x8, 0xF,
    0x28, 0x28, 0x28, 0x3F, 0x0,
    0x28, 0x28, 0xEF, 0x0, 0xFF,
    0x0, 0x0, 0xFF, 0x0, 0xFF,
    0x28, 0x28, 0x2F, 0x20, 0x3F,
    0x28, 0x28, 0xE8, 0x8, 0xF8,
    0x8, 0x8, 0xF8, 0x8, 0xF8,
    0x28, 0x28, 0x28, 0xF8, 0x0,
    0x8, 0x8, 0x8, 0xF, 0x0,
    0x0, 0x0, 0x0, 0xF8, 0x8,
    0x8, 0x8, 0x8, 0xF8, 0x8,
    0x8, 0x8, 0x8, 0xF, 0x8,
    0x0, 0x0, 0x0, 0xFF, 0x8,
    0x8, 0x8, 0x8, 0x8, 0x8,
    0x8, 0x8, 0x8, 0xFF, 0x8,
    0x0, 0x0, 0x0, 0xFF, 0x28,
    0x0, 0x0, 0xFF, 0x0, 0xFF,
    0x0, 0x0, 0xF8, 0x8, 0xE8,
    0x0, 0x0, 0x3F, 0x20, 0x2F,
    0x28, 0x28, 0xE8, 0x8, 0xE8,
    0x28, 0x28, 0x2F, 0x20, 0x2F,
    0x0, 0x0, 0xFF, 0x0, 0xEF,
    0x28, 0x28, 0x28, 0x28, 0x28,
    0x28, 0x28, 0xEF, 0x0, 0xEF,
    0x28, 0x28, 0x28, 0xE8, 0x28,
    0x8, 0x8, 0xF8, 0x8, 0xF8,
    0x28, 0x28, 0x28, 0x2F, 0x28,
    0x8, 0x8, 0xF, 0x8, 0xF,
    0x0, 0x0, 0xF8, 0x8, 0xF8,
    0x0, 0x0, 0x0, 0xF8, 0x28,
    0x0, 0x0, 0x0, 0x3F, 0x28,
    0x0, 0x0, 0xF, 0x8, 0xF,
    0x8, 0x8, 0xFF, 0x8, 0xFF,
    0x28, 0x28, 0x28, 0xFF, 0x28,
    0x8, 0x8, 0x8, 0xF8, 0x0,
    0x0, 0x0, 0x0, 0xF, 0x8,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF, 0xF, 0xF, 0xF, 0xF,
    0xFF, 0xFF, 0xFF, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xFF, 0xFF,
    0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
    0x1C, 0x22, 0x22, 0x1C, 0x22,
    0x3E, 0x54, 0x54, 0x7C, 0x28,
    0x7E, 0x40, 0x40, 0x60, 0x60,
    0x40, 0x7E, 0x40, 0x7E, 0x40,
    0xC6, 0xAA, 0x92, 0x82, 0xC6,
    0x1C, 0x22, 0x22, 0x3C, 0x20,
    0x2, 0x7E, 0x4, 0x78, 0x4,
    0x60, 0x40, 0x7E, 0x40, 0x40,
    0x99, 0xA5, 0xE7, 0xA5, 0x99,
    0x38, 0x54, 0x92, 0x54, 0x38,
    0x32, 0x4E, 0x80, 0x4E, 0x32,
    0xC, 0x52, 0xB2, 0xB2, 0xC,
    0xC, 0x12, 0x1E, 0x12, 0xC,
    0x3D, 0x46, 0x5A, 0x62, 0xBC,
    0x7C, 0x92, 0x92, 0x92, 0x0,
    0x7E, 0x80, 0x80, 0x80, 0x7E,
    0x54, 0x54, 0x54, 0x54, 0x54,
    0x22, 0x22, 0xFA, 0x22, 0x22,
    0x2, 0x8A, 0x52, 0x22, 0x2,
    0x2, 0x22, 0x52, 0x8A, 0x2,
    0x0, 0x0, 0xFF, 0x80, 0xC0,
    0x7, 0x1, 0xFF, 0x0, 0x0,
    0x10, 0x10, 0xD6, 0xD6, 0x10,
    0x6C, 0x48, 0x6C, 0x24, 0x6C,
    0x60, 0xF0, 0x90, 0xF0, 0x60,
    0x0, 0x0, 0x18, 0x18, 0x0,
    0x0, 0x0, 0x8, 0x8, 0x0,
    0xC, 0x2, 0xFF, 0x80, 0x80,
    0x0, 0xF8, 0x80, 0x80, 0x78,
    0x0, 0x98, 0xB8, 0xE8, 0x48,
    0x0, 0x3C, 0x3C, 0x3C, 0x3C,};


// the memory buffer for the LCD
// reduces how much is refreshed, which speeds it up!
// originally derived from Steve Evans/JCW's mod but cleaned up and
// optimized
#define enablePartialUpdate

#ifdef enablePartialUpdate
static uint8_t xUpdateMin, xUpdateMax, yUpdateMin, yUpdateMax;
static uint8_t get_x0bound(){
    return xUpdateMin;
}

static uint8_t get_x1bound(){
    return xUpdateMax;
}

static uint8_t get_y0bound(){
    return yUpdateMin;
}

static uint8_t get_y1bound(){
    return yUpdateMax;
}

#endif

uint8_t * ST7565::get_st7565_buffer(){
    return st7565_buffer;
}


static void updateBoundingBox(uint8_t xmin, uint8_t ymin, uint8_t xmax, uint8_t ymax) {
#ifdef enablePartialUpdate
    if (xmin < xUpdateMin) xUpdateMin = xmin;
    if (xmax > xUpdateMax) xUpdateMax = xmax;
    if (ymin < yUpdateMin) yUpdateMin = ymin;
    if (ymax > yUpdateMax) yUpdateMax = ymax;
#endif
}

void ST7565::drawbitmap(uint8_t x, uint8_t y,
                        const uint8_t *bitmap, uint8_t w, uint8_t h,
                        uint8_t color) {
    for (uint8_t j=0; j<h; j++) {
        for (uint8_t i=0; i<w; i++ ) {
            if((bitmap[i + (j/8) * w]) & (1<<(j % 8))) {
                my_setpixel(x+i, y+j, color);
            }
        }
    }
    
    updateBoundingBox(x, y, x+w, y+h);
}

void ST7565::drawpixel(uint8_t x, uint8_t y, uint8_t color) {
    my_setpixel(x, y, color);
    updateBoundingBox(x, y, x+1, y+1);
}


void ST7565::savebitmap(std::string filename){

     bitmap_image image(128, 64);
     
     for (unsigned int x = 0; x < 128; ++x)
     {
         for (unsigned int y = 0; y < 8; ++y)
         {
             for(int j=0; j < 8;j++){
                 if((st7565_buffer[x + (y * 128)] & (0x01 << j)) == 0){
                     continue;
                 }
                 image.set_pixel(x,y*8+ (8-j),255,255,255);
             }
         }
     }
     
     
     image.save_image(filename);
}

void ST7565::drawstring(uint8_t x, uint8_t line, char *c) {
    while (c[0] != 0) {
        drawchar(x, line, c[0]);
        c++;
        x += 6; // 6 pixels wide
        if (x + 6 >= LCDWIDTH) {
            x = 0;    // ran out of this line
            line++;
        }
        if (line >= (LCDHEIGHT/8))
            return;        // ran out of space :(
    }
}


void ST7565::drawstring_P(uint8_t x, uint8_t line, const char *str) {
    while (1) {
        char c = *str++;
        if (! c)
            return;
        drawchar(x, line, c);
        x += 6; // 6 pixels wide
        if (x + 6 >= LCDWIDTH) {
            x = 0;    // ran out of this line
            line++;
        }
        if (line >= (LCDHEIGHT/8))
            return;        // ran out of space :(
    }
}

void  ST7565::drawchar(uint8_t x, uint8_t line, char c) {
    for (uint8_t i = 0; i<5; i++ ) {
        st7565_buffer[x + (line * 128) ] = font[(c * 5) + i];
        x++;
    }
    
    updateBoundingBox(x, line*8, x+5, line*8 + 8);
}


// bresenham's algorithm - thx wikpedia
void ST7565::drawline(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                      uint8_t color) {
    uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }
    
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }
    
    // much faster to put the test here, since we've already sorted the points
    updateBoundingBox(x0, y0, x1, y1);
    
    uint8_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);
    
    int8_t err = dx / 2;
    int8_t ystep;
    
    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;}
    
    for (; x0<=x1; x0++) {
        if (steep) {
            my_setpixel(y0, x0, color);
        } else {
            my_setpixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// filled rectangle
void ST7565::fillrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                      uint8_t color) {
    
    // stupidest version - just pixels - but fast with internal buffer!
    for (uint8_t i=x; i<x+w; i++) {
        for (uint8_t j=y; j<y+h; j++) {
            my_setpixel(i, j, color);
        }
    }
    
    updateBoundingBox(x, y, x+w, y+h);
}

// draw a rectangle
void ST7565::drawrect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                      uint8_t color) {
    // stupidest version - just pixels - but fast with internal buffer!
    for (uint8_t i=x; i<x+w; i++) {
        my_setpixel(i, y, color);
        my_setpixel(i, y+h-1, color);
    }
    for (uint8_t i=y; i<y+h; i++) {
        my_setpixel(x, i, color);
        my_setpixel(x+w-1, i, color);
    }
    
    updateBoundingBox(x, y, x+w, y+h);
}

// draw a circle outline
void ST7565::drawcircle(uint8_t x0, uint8_t y0, uint8_t r,
                        uint8_t color) {
    updateBoundingBox(x0-r, y0-r, x0+r, y0+r);
    
    int8_t f = 1 - r;
    int8_t ddF_x = 1;
    int8_t ddF_y = -2 * r;
    int8_t x = 0;
    int8_t y = r;
    
    my_setpixel(x0, y0+r, color);
    my_setpixel(x0, y0-r, color);
    my_setpixel(x0+r, y0, color);
    my_setpixel(x0-r, y0, color);
    
    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        my_setpixel(x0 + x, y0 + y, color);
        my_setpixel(x0 - x, y0 + y, color);
        my_setpixel(x0 + x, y0 - y, color);
        my_setpixel(x0 - x, y0 - y, color);
        
        my_setpixel(x0 + y, y0 + x, color);
        my_setpixel(x0 - y, y0 + x, color);
        my_setpixel(x0 + y, y0 - x, color);
        my_setpixel(x0 - y, y0 - x, color);
        
    }
    
    
    
}

void ST7565::fillcircle(uint8_t x0, uint8_t y0, uint8_t r,
                        uint8_t color) {
    updateBoundingBox(x0-r, y0-r, x0+r, y0+r);
    
    int8_t f = 1 - r;
    int8_t ddF_x = 1;
    int8_t ddF_y = -2 * r;
    int8_t x = 0;
    int8_t y = r;
    
    for (uint8_t i=y0-r; i<=y0+r; i++) {
        my_setpixel(x0, i, color);
    }
    
    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        for (uint8_t i=y0-y; i<=y0+y; i++) {
            my_setpixel(x0+x, i, color);
            my_setpixel(x0-x, i, color);
        }
        for (uint8_t i=y0-x; i<=y0+x; i++) {
            my_setpixel(x0+y, i, color);
            my_setpixel(x0-y, i, color);
        }
    }
}

void ST7565::my_setpixel(uint8_t x, uint8_t y, uint8_t color) {
    if ((x >= LCDWIDTH) || (y >= LCDHEIGHT))
        return;
    
    // x is which column
    if (color)
        st7565_buffer[x+ (y/8)*128] |= (1<<(7-(y%8)));
    else
        st7565_buffer[x+ (y/8)*128] &= ~(1<<(7-(y%8)));
}

// the most basic function, set a single pixel
void ST7565::setpixel(uint8_t x, uint8_t y, uint8_t color) {
    if ((x >= LCDWIDTH) || (y >= LCDHEIGHT))
        return;
    
    // x is which column
    if (color)
        st7565_buffer[x+ (y/8)*128] |= (1<<(7-(y%8)));
    else
        st7565_buffer[x+ (y/8)*128] &= ~(1<<(7-(y%8)));
    
    updateBoundingBox(x,y,x,y);
}



// the most basic function, get a single pixel
uint8_t ST7565::getpixel(uint8_t x, uint8_t y) {
    if ((x >= LCDWIDTH) || (y >= LCDHEIGHT))
        return 0;
    
    return (st7565_buffer[x+ (y/8)*128] >> (7-(y%8))) & 0x1;
}

void ST7565::begin(uint8_t contrast) {
    //wiringPiSetupGpio();
    
    st7565_init();
    //st7565_set_brightness(contrast);
    // st7565_command(CMD_DISPLAY_ON);
    //st7565_command(CMD_SET_ALLPTS_NORMAL);
}

void ST7565::st7565_init(void) {
    // set pin directions
    // pinMode(sid, OUTPUT);
    // pinMode(sclk, OUTPUT);
    // pinMode(a0, OUTPUT);
    // pinMode(rst, OUTPUT);
    // pinMode(cs, OUTPUT);
    
    // // toggle RST low to reset; CS low so it'll listen to us
    // if (cs > 0)
    //   digitalWrite(cs, LOW);
    
    // digitalWrite(rst, LOW);
    // usleep(500000);
    // digitalWrite(rst, HIGH);
    
    // // LCD bias select
    // st7565_command(CMD_SET_BIAS_9);
    // // ADC select
    // st7565_command(CMD_SET_ADC_NORMAL);
    // // SHL select
    // st7565_command(CMD_SET_COM_REVERSE);
    // st7565_command(CMD_SET_COM_NORMAL);
    // // Initial display line
    // st7565_command(CMD_SET_DISP_START_LINE);
    /*
     // turn on voltage converter (VC=1, VR=0, VF=0)
     st7565_command(CMD_SET_POWER_CONTROL | 0x4);
     // wait for 50% rising
     usleep(50000);
     
     // turn on voltage regulator (VC=1, VR=1, VF=0)
     st7565_command(CMD_SET_POWER_CONTROL | 0x6);
     // wait >=50ms
     usleep(50000);
     
     // turn on voltage follower (VC=1, VR=1, VF=1)
     st7565_command(CMD_SET_POWER_CONTROL | 0x7);
     // wait
     usleep(10000);
     
     // set lcd operating voltage (regulator resistor, ref voltage resistor)
     st7565_command(CMD_SET_RESISTOR_RATIO | 0x6);
     
     // initial display line
     // set page address
     // set column address
     // write display data
     
     // set up a bounding box for screen updates
     */
    // st7565_command(0x25);
    // st7565_command(0x81);
    // st7565_command(0x19);
    // st7565_command(0x2F);
    updateBoundingBox(0, 0, LCDWIDTH-1, LCDHEIGHT-1);
}

inline void ST7565::spiwrite(uint8_t c) {
    // shiftOut(sid, sclk, MSBFIRST, c);
    /*
     int8_t i;
     for (i=7; i>=0; i--) {
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(i))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     }
     */
    
    /*
     // loop unwrapped! too fast doesnt work :(
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(7))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(6))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(5))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(4))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(3))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(2))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(1))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     
     SCLK_PORT &= ~_BV(SCLK);
     if (c & _BV(0))
     SID_PORT |= _BV(SID);
     else
     SID_PORT &= ~_BV(SID);
     SCLK_PORT |= _BV(SCLK);
     */
    
}
void ST7565::st7565_command(uint8_t c) {
    // digitalWrite(a0, LOW);
    // spiwrite(c);
}

void ST7565::st7565_data(uint8_t c) {
    // digitalWrite(a0, HIGH);
    
    // spiwrite(c);
}
void ST7565::st7565_set_brightness(uint8_t val) {
    // st7565_command(CMD_SET_VOLUME_FIRST);
    // st7565_command(CMD_SET_VOLUME_SECOND | (val & 0x3f));
}


void ST7565::display(void) {
    uint8_t col, maxcol, p;
    
    /*
     Serial.print("Refresh ("); Serial.print(xUpdateMin, DEC);
     Serial.print(", "); Serial.print(xUpdateMax, DEC);
     Serial.print(","); Serial.print(yUpdateMin, DEC);
     Serial.print(", "); Serial.print(yUpdateMax, DEC); Serial.println(")");
     */
    
    for(p = 0; p < 8; p++) {
        /*
         putstring("new page! ");
         uart_putw_dec(p);
         putstring_nl("");
         */
#ifdef enablePartialUpdate
        // check if this page is part of update
        if ( yUpdateMin >= ((p+1)*8) ) {
            continue;   // nope, skip it!
        }
        if (yUpdateMax < p*8) {
            break;
        }
#endif
        
        st7565_command(CMD_SET_PAGE | pagemap[p]);
        
        
#ifdef enablePartialUpdate
        col = xUpdateMin;
        maxcol = xUpdateMax;
#else
        // start at the beginning of the row
        col = 0;
        maxcol = LCDWIDTH-1;
#endif
        
//        st7565_command(CMD_SET_COLUMN_LOWER | ((col+ST7565_STARTBYTES) & 0xf));
//        st7565_command(CMD_SET_COLUMN_UPPER | (((col+ST7565_STARTBYTES) >> 4) & 0x0F));
//        st7565_command(CMD_RMW);
        
//        for(; col <= maxcol; col++) {
            //uart_putw_dec(col);
            //uart_putchar(' ');
//            st7565_data(st7565_buffer[(128*p)+col]);
//        }
    }
    
#ifdef enablePartialUpdate
    xUpdateMin = LCDWIDTH - 1;
    xUpdateMax = 0;
    yUpdateMin = LCDHEIGHT-1;
    yUpdateMax = 0;
#endif
}

// clear everything
void ST7565::clear(void) {
    memset(st7565_buffer, 0, 1024);
    updateBoundingBox(0, 0, LCDWIDTH-1, LCDHEIGHT-1);
}

ST7565::ST7565(int8_t JSD){
    memset(st7565_buffer, 0, 1024);
}


// this doesnt touch the buffer, just clears the display RAM - might be handy
void ST7565::clear_display(void) {
    uint8_t p, c;
    
    for(p = 0; p < 8; p++) {
        /*
         putstring("new page! ");
         uart_putw_dec(p);
         putstring_nl("");
         */
        
        st7565_command(CMD_SET_PAGE | p);
        for(c = 0; c < 129; c++) {
            //uart_putw_dec(c);
            //uart_putchar(' ');
            st7565_command(CMD_SET_COLUMN_LOWER | (c & 0xf));
            st7565_command(CMD_SET_COLUMN_UPPER | ((c >> 4) & 0xf));
            st7565_data(0x0);
        }     
    }
}
