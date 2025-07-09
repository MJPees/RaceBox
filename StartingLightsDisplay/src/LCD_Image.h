#pragma once

#include <PNGdec.h>
#include "SD_Card.h"
#include "Display_ST7789.h"

#define BOOT_KEY_PIN     9
#define MAX_IMAGE_WIDTH   172 // Adjust for your images

void Search_Image(const char* directory, const char* fileExtension);
void Show_Image(const char * filePath);
void Display_Image(const char* directory, const char* fileExtension, uint16_t ID);
void Image_Next(const char* directory, const char* fileExtension);
void Image_Next_Loop(const char* directory, const char* fileExtension,uint32_t NextTime, uint8_t ledPin);
