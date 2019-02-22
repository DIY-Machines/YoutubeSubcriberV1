// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

const char* ssid     = "Your-SSID-Here";      // SSID of local network
const char* password = "Your-Wifi-Password-Here";    // Password on network
const char* YTchannel = "Your-Youtube-User-ID-Here";   // YT user id

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


/*
  The code for this project is based on code originally written by Pawel A. Hernik
  https://youtu.be/mn9L85bhyjI

  Revised by Lewis for DIY Machines
  https://youtu.be/QWaVYCVoqbc

  VCC - 3.3v
  GND - G
  D8 - DataIn
  D7 - LOAD/CS
  D6 - CLK
*/

#include "Arduino.h"
#include <ESP8266WiFi.h>

WiFiClient client;

#define NUM_MAX 4
#define ROTATE 270


// for NodeMCU 1.0
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 12  // D6

#include "max7219.h"
#include "fonts.h"


void setup() 
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN,1);
  sendCmdAll(CMD_INTENSITY,0);
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift(" WiFi ...",15);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
}
// =======================================================================

void loop()
{
  Serial.println("Getting data ...");
  printStringWithShift("  ... YT ...",15);
  int subs, views, cnt = 0;
  String yt1,yt2;
  while(1) {
    if(!cnt--) {
      cnt = 20;  // data is refreshed every 20 loops
      if(getYTSubs(YTchannel,&subs,&views)==0) {
        yt1 = "     SUBSCRIBERS:      "+String(subs)+" ";
        yt2 = "      VIEWS:   "+String(views);
      } else {
        yt1 = "   YouTube";
        yt2 = "   Error!";
      }
    }
    printStringWithShift(yt1.c_str(),50);
    delay(5000);
    printStringWithShift(yt2.c_str(),50);
    delay(5000);
  }
  Serial.println(yt1);
  Serial.println(yt2);
}
// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX*8 + i] = 0;
  return w;
}

// =======================================================================
int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if(c==196 || c==197 || c==195) {
    dualChar = c;
    return 0;
  }
  if(dualChar) {
    switch(_c) {
      case 133: c = 1+'~'; break; // 'ą'
      case 135: c = 2+'~'; break; // 'ć'
      case 153: c = 3+'~'; break; // 'ę'
      case 130: c = 4+'~'; break; // 'ł'
      case 132: c = dualChar==197 ? 5+'~' : 10+'~'; break; // 'ń' and 'Ą'
      case 179: c = 6+'~'; break; // 'ó'
      case 155: c = 7+'~'; break; // 'ś'
      case 186: c = 8+'~'; break; // 'ź'
      case 188: c = 9+'~'; break; // 'ż'
      //case 132: c = 10+'~'; break; // 'Ą'
      case 134: c = 11+'~'; break; // 'Ć'
      case 152: c = 12+'~'; break; // 'Ę'
      case 129: c = 13+'~'; break; // 'Ł'
      case 131: c = 14+'~'; break; // 'Ń'
      case 147: c = 15+'~'; break; // 'Ó'
      case 154: c = 16+'~'; break; // 'Ś'
      case 185: c = 17+'~'; break; // 'Ź'
      case 187: c = 18+'~'; break; // 'Ż'
      default:  break;
    }
    dualChar = 0;
    return c;
  }    
  switch(_c) {
    case 185: c = 1+'~'; break;
    case 230: c = 2+'~'; break;
    case 234: c = 3+'~'; break;
    case 179: c = 4+'~'; break;
    case 241: c = 5+'~'; break;
    case 243: c = 6+'~'; break;
    case 156: c = 7+'~'; break;
    case 159: c = 8+'~'; break;
    case 191: c = 9+'~'; break;
    case 165: c = 10+'~'; break;
    case 198: c = 11+'~'; break;
    case 202: c = 12+'~'; break;
    case 163: c = 13+'~'; break;
    case 209: c = 14+'~'; break;
    case 211: c = 15+'~'; break;
    case 140: c = 16+'~'; break;
    case 143: c = 17+'~'; break;
    case 175: c = 18+'~'; break;
    default:  break;
  }
  return c;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {
  c = convertPolish(c);
  if (c < ' ' || c > MAX_CHAR) return;
  c -= 32;
  int w = showChar(c, font);
  for (int i=0; i<w+1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char* s, int shiftDelay){
  while (*s) {
    printCharWithShift(*s++, shiftDelay);
  }
}

// =======================================================================
unsigned int convToInt(const char *txt)
{
  unsigned int val = 0;
  for(int i=0; i<strlen(txt); i++)
    if(isdigit(txt[i])) val=val*10+(txt[i]&0xf);
  return val;
}
// =======================================================================

const char* ytHost = "www.youtube.com";
int getYTSubs(const char *channelId, int *pSubs, int *pViews)
{
  if(!pSubs || !pViews) return -2;
  WiFiClientSecure client;
  Serial.print("connecting to "); Serial.println(ytHost);
  if (!client.connect(ytHost, 443)) {
    Serial.println("connection failed");
    return -1;
  }
  client.print(String("GET /channel/") + String(channelId) +"/about HTTP/1.1\r\n" + "Host: " + ytHost + "\r\nConnection: close\r\n\r\n");
  int repeatCounter = 10;
  while (!client.available() && repeatCounter--) {
    Serial.println("y."); delay(500);
  }
  int idxS, idxE, statsFound = 0;
  *pSubs = *pViews = 0;
  while (client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    if(statsFound == 0) {
      statsFound = (line.indexOf("about-stats")>0);
    } else {
      idxS = line.indexOf("<b>");
      idxE = line.indexOf("</b>");
      String val = line.substring(idxS + 3, idxE);
      if(!*pSubs)
        *pSubs = convToInt(val.c_str());
      else {
        *pViews = convToInt(val.c_str());
        break;
      }
    }
  }
  client.stop();
  return 0;
}
