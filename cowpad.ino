#if 1

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#include <TouchScreen.h>
#include <ArduinoJson.h>
#define MINPRESSURE 200
#define MAXPRESSURE 1000

// ALL Touch panels and wiring is DIFFERENT
// copy-paste results from TouchScreen_Calibr_native.ino
const int XP=8,XM=A2,YP=A3,YM=9; //480x320 ID=0x7796
const int TS_LEFT=56,TS_RT=929,TS_TOP=109,TS_BOT=928;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Adafruit_GFX_Button tab1, tab2, tab3, tab4, tab5, button1, button2, button3, button4, button5, button6;

Adafruit_GFX_Button *tabs[] = {&tab1, &tab2, &tab3, &tab4, &tab5};

Adafruit_GFX_Button *buttons[] = {&button1, &button2, &button3, &button4, &button5, &button6};

String message = "";

// Button commands
String btncmds[] = {"", "", "", "", "", ""};

// Clock vars
int t=2, f, second=0, minutes=0, hours=0;

int pixel_x, pixel_y;     //Touch_getXY() updates global vars
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        pixel_x = map(p.y, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
        pixel_y = map(p.x, TS_TOP, TS_BOT, 0, tft.height());
    }
    return pressed;
}

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// CURRENT SETTINGS
// 5 objects, with 6 members, with 2 members
const size_t bufferSize = JSON_OBJECT_SIZE(6) + 6 * JSON_OBJECT_SIZE(6) + 2 * JSON_OBJECT_SIZE(5) + 6 * JSON_OBJECT_SIZE(6);
StaticJsonDocument<bufferSize> doc;

void setup(void)
{
    Serial.begin(9600);
    uint16_t ID = tft.readID();
    Serial.print("TFT ID = 0x");
    Serial.println(ID, HEX);
    if (ID == 0xD3D3) ID = 0x9486; // write-only shield
    tft.begin(ID);
    tft.setRotation(1);            //LANDSCAPE
    // Background Color
    tft.fillScreen(BLACK);
    
    // bottom text
    String text = "AkiPad v1.0";
    tft.setCursor(0, tft.height()-10);
    tft.print(text);
}

void renderTabButtons(int tab) {
  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();

  // Tab system set up buttons
  int tabWidth = tft.width()/5;
  int tabOffset = (tabWidth/2);
  // init tab system
  for (int i = 0; i < 5; i++) {
    String page = "page";
    String tabNum = page+(i+1);
    char* tab_name = doc[tabNum]["name"] | "-";
    
    if (tab == (i+1)) {
      // set current tab color to white
      tabs[i]->initButton(&tft, (tabWidth*i)+tabOffset, 20, tabWidth, 40, WHITE, WHITE, BLACK, tab_name, 2);
    } else {
      // set other tabs to default color
      tabs[i]->initButton(&tft, (tabWidth*i)+tabOffset, 20, tabWidth, 40, WHITE, BLACK, WHITE, tab_name, 2);
    }
    
    tabs[i]->drawButton(false);
  }

  // set up button names
  String page = "page";
  String tabNum = page+tab;
  char* button_names[] = {
    doc[tabNum]["button1"]["name"] | "-",
    doc[tabNum]["button2"]["name"] | "-",
    doc[tabNum]["button3"]["name"] | "-",
    doc[tabNum]["button4"]["name"] | "-",
    doc[tabNum]["button5"]["name"] | "-",
    doc[tabNum]["button6"]["name"] | "-",
  };

  // map current commands
  for (int i = 0; i < 6; i++) {
    String buttonText = "button";
    String buttonNum = buttonText+(i+1);
    const char* cmdText = doc[tabNum][buttonNum]["macro"] | "";
    String pre = "cow>";
    String fullCmd = pre + cmdText;
    String empty = "";
    btncmds[i] = fullCmd;
  }

  // draw current page's button on the screen
  int buttonWidth = tft.width()/3;
  int offset = (buttonWidth/2);
  for (int i = 0; i < 6; i++) {
    if (i < 3) {
      buttons[i]->initButton(&tft, (buttonWidth*i)+offset, 100, buttonWidth-20, 80, WHITE, CYAN, BLACK, button_names[i], 2);
      buttons[i]->drawButton(false);
    }
    else {
      buttons[i]->initButton(&tft, (buttonWidth*(i-3))+offset, 220, buttonWidth-20, 80, WHITE, CYAN, BLACK, button_names[i], 2);
      buttons[i]->drawButton(false);
    }
  }
}

void processMessage(String message) {
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, message);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Init Buttons Tab-1
  renderTabButtons(1);
  
  Serial.println("<e>");

}

void loop(void)
{
    bool down = Touch_getXY();

    // Listen for serial communication to set up buttons
    if(Serial.available()){
      message = Serial.readString();
      message.trim();
      message.replace("\n", "");
      // reached end of json
      if (message.endsWith("<e>")) {
        // remove the <e> part
        message = message.substring(0, message.indexOf("<e>"));
        processMessage(message);
        // clear message for new input
        message = "";
      }
    }

    // button presses
    for (int i = 0; i < 6; i++) {
      // activate press if press is in button bounds
      buttons[i]->press(down && buttons[i]->contains(pixel_x, pixel_y));
  
      // button release, so draw regular unpressed state 
      if (buttons[i]->justReleased())
          buttons[i]->drawButton();
  
      // button pressed, then do action
      if (buttons[i]->justPressed()) {
          buttons[i]->drawButton(true);
          // send command to pc
          Serial.println(btncmds[i]);
          break;
      }
    }

    // tab system button presses
    for (int i = 0; i < 5; i++) {
      // activate press if press is in button bounds
      tabs[i]->press(down && tabs[i]->contains(pixel_x, pixel_y));
  
      // button release, so draw regular unpressed state 
      if (tabs[i]->justReleased())
          tabs[i]->drawButton();
  
      // button pressed, then do action
      if (tabs[i]->justPressed()) {
          tabs[i]->drawButton();
          // switch current macros
          renderTabButtons(i+1);
          break;
      }
    }
}
#endif
