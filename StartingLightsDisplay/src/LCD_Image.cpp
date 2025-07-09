#include "LCD_Image.h"

PNG png;
File Image_file;

uint16_t Image_CNT;
char SD_Image_Name[100][100] ;
char File_Image_Name[100][100] ;

int16_t xpos = 0;
int16_t ypos = 0;
void * pngOpen(const char *filePath, int32_t *size) {
  Image_file = SD.open(filePath);
  *size = Image_file.size();
  return &Image_file;
}

void pngClose(void *handle) {
  File Image_file = *((File*)handle);
  if (Image_file) Image_file.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length) {
  if (!Image_file) return 0;
  page = page; // Avoid warning
  return Image_file.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position) {
  if (!Image_file) return 0;
  page = page; // Avoid warning
  return Image_file.seek(position);
}
//=========================================v==========================================
//                                      pngDraw
//====================================================================================
// This next function will be called during decoding of the png file to
// render each image line to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
// Callback function to draw pixels to the display
static uint16_t lineBuffer[MAX_IMAGE_WIDTH];
void pngDraw(PNGDRAW *pDraw) {
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  uint32_t size = pDraw->iWidth;
  for (size_t i = 0; i < size; i++) {
    lineBuffer[i] = (((lineBuffer[i] >> 8) & 0xFF) | ((lineBuffer[i] << 8) & 0xFF00));        //  所有数据修正
  }
  LCD_addWindow(xpos, pDraw->y, xpos + pDraw->iWidth, ypos + pDraw->y + 1,lineBuffer);                   // x_end End index on x-axis (x_end not included)

}
/////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

void Search_Image(const char* directory, const char* fileExtension) {
  Image_CNT = Folder_retrieval(directory,fileExtension,SD_Image_Name,100);
  if(Image_CNT) {
    for (int i = 0; i < Image_CNT; i++) {
      strcpy(File_Image_Name[i], SD_Image_Name[i]);
      remove_file_extension(File_Image_Name[i]);
    }
  }
}
void Show_Image(const char * filePath)
{
  // printf("Currently display picture %s\r\n",filePath);
  int16_t ret = png.open(filePath, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
  if (ret == PNG_SUCCESS) {
    // printf("image specs: (%d x %d), %d bpp, pixel type: %d\r\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());

    uint32_t dt = millis();
    if (png.getWidth() > MAX_IMAGE_WIDTH) {
      printf("Image too wide for allocated line buffer size!\r\n");
    }
    else {
      ret = png.decode(NULL, 0);
      png.close();
    }
    // printf("%d ms\r\n",millis()-dt);
  }
}

void Display_Image(const char* directory, const char* fileExtension, uint16_t ID)
{
  Search_Image(directory,fileExtension);
  if(Image_CNT) {
    String FilePath;
    if (String(directory) == "/") {                               // Handle the case when the directory is the root
      FilePath = String(directory) + SD_Image_Name[ID];
    } else {
      FilePath = String(directory) + "/" + SD_Image_Name[ID];
    }
    const char* filePathCStr = FilePath.c_str();          // Convert String to c_str() for Show_Image function
    printf("Show  : %s \r\n", filePathCStr);              // Print file path for debugging
    Show_Image(filePathCStr);                             // Show the image using the file path
  }
  else
    printf("No files with extension '%s' found in directory: %s\r\n", fileExtension, directory);

}
uint16_t Now_Image = 0;
void Image_Next(const char* directory, const char* fileExtension)
{
  if(!digitalRead(BOOT_KEY_PIN)){
    while(!digitalRead(BOOT_KEY_PIN));
    Now_Image ++;
    if(Now_Image == Image_CNT)
      Now_Image = 0;
    Display_Image(directory,fileExtension,Now_Image);
  }
}

void Image_Next_Loop(const char* directory, const char* fileExtension, uint32_t NextTime, uint8_t ledPin)
{
  static uint32_t NextTime_Now = 0;

  NextTime_Now++;

  if (NextTime_Now >= NextTime) {
    NextTime_Now = 0;

    randomSeed(millis()); // oder ein anderer Seed
    Now_Image = random(0, Image_CNT);

    printf("Now Image: %d\r\n", Now_Image);

    rgbLedWrite(ledPin, 255, 255, 255);
    Display_Image(directory, fileExtension, Now_Image);
  }
}
