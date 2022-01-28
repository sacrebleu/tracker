// comment out this line to stop debugs
#define DEBUG

// state machine variables
#define STATE_OFF 0
#define STATE_TRACKING 3
#define STATE_REV 2
#define STATE_FWD 1

// LED output pins begin
#define LED_REVERSE 6  // LED that lights when the drive is reversing
#define LED_TRACK 7    // LED that lights when the drive is tracking
#define LED_FORWARD 8  // LED that lights when the drive is fast-forwarding
// LED output pins end

// button input pins
#define BTN_REVERSE 2
#define BTN_FORWARD 3
#define BTN_TRACKING 9
// button input pins end

// MOTOR CONFIG
#define EARTH_ROTATION 0.0041667 // approximation of 1/240
#define THRESHOLD 3000 // ms window length

// motor output pins

// motor output pins end

/* class model for an LED - encodes the pin and provides utility methods to turn the LED on and off */
class Led {
  private:
    byte pin;
    
  public:
    Led(byte pin) {
      this->pin = pin;
      pinMode(pin, OUTPUT);
      off();
    }
     
    // set the mapped pin HIGH - turning on any correctly-connected component, in this case hopefully an LED
    void on() {
      digitalWrite(pin, HIGH);
    }

    // set the pin low
    void off() {
      digitalWrite(pin, LOW);
    }

    void set(bool v) {
      if(v) {
        on();
      } else {
        off();
      }
    }
};

// Abstract superclass to resolve circular dependency that otherwise is created
class AbstractMotor {
    
  public:
    AbstractMotor() { } 

    virtual void drive_motor(int steps) { }
};

/* model the system state in a single class and provides utility methods for changing the state */
class ControlSystem {
  private:
    Led reverse = Led(LED_REVERSE);
    Led forward = Led(LED_FORWARD);
    Led track   = Led(LED_TRACK);
    AbstractMotor *motor;

    int state;
    unsigned long start_time;
    unsigned long end_time;
    unsigned long window = 0;
    volatile bool interrupted = false;
    
  public:
    ControlSystem() {
      init();
    }

    void init() {
      set_off();
      start_time = millis(); // read clock start time
      end_time   = millis(); // read clock end time
    }

    void set_motor(AbstractMotor *motor) {
      this->motor = motor; 
    }

    void set_off() {
      state = STATE_OFF;
    }

    bool is_off() {
      return STATE_OFF == state;
    }

    void set_tracking() {
      state = STATE_TRACKING;
    }

    bool is_tracking() {
      return STATE_TRACKING == state;
    }

    void set_reverse() {
      state = STATE_REV; 
    }

    bool is_reverse() {
      return STATE_REV == state;
    }

    void set_forward() {
      state = STATE_FWD;
    }

    bool is_forward() {
      return STATE_FWD == state;
    }

    // toggles between TRACKING and OFF
    void toggle_tracking(void) {
      if(digitalRead(BTN_TRACKING) == 0) {
        if(!this->is_interrupted()) {
          if(this->is_tracking()) {
            while(digitalRead(BTN_TRACKING) == 0) { // debounce toggle
              this->set_off();
              delay(50);
            }
          } else {
             while(digitalRead(BTN_TRACKING) == 0) { // debounce toggle
               this->set_tracking();
               delay(50);
             }
          }
        }
        this->interrupt();
      }
    }    

    // display the current state of the system
    void render() {
      reverse.set(is_reverse());
      forward.set(is_forward());
      track.set(is_tracking());
    }

    // action tick
    void tick() {
      start_time = end_time;
      end_time   = millis();
      window     += (end_time - start_time);
      
      render();
      step_motor();
      
      toggle_tracking();
      clear_interrupt();
    }

    void step_motor() {
     if(window >= THRESHOLD) {
        motor->drive_motor(steps());
        window = 0;
      }
    }

    int steps() {
      return 1;
    }

    // output the state of the system if debugging is turned on
    void debug() {
      Serial.print("ø: ");
      Serial.print(state);
      Serial.print(" | dT : ");
      Serial.print(end_time - start_time);
      Serial.print(" | ∫ : ");
      Serial.println(window);
    }

    bool is_interrupted() {
      return interrupted;
    }

    void interrupt() {
      interrupted = true;
    }

    void clear_interrupt() {
      interrupted = false;
    }
};

/*
 * Encapsulate motor driver and timing information / calculations 
 */
class Motor : public AbstractMotor {
  private:
    ControlSystem *sys;
  
  public:
    // construct a motor representation for `dps` degrees per step
    Motor() { }

    void set_controller(ControlSystem *controller) {
      this->sys = controller; 
    }

    
    /*
     * Based on the gearing of the desired motor (1.8 deg / step) with a 1:51 planetary gearbox,
     * and coupled to a 16t pinion driving a 45T gear on the RA pulley, the required frequency
     * to drive the shaft at EARTH_ROTATION degrees / second will be ~ 1/3 Hz
     * 
     * This can be obtained by keeping a milisecond counter between start + end, and stepping the motor and resetting this counter when it exceeds 3000.
     * For this to be reasonably accurate the main loop should execute approximately at 10Hz or better.  An error accumulator could be used to shorten the
     * duration of subsequent steps perhaps.
     * 
     * Some sort of floating error accumulator that tries to tend back to zero might be appropriate.
     *  
     * Also needs to factor in tracking speed multipliers below 
     *  
     * Drives the primary telescope drive motor in accordance to the controller state machine where 1x 
     * will counteract the rotation of the earth (approximately 1/4 degree a minute)
     * 
     * state    | drive speed
     * -------------------
     * OFF .    | 0
     * FF .     | 5x 
     * TRACKING | 1x
     * REV .    | -1x
     */
    virtual void drive_motor(int steps) {
        debug(steps);
    }     

    void debug(int steps) {
      Serial.print("* Driving stepper ");
      Serial.print(steps);
      Serial.println(" steps.");
    }
};


ControlSystem sys;
Motor motor;

// trigger on interrupt for reverse direction
void set_rev(void) {
    if(!sys.is_interrupted()) {
      while(digitalRead(BTN_REVERSE) == 0) { // debounce toggle
        sys.set_reverse();
      }
    }
    sys.interrupt();
}

// trigger on interrupt for fast forward
void set_ff(void) {
    if(!sys.is_interrupted()) {
      while(digitalRead(BTN_FORWARD) == 0) { // debounce toggle
        sys.set_forward();
      }
    }
    sys.interrupt();
}

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  #endif
  
  sys = ControlSystem();
  motor = Motor();
  sys.set_motor(&motor);
  motor.set_controller(&sys);
  

  // set the three input pins
  pinMode(BTN_REVERSE, INPUT_PULLUP); // set the internal pull up resistor, unpressed button is HIGH
  pinMode(BTN_TRACKING, INPUT_PULLUP); // set the internal pull up resistor, unpressed button is HIGH
  pinMode(BTN_FORWARD, INPUT_PULLUP); // set the internal pull up resistor, unpressed button is HIGH

  // now register interrupts on them
  // NANO only supports interrupts on D2 + D3, so move btn_trk to main loop.
  attachInterrupt(digitalPinToInterrupt(BTN_REVERSE), set_rev, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_FORWARD), set_ff, FALLING);
}


void loop(){
  sys.tick();

  #ifdef DEBUG
    sys.debug();
  #endif

  // despam
  delay(100);
}
