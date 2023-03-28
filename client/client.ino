/*
  ============================

    SERVO CAM Arduino Client
    v 0.9.2 | 2023.03.22

    (c) 2023, servocam.org
    https://servocam.org
    https://github.com/servo-cam
    info@servocam.org
    
  ============================
*/

#include <Servo.h>

// --------- BEGIN CONFIG -----------

#define DEBUG false // debug mode (sends status to serial after every command)

#define PIN_SERVO_X 10 // servo X (horizontal) PWM pin
#define PIN_SERVO_Y 11 // servo Y (vertical) PWM pin
#define PIN_ACTION_1 2 // action #1 (A1) DIGITAL pin
#define PIN_ACTION_2 4 // action #2 (A2) DIGITAL pin
#define PIN_ACTION_3 7 // action #3 (A3) DIGITAL pin
#define PIN_ACTION_4 8 // action #4 (B4) DIGITAL pin
#define PIN_ACTION_5 12 // action #5 (B5) DIGITAL pin
#define PIN_ACTION_6 13 // action #6 (B6) DIGITAL pin

#define SERVO_X_MIN 0 // servo X min angle
#define SERVO_X_MAX 180 // servo X max angle
#define SERVO_Y_MIN 0 // servo Y min angle
#define SERVO_Y_MAX 180 // servo Y max angle

#define SERVO_X_LIMIT_MIN 0 // servo X min allowed movement angle
#define SERVO_X_LIMIT_MAX 180 // servo X max allowed movement angle
#define SERVO_Y_LIMIT_MIN 0 // servo Y min allowed movement angle
#define SERVO_Y_LIMIT_MAX 180 // servo Y max allowed movement angle

#define SERVO_X_PULSE_MIN 771 // servo X min pulse
#define SERVO_X_PULSE_MAX 2193 // servo X max pulse
#define SERVO_Y_PULSE_MIN 771 // servo Y min pulse
#define SERVO_Y_PULSE_MAX 2193 // servo Y max pulse

#define SERVO_X_INITIAL_ANGLE 90 // servo X start angle
#define SERVO_Y_INITIAL_ANGLE 90 // servo Y start angle

#define SERIAL_TIMEOUT 4 // serial port timeout
#define SERIAL_BAUD_RATE 9600 // serial port baud rate

// --------- END OF CONFIG -----------


bool actions[6] = {}; // action states
int pins[6] = {}; // action pins
int counter = 0; // detected objects counter
int x = 0; // servo X current angle
int y = 0; // servo Y current angle

char *cmd[9]; // extracted commands
char *ptr = NULL; // str cmd pointer

Servo servo_x; // servo X
Servo servo_y; // servo Y
String buffer; // serial data buffer

void setup() 
{ 
  Serial.begin(SERIAL_BAUD_RATE); // begin serial
  Serial.setTimeout(SERIAL_TIMEOUT); // set minimal timeout

  // setup action pins
  pins[0] = PIN_ACTION_1; // action #1 (A1) pin
  pins[1] = PIN_ACTION_2; // action #2 (A2) pin
  pins[2] = PIN_ACTION_3; // action #3 (A3) pin
  pins[3] = PIN_ACTION_4; // action #4 (B4) pin
  pins[4] = PIN_ACTION_5; // action #5 (B5) pin
  pins[5] = PIN_ACTION_6; // action #6 (B6) pin

  x = SERVO_X_INITIAL_ANGLE; // start servo X with default position
  y = SERVO_Y_INITIAL_ANGLE; // start servo Y with default position

  // setup servo
  servo_x.write(x); // set initial servo X position
  servo_y.write(y); // set initial servo Y position
  servo_x.attach(PIN_SERVO_X, SERVO_X_PULSE_MIN, SERVO_X_PULSE_MAX); // attach servo X
  servo_y.attach(PIN_SERVO_Y, SERVO_Y_PULSE_MIN, SERVO_Y_PULSE_MAX); // attach servo Y

  // initialize action pins with LOW state
  for (int i = 0; i < 6; i++) {
    pinMode(pins[i], OUTPUT); // set pin to OUTPUT mode
    actions[i] = false; // store disabled state
    digitalWrite(pins[i], LOW); // set state = LOW   
  }
} 

// define status message here, must returns String
String get_status() {
  return "RECV: " + buffer;
}
 
void loop() 
{  
  buffer = ""; // clear

  // read serial buffer
  if (Serial.available() > 0) {
    buffer = Serial.readStringUntil('\n'); // \n must be the command terminator
    if (buffer == "\n" || buffer == "") {
      return; // abort if empty command
    }
    // send parsed status message if command == 0 received
    if (buffer == "0") {
      Serial.println(get_status() + ""); // <---------- STATUS SEND
      return;
    }
  } else {
    return; // abort if no command
  }

  // parse buffer
  if (buffer.length() > 0) {
    byte index = 0;
    byte ary[buffer.length() + 1];
    buffer.toCharArray(ary, buffer.length() + 1);
    ptr = strtok(ary, ",");  // command delimiter    
    while (ptr != NULL) {
      cmd[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");
    }

    String tmp_cmd; // tmp string
    
    // get servo positions
    if (index > 0) {
      tmp_cmd = String(cmd[0]); // servo X angle
      x = tmp_cmd.toInt(); // to integer
    }
    if (index > 1) {
      tmp_cmd = String(cmd[1]); // servo Y angle
      y = tmp_cmd.toInt(); // to integer
    }

    // fix min/max angle
    if (x < SERVO_X_MIN) {
      x = SERVO_X_MIN;
    } else if (x > SERVO_X_MAX) {
      x = SERVO_X_MAX;
    }
    if (y < SERVO_Y_MIN) {
      y = SERVO_Y_MIN;
    } else if (y > SERVO_Y_MAX) {
      y = SERVO_Y_MAX;
    }

    // fix min/max allowed movement angle
    if (x < SERVO_X_LIMIT_MIN) {
      x = SERVO_X_LIMIT_MIN;
    } else if (x > SERVO_X_LIMIT_MAX) {
      x = SERVO_X_LIMIT_MAX;
    }
    if (y < SERVO_Y_LIMIT_MIN) {
      y = SERVO_Y_LIMIT_MIN;
    } else if (y > SERVO_Y_LIMIT_MAX) {
      y = SERVO_Y_LIMIT_MAX;
    }

    // move servo position
    servo_x.write(x); // move servo X
    servo_y.write(y); // move servo Y

    // prepare rest of commands
    if (index > 2) {
      tmp_cmd = String(cmd[2]); // detected objects count
      if (tmp_cmd != "") {
        counter = tmp_cmd.toInt(); // to integer
      }      
    }

    // get action states
    for (int i = 0; i < 6; i++) {
      int j = i + 3;
      if (index > j) {
        tmp_cmd = String(cmd[j]);
        if (tmp_cmd != "") {
          actions[i] = (tmp_cmd.toInt() != 0); // to bool
        }
      }
    }

    // apply action commands to pins
    for (int i = 0; i < 6; i++) {
      if (actions[i] == true) {
        digitalWrite(pins[i], HIGH); // enable action
      } else {
        digitalWrite(pins[i], LOW); // disable action
      }
    }
  }

  // debug current position and command
  if (DEBUG == true) {
    Serial.print("CMD: ");
    Serial.println(buffer);  
    Serial.print("POS: ");    
    Serial.println(servo_x.read());
    Serial.println(servo_y.read());
    Serial.println();
  }  

  buffer = ""; // clear serial buffer
}