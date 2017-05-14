#include "LedPanel.h"
#include "fastDigitalWrite.h"

#define CLK 4
#define ADDR1 5
#define ADDR0 6
#define LATCH 7
#define OUTPUTENABLE 8
#define DATA2 9
#define DATA1 10

LedPanel *currentPanel = NULL;

uint16_t LedPanel::updatetime = 0;

/*
void drawText(byte x, byte y, byte col, const char*msg)
{
  while(*msg)
  {
    const char c = *msg;
    if (c==32) col++;
    const unsigned char * f  = &font[((uint8_t)c)*5];
    for( byte fx = 0; fx < 5; fx++)
    {
      unsigned char fc = pgm_read_byte_near(f++);
      for(byte fy = 0; fy < 7; fy++)
      {
        if (fc & (1<<fy))
          setpixel(x+fx, y+fy,col);
      }
    }
    x += 6;
    if ( x >= (63-5) )
    {
      x = 0;
      y += 8;
    }
    
    msg++;
  }
}
*/

LedPanel::LedPanel() : Adafruit_GFX(64,8)
{
}

void LedPanel::Init()
{
  uint8_t x;
  uint16_t y;
  
  for(x=0;x<BUF_PAGES;x++)
   for(y=0;y<BUF_WIDTH;y++)
    buf[x][y] = 0;

  pinMode(CLK,OUTPUT);
  pinMode(ADDR1,OUTPUT);
  pinMode(ADDR0,OUTPUT);
  pinMode(LATCH,OUTPUT);
  pinMode(OUTPUTENABLE,OUTPUT);
  pinMode(DATA2,OUTPUT);
  pinMode(DATA1,OUTPUT);

  digitalWrite(CLK,LOW);
  digitalWrite(LATCH,LOW);
  digitalWrite(OUTPUTENABLE,HIGH);

  currentPanel = this;

  /*
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
  */

  // initialize Timer1 ~400Hz
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 32;              // compare match register 16MHz/256/390Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts

};

void UpdateFrame()
{
	static byte b = 0;
	static byte c = 0;
	static byte y = 0;
	if (c == 0)
	{
		unsigned long start = micros();
		if (currentPanel)
			currentPanel->update(b, y);
		LedPanel::updatetime = micros() - start;
	}
	if ((++c) >= (1<<b))
	{
		c = 0;
		if (++b > 2)
		{
			b = 0;
			y = (y+1) & 3;
		}
	}
}


ISR(TIMER1_COMPA_vect){     // timer compare interrupt service routine
	UpdateFrame();
}

void LedPanel::clear()
{
      memset(buf,0,BUF_PAGES*BUF_WIDTH);

}

void LedPanel::dump()
{
  for(byte b = 0; b < BUF_PAGES;b++)
  {
    for(uint16_t x = 0; x < BUF_WIDTH;x++)
    {
      if ( buf[b][x] != 0 )
      {
        char cmd[30];
        sprintf(cmd, "a:%04d b:%d x:%d = %2x",(int)&buf[b][x],b,x,buf[b][x]);
        Serial.println(cmd);
      }
    }
  }
}

#define digitalWriteFastV(P,V) if(V) {digitalWriteFast(P,HIGH);} else {digitalWriteFast(P,LOW);}

void LedPanel::update(byte b, byte y)
{
    byte *p;
  p = buf[y&3];
  p += b * 48;

  byte *pp;
  pp = buf[(y & 3)+4];
  pp += b * 48;

  for (byte x = 48; x > 0; --x)
  {
	  byte c,cc;
	  c = *p++;
	  cc = *pp++;

	  digitalWriteFastV(DATA1, c & 1);
	  digitalWriteFastV(DATA2, cc & 1);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);
		
	  digitalWriteFastV(DATA1, c & 2);
	  digitalWriteFastV(DATA2, cc & 2);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);

	  digitalWriteFastV(DATA1, c & 4);
	  digitalWriteFastV(DATA2, cc & 4);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);

	  digitalWriteFastV(DATA1, c & 8);
	  digitalWriteFastV(DATA2, cc & 8);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);

	  digitalWriteFastV(DATA1, c & 16);
	  digitalWriteFastV(DATA2, cc & 16);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);

	  digitalWriteFastV(DATA1, c & 32);
	  digitalWriteFastV(DATA2, cc & 32);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);

	  digitalWriteFastV(DATA1, c & 64);
	  digitalWriteFastV(DATA2, cc & 64);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);

	  digitalWriteFastV(DATA1, c & 128);
	  digitalWriteFastV(DATA2, cc & 128);
	  digitalWriteFast(CLK, HIGH);
	  digitalWriteFast(CLK, LOW);
  }

  digitalWriteFast(OUTPUTENABLE,HIGH); // disable output
  digitalWriteFastV(ADDR0, y & 1); // setup address
  digitalWriteFastV(ADDR1, y & 2);
  digitalWriteFast(LATCH,HIGH); // pulse latch
  digitalWriteFast(LATCH,LOW); // pulse latch
  digitalWriteFast(OUTPUTENABLE,LOW); // ensable output
}


/*
SIGNAL(TIMER0_COMPA_vect) 
{
	UpdateFrame();
}
*/


void LedPanel::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  int16_t off = (x>>3)*6 + ((y & 4)>>2);
	byte *p = &buf[(y & 3) + ((y&8)>>1)][off];

  byte maskbit = 1<< (x & 7);
  byte mask = ~maskbit;

  for(byte c = 0; c < 3;c++)
  {
    *p = (*p & mask) | ((color & 0x1)?maskbit:0);
	*(p+2) = (*(p+2) & mask) | ((color & 0x10) ? maskbit : 0);
	*(p+4) = (*(p+4) & mask) | ((color & 0x100) ? maskbit : 0);
	color >>= 1;
    p += 48;
  }
}


uint16_t LedPanel::newColor(uint8_t red, uint8_t green, uint8_t blue)
{
    return ((blue&0xf0)<<8) | (green&0xf0) | ((blue&0xf0)>>4);
//  return (blue>>7) | ((green&0x80)>>6) | ((red&0x80)>>5);
}


void LedPanel::drawBitmapMem(int16_t x, int16_t y, 
            const uint8_t *bitmap, int16_t w, int16_t h,
            uint16_t color) 
{
  int16_t i, j, byteWidth = (w + 7) / 8;

  if (useFill)
  {
    for(j=0; j<h; j++) {
      for(i=0; i<w; i++ ) {
        if(bitmap[j * byteWidth + i / 8] & (128 >> (i & 7))) {
          drawPixel(x+i, y+j, color);
        }
        else
          drawPixel(x+i,y+j,fillColor);
      }
    }
  }
  else
  {
    for(j=0; j<h; j++) {
      for(i=0; i<w; i++ ) {
        if(bitmap[j * byteWidth + i / 8] & (128 >> (i & 7))) {
          drawPixel(x+i, y+j, color);
        }
      }
    }
  }
}

void LedPanel::setBrightness(byte red, byte green, byte blue)
{

  uint16_t config_reg = 0x7140;

  noInterrupts();

  for(byte y = 0; y < 8;y++)
  {
    for(byte c =0 ; c < 3;c++)
    {
      for(byte x = 15;x>=6;x--)
      {
        if (config_reg & (1<<x))
        {
digitalWriteFast(DATA2,HIGH);
digitalWriteFast(DATA1,HIGH);
        }
        else
        {
digitalWriteFast(DATA2,LOW);
digitalWriteFast(DATA1,LOW);
        }

digitalWriteFast(CLK,HIGH);
digitalWriteFast(CLK,LOW);
      }
      for(byte x = 0;x < 6 ;x++)
      {
        if ((c==0?blue:(c==1?green:red)) & (0x20>>x))
        {
digitalWriteFast(DATA2,HIGH);
digitalWriteFast(DATA1,HIGH);
        }
        else
        {
digitalWriteFast(DATA2,LOW);
digitalWriteFast(DATA1,LOW);
        }
        if (y == 7 && c==2 && x==2) // last 4 clocks
        {
          digitalWrite(LATCH,HIGH); // pulse latch
        }
          
digitalWriteFast(CLK,HIGH);
digitalWriteFast(CLK,LOW);
      }
    }
  }
  digitalWrite(LATCH,LOW); // pulse latch

  interrupts();
}


