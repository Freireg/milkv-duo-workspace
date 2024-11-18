#include <string.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "mailbox.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum SYS_CMD_ID {
    CMD_SEND_IP_1 = 0x20,  // New command ID for sending IP
    CMD_SEND_IP_2,         // New command ID for sending IP
    CMD_SEND_IP_3,         // New command ID for sending IP
    CMD_SEND_IP_4,         // New command ID for sending IP
    CMD_SEND_CONNECTED_DEVICES, // New command ID for sending connected device
    CMD_SEND_TEMP,       // New command ID for sending temperature
    SYS_CMD_INFO_LIMIT,
};

struct valid_t {
  uint8_t linux_valid;
  uint8_t rtos_valid;
} __attribute__((packed));

typedef union resv_t {
  struct valid_t valid;
  unsigned short mstime;  // 0 : noblock, -1 : block infinite
} resv_t;

typedef struct cmdqu_t cmdqu_t;
/* cmdqu size should be 8 bytes because of mailbox buffer size */
struct cmdqu_t {
  uint8_t ip_id;
  uint8_t cmd_id : 7;
  uint8_t block : 1;
  union resv_t resv;
  unsigned int param_ptr;
} __attribute__((packed)) __attribute__((aligned(0x8)));

int DUO_IP[4];
int DUO_TEMPERATURE;
int CONNECTED_DEVICES;

void showmsg(MailboxMsg msg) {
  int temp;
  cmdqu_t *cmdq = (cmdqu_t *)msg.data;
  // Check the command ID to differentiate between IP and temperature data
  switch(cmdq->cmd_id) {
  case CMD_SEND_IP_1:
    DUO_IP[0] = (int)cmdq->param_ptr;
  break;
  case CMD_SEND_IP_2:
    DUO_IP[1] = (int)cmdq->param_ptr;
  break;
  case CMD_SEND_IP_3:
    DUO_IP[2] = (int)cmdq->param_ptr;
  break; 
  case CMD_SEND_IP_4:
    DUO_IP[3] = (int)cmdq->param_ptr;
  break;
  case CMD_SEND_CONNECTED_DEVICES:
    CONNECTED_DEVICES = cmdq->param_ptr;
  break;
  case CMD_SEND_TEMP:
    int temperature = (int)cmdq->param_ptr; // Assuming temperature is sent as an integer in °C
    Serial.print("Received CPU Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    DUO_TEMPERATURE = temperature;
  break;
  }
  // Reset message data
  *(msg.data) = 0;
}

void setup() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500); // Pause for 2 seconds
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("Mailbox Start");
  display.display();

  mailbox_init(false);
  mailbox_register(0, showmsg);
  mailbox_enable_receive(0);

  delay(500);
  // pinMode(0, OUTPUT);
}

void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_INVERSE);
    display.display();
    delay(1);
  }
  
  delay(2000);
}

void displayIPAddress(uint32_t counter) {
  const char *IP = "192.168.42.1";
  char temp[50] = {0};
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner

  // sprintf(temp, "Duo's\nIP: %ld.%3d.%3d.%3d\nTemperature: %ldC", DUO_IP[0], DUO_IP[1], DUO_IP[2], DUO_IP[3], DUO_TEMPERATURE);
  // display.println(temp);
  sprintf(temp, "> IP:\n192.168.42.1\n\n> Uptime: %d s", counter);
  display.println(temp);
  display.display();

}

void loop() {
  uint32_t counter = 0;
  char temp[100] = {0};

  for(;;) {
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner

    sprintf(temp, "> IP:\n %d.%d.%d.%d\n> Devices: %d\n> Core temp: %dC", DUO_IP[0], DUO_IP[1], DUO_IP[2], DUO_IP[3], CONNECTED_DEVICES, DUO_TEMPERATURE);

    display.println(temp);
    display.display();

    counter++;
    delay(1000);
  }
}
