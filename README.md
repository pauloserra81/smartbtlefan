# smartbtlefan
smart btle hr controlle fan

What you need to configure for your case : 
Look for this defines in the code

// Set number of relays
#define NUM_RELAYS 3

// Heart Rate Zones
#define ZONE_1 117 // minimum to turn on fan
#define ZONE_2 146 // 140 is hard work, go to speed 2
#define ZONE_3 169 // almost dying full wind

//LED control
#define LED_PIN 27
//number of LEDS in the led strip
#define NUM_LEDS 1

//Button setup
//using pins 25 and 21 for the push button, 25 will be used as ground to make connections easier
#define BUTTON_PIN 21
#define BUTTON_GDPIN 25


// Assign each GPIO to a relay
//in my case GPIOS number 19, 23 and 33 are used, check numbers on your board.
uint8_t relayGPIOs[NUM_RELAYS] = {19, 23, 33};
