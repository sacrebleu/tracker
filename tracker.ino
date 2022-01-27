/* 
 *  reads input from an analog pin, and on v> v_crit it sets D13 to HIGH for 3 seconds.
 */
// set debug to 0 to stop sysouts
#define DEBUG 1

// state machine variables
#define STATE_OFF 0
#define STATE_TRACKING 3
#define STATE_REV 2
#define STATE_FWD 1

// LED output pins begin
#define LED_REVERSE 6      // LED that lights when the drive is reversing
#define LED_TRACK 7    // LED that lights when the drive is tracking
#define LED_FORWARD 8  // LED that lights when the drive is fast-forwarding
// LED output pins end

// motor output pins

// motor output pins end

// inputs
#define BTN_REVERSE 2
#define BTN_FORWARD 3
#define BTN_TRACKING 9

/* class model for an LED */
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

/* model the system state in a single class */
class ControlSystem {
  private:
    Led reverse = Led(LED_REVERSE);
    Led forward = Led(LED_FORWARD);
    Led track   = Led(LED_TRACK);

    int state;
    volatile bool interrupted = false;
    
  public:
    ControlSystem() {
      init();
    }

    void init() {
      set_off();
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

    // this is not toggling correctly, needs more investigation into why.
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

    void render() {
      reverse.set(is_reverse());
      forward.set(is_forward());
      track.set(is_tracking());
    }

    void tick() {
      render();
      
      toggle_tracking();
      
      clear_interrupt();
    }

    void debug() {
      Serial.print("Current ControlSystem state is: ");
      Serial.println(state);
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


ControlSystem sys;

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

// outputs
//int led_rev = 6;   // LED that lights when the drive is reversing
//int led_trk = 7; // LED that lights when the drive is tracking
//int led_fwd = 8; // LED that lights when the drive is fast-forwarding

//int state = STATE_OFF;


// investigate using some structs
// single vars everywhere are messy

/* 
 * short hand state setter 
 * int r - reverse led state
 * int t - track led state
 * int f - fast-forward led state
 */
//void set_leds(int r, int t, int f) {
//  digitalWrite(led_rev, r);
//  digitalWrite(led_trk, t);
//  digitalWrite(led_fwd, f);  
//}

// set the leds to indicate the current controller state
//void show_state(int state) {
//  sys.tick();
// 
//  interrupted=false;
//}




/*
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
void drive_stepper(void) {
  
}

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  #endif
  
//  pinMode(led_rev, OUTPUT);
//  pinMode(led_trk, OUTPUT);
//  pinMode(led_fwd, OUTPUT);

  sys = ControlSystem();
  

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
  // this should be an ISR
//  toggle_trk();
  sys.tick();

  #ifdef DEBUG
    sys.debug();
//  }
  #endif

  // display button states
//  show_state(state);
  drive_stepper();
  // rotate the stepper motor in accordance with the system state

  // despam
  delay(300);
}
