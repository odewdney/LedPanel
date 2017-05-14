#include <TFT.h>
#include <avr/pgmspace.h> 

extern const unsigned char  font[] PROGMEM;

class LedPanel : public Adafruit_GFX
{
  public:
    LedPanel();

  void Init();
  void clear();
  void dump();
  void setBrightness(byte red, byte green, byte blue);

    
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    uint16_t newColor(uint8_t red, uint8_t green, uint8_t blue);

    uint16_t getColor() { return textcolor; }
    void drawBitmapMem(int16_t x, int16_t y, 
            const uint8_t *bitmap, int16_t w, int16_t h,
            uint16_t color);

static uint16_t updatetime;
// should be friend
  void update(byte b, byte y);
protected:
#define BUF_PAGES 8
#define BUF_WIDTH 144
	byte buf[BUF_PAGES][BUF_WIDTH];

};

