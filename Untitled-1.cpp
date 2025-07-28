const int BUTTON1_PIN = 8;     // [PCI SIMULATED] Part of PCI Group 0
const int BUTTON2_PIN = 9;     // [PCI SIMULATED] Part of PCI Group 0
const int LED1_PIN = 12;
const int LED2_PIN = 11;
const int LED3_PIN = 10;
const int LDR_PIN = A0;        // Used in Timer-based periodic task

volatile bool button1Event = false;  // [FLAG-BASED LOGIC] Set in Timer ISR when button 1 pressed
volatile bool button2Event = false;  // [FLAG-BASED LOGIC] Set in Timer ISR when button 2 pressed
bool button1State = false;           // Tracks LED1 state
bool button2State = false;           // Tracks LED2 state
bool bothButtonsFlag = false;        // [GROUPED LOGIC] Becomes true when both buttons pressed within 3s

unsigned long button1LastPressed = 0;
unsigned long button2LastPressed = 0;
const unsigned long pressWindow = 3000; // Time window for grouped logic

// Tracks last read pin values (to detect state changes)
bool lastButton1Read = HIGH;
bool lastButton2Read = HIGH;

// [TIMER INTERRUPT]
// This ISR fires every 5ms (200Hz) and:
// Simulates PCI by checking D8 and D9 state
// Performs a periodic task every 1s: read LDR sensor
ISR(TIMER1_COMPA_vect) {
  // [PCI SIMULATED] Check buttons D8 and D9 for state changes
  bool read1 = digitalRead(BUTTON1_PIN);
  bool read2 = digitalRead(BUTTON2_PIN);


  if (read1 != lastButton1Read) {
    lastButton1Read = read1;
    if (read1 == LOW) button1Event = true; // [INTERRUPT-SAFE] set flag only
  }

  if (read2 != lastButton2Read) {
    lastButton2Read = read2;
    if (read2 == LOW) button2Event = true; // [INTERRUPT-SAFE]
  }

  // [TIMER INTERRUPT] – Every 1s: read and log LDR value
  static unsigned long counter = 0;
  counter++;
  if (counter >= 200) { // 200 * 5ms = 1s
    counter = 0;
    Serial.println("[Timer] Periodic check – system running");  // [TRACEABLE OUTPUT]
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  setupTimer1();  
}

// === Timer1 Setup ===
// [PROPER TIMER SETUP] 200Hz = every 5ms (for button polling & 1s counter)
void setupTimer1() {
  noInterrupts();                // [INTERRUPT-SAFE] 
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 1249;                  
  TCCR1B |= (1 << WGM12);       
  TCCR1B |= (1 << CS11) | (1 << CS10); 
  TIMSK1 |= (1 << OCIE1A);       
  interrupts();                  
}


void loop() {
  handleButtonEvents();        // [INPUT HANDLING]
  evaluateGroupedLogic();      // [GROUPED LOGIC]
}

// [FLAG-BASED LOGIC] ISR sets flags, logic processed here
void handleButtonEvents() {
  if (button1Event) {
    button1Event = false;
    button1State = !button1State;
    digitalWrite(LED1_PIN, button1State);  // [ACTUATE LED1]
    Serial.println("Button 1 Pressed, LED1 toggled");  
    button1LastPressed = millis();
  }

  if (button2Event) {
    button2Event = false;
    button2State = !button2State;
    digitalWrite(LED2_PIN, button2State);  // [ACTUATE LED2]
    Serial.println("Button 2 Pressed, LED2 toggled");  
    button2LastPressed = millis();
  }
}

// === Evaluate Grouped Logic ===
// [GROUPED LOGIC] If both buttons pressed within 3s, turn on LED3
void evaluateGroupedLogic() {
  if (abs((long)button1LastPressed - (long)button2LastPressed) <= pressWindow) {
    if (!bothButtonsFlag) {
      bothButtonsFlag = true;
      digitalWrite(LED3_PIN, HIGH);
      Serial.println("Both buttons pressed within 3 seconds, LED3 ON");
    }
  } else {
    if (bothButtonsFlag) {
      bothButtonsFlag = false;
      digitalWrite(LED3_PIN, LOW);
      Serial.println("Button combo timeout, LED3 OFF");
    }
  }
}
