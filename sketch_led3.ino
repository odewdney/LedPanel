#include "LedPanel.h"

LedPanel panel;


void setup() {
  // put your setup code here, to run once:

//  Serial.begin(115200);
  Serial.begin(9600);
  Serial.println(F("sw_init"));

  panel.Init();
}



byte convertBrightness(byte percent)
{
  int16_t ret = percent;
  if (percent<50)
  {
    ret = (ret*256 - 3200)/300;
    if ( ret < 0 )
      ret = 0;
    else if ( ret > 0x1f )
      ret = 0x1f;
  }
  else
  {
    ret = (ret*65 - 3300)/300;
    if ( ret < 0 )
      ret = 0;
    else if ( ret > 0x1f )
      ret = 0x1f;
    ret |= 0x20;
  }
  return ret;
}

//static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char cd64[] PROGMEM="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static void decodeblock( unsigned char *in, unsigned char *out )
{   
    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

void readScreen()
{
  char b64[4];
  byte l,r;

    while(1)
    {
      l = 0;
      while(l < 4)
      {
        r = Serial.readBytes(b64 + l, 4-l);
        if ( r==0 ) return;
        while(r>0)
        {
          if (b64[l] >= 43 && b64[l] <= 122)
          {
            b64[l] = pgm_read_byte_near(cd64 + b64[l] - '+'); // 43 
            if ( b64[l] == '$' ) return;
            b64[l] -= '='; // 61
            b64[l]--;
            l++;
          }
          else if (b64[l] == 10 || b64[l] == 13)
          { // eat char
            if ( r > 1 )
            {
              for( byte e = 0; e < r; e++ )
                b64[l+e] = b64[l+e+1];
            }
          }
          else
            return;
          r--;
        }
      }
    }
}



char cmd[40];
byte cmdcnt = 0;

#define MAX_BITMAPS 1
#define MAX_BITMAPHEIGHT 8

struct BITMAPS
{
  byte w,h;
  byte data[MAX_BITMAPHEIGHT];
} bitmaps[MAX_BITMAPS];


byte nextToken(byte pos)
{
  for(; pos < cmdcnt; pos++)
  {
    if ( cmd[pos] == ' ')
    {
      cmd[pos] = 0;
      pos++;
      break;
    }
  }
  return pos;
}

const byte dim_curve[] PROGMEM = {
	0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
	6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
	8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11,
	11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15,
	15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20,
	20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 25, 25, 25, 26, 26,
	27, 27, 28, 28, 29, 29, 30, 30, 31, 32, 32, 33, 33, 34, 35, 35,
	36, 36, 37, 38, 38, 39, 40, 40, 41, 42, 43, 43, 44, 45, 46, 47,
	48, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
	63, 64, 65, 66, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 81, 82,
	83, 85, 86, 88, 90, 91, 93, 94, 96, 98, 99, 101, 103, 105, 107, 109,
	110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
	146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
	193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};


  // hue is a number between 0 and 360
  // saturation is a number between 0 - 255
  // value is a number between 0 - 255
void getRGB(uint16_t hue, byte sat, byte val, byte colors[3]) {
	/* convert hue, saturation and brightness ( HSB/HSV ) to RGB
	The dim_curve is used only on brightness/value and on saturation (inverted).
	This looks the most natural.
	*/

//	sat = 255 - dim_curve[255 - sat];
//	val = pgm_read_byte_near(&dim_curve[val]);
//	sat = 255 - pgm_read_byte_near(&dim_curve[255 - sat]);

	byte r;
	byte g;
	byte b;
	uint16_t base;

	if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
		colors[0] = val;
		colors[1] = val;
		colors[2] = val;
	}
	else  {

		base = ((255 - sat) * val) >> 8;

		switch (hue / 60) {
		case 0:
			r = val;
			g = (((val - base)*hue) / 60) + base;
			b = base;
			break;

		case 1:
			r = (((val - base)*(60 - (hue % 60))) / 60) + base;
			g = val;
			b = base;
			break;

		case 2:
			r = base;
			g = val;
			b = (((val - base)*(hue % 60)) / 60) + base;
			break;

		case 3:
			r = base;
			g = (((val - base)*(60 - (hue % 60))) / 60) + base;
			b = val;
			break;

		case 4:
			r = (((val - base)*(hue % 60)) / 60) + base;
			g = base;
			b = val;
			break;

		case 5:
			r = val;
			g = base;
			b = (((val - base)*(60 - (hue % 60))) / 60) + base;
			break;
		}

		colors[0] = r;
		colors[1] = g;
		colors[2] = b;
	}
}

void demo(byte n)
{
	for (int h = 0; h < 64; h++)
	{
//		for (int s = 0; s < 8; s++)
		byte s = 255;
		{
			for (byte v = 0; v < 8; v++)
			{
				byte col[3];
				getRGB((360*h)/64, s << 5, (255*(v+2))/10, col);
				uint16_t c = ((col[0] >> 5) << 8) | ((col[1] >> 5) << 4) | ((col[2] >> 5));
//				c = ((c >> 1) & 0x333) | ((c & 0x111) << 2);
/*				if (v == 1)
				{
					char bbb[40];
					sprintf(bbb, "%d %d %d = %d %d %d %03x", h, s, v, col[0],col[1],col[2],c);
					Serial.println(bbb);
				}
*/
				panel.drawPixel((n+h) & 63, v, c);
			}
		}
	}
}

void processCommand()
{
  byte arg,nextarg;

  arg = nextToken(0);

  if (cmd[0]=='d')
  {
	  if (cmd[1] == 'd')
	  {
		  uint16_t c;
		  c = atoi(cmd + arg);
		  demo(c);
	  }
	  if (cmd[1] == 'x')
	  {

		  uint16_t c;
		  for (c = 0; c < 63; c++)
		  {
			  delay(100);
			  demo(c);
		  }
	  }
	  else if (cmd[1] == 'c')
    {
      uint16_t x,y,r;
      nextarg = nextToken(arg);
      x = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      r = atoi(cmd+arg);
      arg = nextarg;
      
      panel.circle(x,y,r);
    }
    else if(cmd[1]=='t')
    {
      uint16_t x,y;
      nextarg = nextToken(arg);
      x = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y = atoi(cmd+arg);
      arg = nextarg;
      
      panel.text(cmd+arg,x,y);
    }
    else if (cmd[1]=='r')
    {
      uint16_t x,y,w,h;
      nextarg = nextToken(arg);
      x = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      w = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      h = atoi(cmd+arg);
      arg = nextarg;
      
      panel.rect(x,y,w,h);
    }
    else if (cmd[1]=='l')
    {
      uint16_t x,y,x2,y2;
      nextarg = nextToken(arg);
      x = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      x2 = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y2 = atoi(cmd+arg);
      arg = nextarg;
      
      panel.line(x,y,x2,y2);
    }
    else if (cmd[1]=='p')
    {
      uint16_t x,y;
      nextarg = nextToken(arg);
      x = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y = atoi(cmd+arg);
      arg = nextarg;
      
      panel.point(x,y);
    }
    else if (cmd[1]=='b')
    {
      uint16_t b,x,y;
      nextarg = nextToken(arg);
      b = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      x = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      y = atoi(cmd+arg);
      arg = nextarg;
      
      panel.drawBitmapMem(x,y,bitmaps[b].data,bitmaps[b].w,bitmaps[b].h,panel.getColor());
    }
  }
  else if(cmd[0]=='c')
  {
    if(cmd[1]=='s')
    {
      uint16_t c;
      c = atoi(cmd+arg);
      panel.stroke(c);
    }
    else if(cmd[1]=='b')
    {
      uint16_t c;
      c = atoi(cmd+arg);
      panel.background(c);
    }
    else if(cmd[1]=='t')
    {
      uint16_t c;
      c = atoi(cmd+arg);
      panel.background(c);
    }
    else if(cmd[1]=='f')
    {
      uint16_t c;
      c = atoi(cmd+arg);
      if (c & 0x8000)
        panel.noFill();
      else
        panel.fill(c);
    }
    else if(cmd[1]=='g')
    {
      byte r,g,b;
      nextarg = nextToken(arg);
      r = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      g = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      b = atoi(cmd+arg);
      arg = nextarg;
      r = convertBrightness(r);
      g = convertBrightness(g);
      b = convertBrightness(b);
//      sprintf(cmd,"b: %02x %02x %02x",(int)r,(int)g,(int)b);
//      Serial.println(cmd);
      panel.setBrightness(r,g,b);
    }
  }
  else if (cmd[0] == 't')
  {
    if(strcmp(cmd,"tt")==0)
    {
      Serial.print("DrawTime:");
      Serial.println(panel.updatetime);
    }
    else if(strcmp(cmd,"ts")==0)
    {
      panel.dump();
    }
    else if (cmd[1] == 'c')
    {
      panel.clear();
      Serial.println("Cleared");
    }
  }
  else if (cmd[0] == 's')
  {
    if (cmd[1] == 'l')
    {
      uint16_t t = atoi(cmd+arg);
      delay(t);
    }
  }
  else if (cmd[0] == 'l')
  {
    if (cmd[1] == 's')
    {
      readScreen();
    }
    else if (cmd[1] == 'b')
    {
      uint16_t b,w,h;
      nextarg = nextToken(arg);
      b = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      w = atoi(cmd+arg);
      arg = nextarg;
      nextarg = nextToken(arg);
      h = atoi(cmd+arg);
      arg = nextarg;
      if ( b < MAX_BITMAPS)
      {
        byte byte_width =  (w + 7) / 8;
        if (h*byte_width <= MAX_BITMAPHEIGHT)
        {
          bitmaps[b].w = w;
          bitmaps[b].h = h;
          for(byte n = 0; n < h; n++)
          {
            for(byte m = 0; m < byte_width;m++)
            {
              nextarg = nextToken(arg);
              bitmaps[b].data[n * byte_width + m] = atoi(cmd+arg);
              arg = nextarg;
            }
          }
        //  sprintf(cmd,"LB %d %d %d", b,w,h);
        //  Serial.println(cmd);
        }
      }
    }
  }
  else
  {
    Serial.println(F("???"));
  }
}

void loop() {
  // put your main code here, to run repeatedly:


  delay(2);
  
  while(Serial.available())
  {
    int c = Serial.read();

    if (c == 13 || c == 10)
    {
      if ( cmdcnt > 0 )
      {
        Serial.println();
        cmd[cmdcnt] = 0;
        processCommand();
        cmdcnt = 0;
      }
    }
    else if(c==127 || c == 8)
    {
      if ( cmdcnt > 0)
      {
        Serial.print(F("\b \b"));
        cmdcnt--; 
      }
    }
    else
    {
      if(c>=32 && c<127)
      {
        if ( cmdcnt < sizeof(cmd)-1)
        {
          cmd[cmdcnt++] = c;
          Serial.print((char)c);
        }
      }
    }
  }
}

