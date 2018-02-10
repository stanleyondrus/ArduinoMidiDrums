//  *****************************************************************************************************************
//  *                                                                                                               *
//  *                                              Arduino MIDI Drums                                               *
//  *                                        2017 (c) StanleyProjects.com                                           *
//  *                                                                                                               *
//  *   Tutorial, Schematics, Kit - http://stanleyprojects.com/projects/programming/arduinodrums/                   *
//  *                                                                                                               *
//  *   Required Hardware:                                                                                          *
//  *     1. 1x Arduino Nano                                                                                        *
//  *     2. 8x Piezoelectric Transducer                                                                            *
//  *     3. 8x 1MΩ Resistor                                                                                        *
//  *     4. 1x Momentary Footswitch                                                                                *
//  *                                                                                                               *
//  *   Required Software:                                                                                          *
//  *     1. Hairless MIDI - http://projectgus.github.io/hairless-midiserial/                                       *
//  *     2. loopMIDI - http://www.tobias-erichsen.de/software/loopmidi.html                                        *
//  *     3. Sample-based drum software synthesizer (e.g. EZdrummer, Superior Drummer,...)                          *
//  *                                                                                                               *
//  *****************************************************************************************************************

#define NUMBER_OF_PADS 8        // number of drum pads (analog inputs)

#define HI_HAT_PEDAL_PIN 2      // digital port number of hi-hat switch
#define HI_HAT_PAD_PIN 2        // analog port number of hi-hat piezo
#define HI_HAT_CLOSED_NOTE 63   // closed hi-hat MIDI note

#define VELOCITY_ACTIVE 1       // velocity sensitive ON-OFF  [1-0]
#define LOG_MAPPING 0           // 1-logarithmic 0-linear velocity mapping
#define MIDI_CHANNEL 1          // MIDI channel [1-16]

#define MIDI_MAX_VELOCITY 127
#define MIDI_NOTE_ON 0b1001     // MS nibble for note on status message
#define MIDI_NOTE_OFF 0b1000    // MS nibble for note off status message

uint8_t padNote[NUMBER_OF_PADS] = {60, 62, 64, 65, 67, 69, 71, 72};               // MIDI notes [0-127]
uint16_t padThreshold[NUMBER_OF_PADS] = {400, 400, 400, 400, 400, 400, 400, 400};  // trigger treshold values [0-1023]
uint16_t padCycles[NUMBER_OF_PADS] = {90, 90, 90, 90, 90, 90, 90, 90};             // number of cycles before the second trigger is allowed [0-65535]

uint8_t activePad;                          // each bit represents a pad state
uint8_t activeHiHat;                        // hi-hat state
uint16_t padCurrentCycles[NUMBER_OF_PADS];   // number of cycles since the pad was triggered

void setup() {
  pinMode(HI_HAT_PEDAL_PIN, INPUT_PULLUP);                                 // intialize the hi-hat pedal pin as input pullup
  Serial.begin(115200);                                                    // initialize the serial port at baud rate 115200
}

void loop() {
  for (uint8_t pin = 0; pin < NUMBER_OF_PADS; pin++) {                     // loop through all of the pads
    uint16_t val = analogRead(pin);                                         // read the input pin   
    if ((val > padThreshold[pin]) && (!padActive(pin))) {                  // if hit strong enough

      val = VELOCITY_ACTIVE ? velocityAlgorithm(val,LOG_MAPPING) : MIDI_MAX_VELOCITY;  // if velocity sensitive, calculate the new value, otherwise apply the maximum value
      uint8_t activeHiHat = checkHiHat(pin) ? 1 : 0;                       // if pedal pressed and current pin is hi-hat, set hi-hat active
      uint8_t note = activeHiHat ? HI_HAT_CLOSED_NOTE : padNote[pin];      // if hi-hat active, set note to HI_HAT_CLOSED_NOTE, otherwise use the padNote[pin]

      midi_tx_note_on(note, val);                                          // send a note on MIDI message
      padCurrentCycles[pin] = 0;                                           // reset the current pad cycle counter
      activePad |= 1 << pin;                                               // set corresponding bit (active flag)
    }
    
    if (padActive(pin)) {                                                  // enter if pad is active
      padCurrentCycles[pin] += 1;                                          // increment the cycle counter by 1

      if (padCurrentCycles[pin] >= padCycles[pin]) {                       // enter if cycle counter reached the desired number of cycles

        if (pin == HI_HAT_PAD_PIN && activeHiHat) {                        // enter if hi-hat active and current pin is hi-hat
          midi_tx_note_off(HI_HAT_CLOSED_NOTE);                            // send closed hi-hat note off MIDI message
        }
        else {
          midi_tx_note_off(padNote[pin]);                                  // send a note off MIDI message
        }
            
        activePad &= ~(1 << pin);                                          // clear corresponding bit (active flag)
      }
    }
  }
}

// remap the val from [0-1023] to [0-127]
uint8_t velocityAlgorithm(uint16_t val, uint8_t logswitch) {
  if (logswitch) {
     return log(val + 1)/ log(1024) * 127;
  }
    return (val - 0) * (127 - 0) / (1023 - 0) + 0;                       
}

uint8_t checkHiHat(uint8_t currentPin) {                                   // check if hi-hat pedal pressed and if current pin is hi-hat
  return (currentPin == HI_HAT_PAD_PIN && pedalPressed()) ? 1 : 0;
}

uint8_t pedalPressed() {                                                   // check if pedal pressed
  return digitalRead(HI_HAT_PEDAL_PIN) ? 0 : 1;
}

uint8_t padActive(uint8_t currentPin) {                                    // check if current pad active
  return (activePad >> currentPin) & 1;
}

void midi_tx_note_on(uint8_t pitch, uint8_t velocity) {                    // send note on MIDI message
  Serial.write((MIDI_NOTE_ON << 4) | (MIDI_CHANNEL - 1));
  Serial.write(pitch);
  Serial.write(velocity);
}

void midi_tx_note_off(uint8_t pitch) {                                     // send note off MIDI message
  Serial.write((MIDI_NOTE_OFF << 4) | (MIDI_CHANNEL - 1));
  Serial.write(pitch);
  Serial.write(0);
}
