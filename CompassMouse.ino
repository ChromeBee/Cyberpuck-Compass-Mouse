// Compass Mouse
// A modern version of the Forte Cyberpuck from the 1990s
// The Cyberpuck was an input device for the Forte VFX1 Headgear VR headset.
// Both the headset and the cyberpuck used the earths magnetic field to determine movement.
// in the headsets case it was used to determine where the user was looking and in the Cyberpucks case, how they moved their hand.
// It was a 2 DOF device using pitch and roll to map to Y and X.
// As well as using it with the VFX1 headset, it could be used alone as a mouse that didn't need desk space.
// this is my modern interpretation of the Cyberpuck device.
// i'm using aSeeed Studio XIAO ESP32C3 as the microcontroller and GY-271 magnetometer sensor board to detect the earths magnetic field
// it uses the QMC5883 compass sensor.
// 3 6x6x5mm tactile buttons are also used to simulate mouse buttons. All came from Amazon in the UK.
//
// This sketch needs the QMC5883LCompass library and a BleMouse library
// The default BLEMouse library doesn't compile due to change in the arduino string type introduced some time ago. Use the library below instead
//  https://github.com/sirfragles/ESP32-BLE-Mouse/tree/dev

#include <BleMouse.h>
#include <QMC5883LCompass.h>
#include "driver/rtc_io.h"




#define MAX_COMPASS_VALUE 600.0 // assumed maximum from compass read once deadzone value is removed
#define MAX_MOUSE_MOVEMENT 7 // Maximum single step for mouse. This is what it takes when compass output is MAX_COMPASS_VALUE or higher
#define MOVEMENT_BUTTON 0 // button that enables mouse movement
#define LEFT_BUTTON 2 // button that simulates left mouse button pressed
#define RIGHT_BUTTON 1 // button that simulates right mouse button press

#define TIMEOUT 2*60*1000 // 2 minutes of inactivity will cause the ESP32 to enter deep sleep mode.

#define BUTTON_PIN_BITMASK  1ULL<<GPIO_NUM_2 | 1ULL<<GPIO_NUM_3 | 1ULL<<GPIO_NUM_4 // pins used for buttons. need bit mask for wakeup action.
gpio_num_t BTNLIST[3] = { // Button pin list - note that BUTTON_PIN_MASK will need changed if this list changes
  GPIO_NUM_2,
  GPIO_NUM_3,
  GPIO_NUM_4
};

static bool reverseX = true; // reverse mouse X direction if true
static bool reverseY = true; // reverse mouse Y direction if true

#define BLUETOOTHTIMEOUT 5*1000 // time connection is allowed to drop before trying to pair with another device
QMC5883LCompass compass;
static std::string deviceName = "Cyberpuck mouse";
BleMouse bleMouse;
int map_array[MAX_MOUSE_MOVEMENT*(MAX_MOUSE_MOVEMENT+1)/2]; // array that is the size all the interger values from MAX_MOUSE_MOVEMENT added together down to zero. i.e. if this were 3 it would equal 3+2+1+0

//-------------------------------------
// convert from compass value to mouse movement value
// the map_array is filled in the setup subroutine to favour small movements
int mapToMovement(float input, float max_output,float max_input) {
  int output;
  int max_index = (int)(max_output*(max_output+1)/2.0); // Size of the map array
  
  output = input*max_index/max_input; // find the array index to use (this may be negative indicating the looked up value must be the same)
  int sgn=output/abs(output); // get the sign of the value (this gives either 1 or -1)
  //Serial.print("array index ");Serial.println(output);
  output = (abs(output)>=max_index) ? sgn*map_array[max_index-1] : sgn*map_array[abs(output)]; // check that the result is not too large then look up the step size to take
  
  return output; // return mouse step size
}

//-----------------------------------
// read buttons with debounce code included
#define DEBOUNCE_TIME 50 // number of milliseconds allowed to reach steady state
unsigned long timeSinceChange[3] = {0,0,0}; // milliseconds since switch state last changed
uint8_t lastKnownState[3] = {0,0,0}; // remember old switch state.
bool stateChanged[3] = {false,false,false};

void readAllFromButtons(uint8_t *buttonValues){
  for(int i=0; i<3; i++){ // read button values
    buttonValues[i] = !gpio_get_level(BTNLIST[i]);
    stateChanged[i] = false;
    if(buttonValues[i] != lastKnownState[i]) {
      if(millis() - timeSinceChange[i] > DEBOUNCE_TIME) {
        lastKnownState[i] = buttonValues[i]; // sufficient time has passed to accept this a the new switch state
        timeSinceChange[i] = millis();
        stateChanged[i] = true;
        Serial.print("button ");Serial.print(i);Serial.println(" pressed");
      } else {
        buttonValues[i] = lastKnownState[i]; // keep the old state as this could be a bounce
      }    
    }
  }
}

//---------------------------------------
void setup() {
  Serial.begin(115200);
  //delay(3000);
  int k = 0;

  // fill map array with steps to be taken. Small movements occupy more of the array than big movements to give more control
  // if MAX_MOUSE_MOVEMENT were to be set to 3, then the array would be filled with 1,1,1,2,2,3
  // The map to movement function uses this to determine how big a mouse movement to make for a particular angle.
  for (int i = 0; i<MAX_MOUSE_MOVEMENT; i++) {
    for (int j = 0; j<MAX_MOUSE_MOVEMENT-i; j++) {
      map_array[k++] = i+1;
      Serial.print(i+1);Serial.print(",");
    }
  }
  Serial.print(" Array Size = ");Serial.println(k);

  // enable the buttons with pull ups (using rtc functions)
  for( int i = 0; i < 3; i++) {
    gpio_set_direction(BTNLIST[i],GPIO_MODE_INPUT);
    gpio_pulldown_dis(BTNLIST[i]);
    gpio_pullup_en(BTNLIST[i]);
  }

  // enable deepsleep to save battery
  esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK,ESP_GPIO_WAKEUP_GPIO_LOW);

  
  Serial.println("Starting BLE work!");
  bleMouse.deviceName = deviceName;
  bleMouse.begin();
  compass.init();
  compass.setCalibration(-1920,1275,-1850,1503,-1288,1912); // maximum and minimum values determined through manual testing - may not be needed for mouse function

}

//----------------------------------------
float cx = 0,cy = 0,cz = 0;
float deadzone = 200;
float mx ,my ,mz;
int sx,sy;
bool bleFirstConnect = true;
unsigned long timeSinceActive=millis(); // mouse inactivity timer
unsigned long bluetoothConnectionTimeout = 0; // bluetooth connection drop time

void loop() {
  float x, y, z;
  double xy,xz,yx,yz,zx,zy;
  uint8_t buttonReads[3];
  float sgn;

  
  if(bleMouse.isConnected()) {
    if (bleFirstConnect) {
      Serial.print(bleFirstConnect);Serial.print("BLE Connected");
      bleFirstConnect = false;
      Serial.println(bleFirstConnect);
    }

  
    // button values true or false
    readAllFromButtons(buttonReads);

    if (buttonReads[MOVEMENT_BUTTON]) {    
      compass.read();

      // Return XYZ readings
      x = float(compass.getX());
      y = float(compass.getY()); // only using x and z compass axis. May use this for getting the max and min values if needed for calibration
      z = float(compass.getZ());
      
      // set up centre coordinates if not set already - these are removed from subsequent readings to get direction of movement
      if ((abs(x)>0) && (cx == 0)) {
        cx = x; cz = z;
      }
      
      if (cx != 0) { // if we have already set a center value for the axis,  the movement value is the actual value minus the center value
        mx = x - cx; mz = z-cz;
      }
      // there is a dead zone in the middle to allow for fluctuations in the readings. If reading is between plus and minus of the deadzone value, ignore it
      // I can rework this section as only the else section is required for movement
      if (abs(mx)<deadzone){
        mx = 0;
        sy = 0;
      } else {
        sgn = mx/abs(mx);
        mx = (reverseX ? -1.0 : 1.0)*sgn*(abs(mx)-deadzone); // the -1 in the front reverses the direction of the movement
        sy = mapToMovement(mx,MAX_MOUSE_MOVEMENT,MAX_COMPASS_VALUE); // map the compass value to a mouse movement value

      }
      if (abs(mz)<deadzone){
        mz = 0;
        sx = 0;
      } else {
        sgn = mz/abs(mz);
        mz = (reverseY ? -1.0 : 1.0)*sgn*(abs(mz)-deadzone);
        sx = mapToMovement(mz,MAX_MOUSE_MOVEMENT,MAX_COMPASS_VALUE);  // map the compass value to a mouse movement value

      }

      // for debug, write out the movement values
      Serial.print("MX = ");
      Serial.print(mx);
      Serial.print(" MZ = ");
      Serial.println(mz);
      
      bleMouse.move(0,sy);
      Serial.print("Scroll vertical "); Serial.print(sy); // for debug

      bleMouse.move(sx,0);
      Serial.print(" Scroll horizontal "); Serial.println(sx); // for debug

      timeSinceActive = millis();  // reset inactivity timer
      } else {
        cx = 0; // reset mid point each time button is released. When  the button is pressed again a new neutral position is calculated
      }

    // check for mouse button presses  
    if ((buttonReads[LEFT_BUTTON]) && (stateChanged[LEFT_BUTTON])) {
      Serial.println("Left button pressed");
      bleMouse.press(MOUSE_LEFT);
      timeSinceActive = millis();  // reset inactivity timer
    } else if (stateChanged[LEFT_BUTTON]) {
      Serial.println("Left button released");
      bleMouse.release(MOUSE_LEFT);
    }
    
    if ((buttonReads[RIGHT_BUTTON]) && (stateChanged[RIGHT_BUTTON])) {
      Serial.println("Right button pressed");
      bleMouse.press(MOUSE_RIGHT);
      timeSinceActive = millis();  // reset inactivity timer
    } else if (stateChanged[RIGHT_BUTTON]) {
      Serial.println("Right button released");
      bleMouse.release(MOUSE_RIGHT);
    }
    bluetoothConnectionTimeout = millis(); // record that we have an active bluetooth connection
  } else { // BLE disconnected
    if ((bluetoothConnectionTimeout != 0) && ((millis() - bluetoothConnectionTimeout) > BLUETOOTHTIMEOUT)) { // If we have had a connection and the last time we had it is greater than the timeout value
      //bleMouse.end(); // close the current connection
      //delay(500);
      //bleMouse.begin(); // try to reconnect - either with the same device if still paired or a new one.
      //bluetoothConnectionTimeout = 0; // record that we don't have an active connection. (stops this section being called again)
      //bleFirstConnect = true; // set this so that a new connection can be printed on the serial monitor
      ESP.restart(); // using this as the bleMouse.end function doesn't release all bluetooth resorces
    }
  }
  // check inactivity timer. Is it beyond the threshold value
  //Serial.print("Current inactivity timer ");Serial.println(millis() - timeSinceActive);
  if (millis() - timeSinceActive > TIMEOUT) {
    Serial.println("Timeout exceeded - time to sleep"); // for debug
    esp_deep_sleep_start();
  }

}
