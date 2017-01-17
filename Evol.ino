/*
  Midi Sequencer

  Anthony Batchelor <tony@ynottony.net>

  Planned features:
  64 step sequencer
  Euclidien Drums
  Song Mode
  Controller Mode
*/

// include the library code:
#include <Bounce2.h>
#include <LiquidCrystal.h>
#include <MIDI.h>

// Initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 7, 8, 9, 10);

const byte playPin = 6;
const byte stopPin = 5;
Bounce debouncer_play = Bounce();
Bounce debouncer_stop = Bounce();

// Create the Midi interface
struct MIDISettings : public midi::DefaultSettings {
    static const unsigned SysExMaxSize = 128;
    static const bool UseRunningStatus = false;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDI, MIDISettings);

bool ui_dirty = false;

byte heart[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
};

byte empty_heart[8] = {
    0b00000,
    0b01010,
    0b10101,
    0b10001,
    0b10001,
    0b01010,
    0b00100,
    0b00000,
};

byte play[8] = {
    0b11000,
    0b11100,
    0b11110,
    0b11111,
    0b11111,
    0b11110,
    0b11100,
    0b11000,
};

struct Note {
    byte note; // 0 to 127
    byte velocity;  // 0 to 127
};

struct Sequence {
    byte id;
    byte channel; // 16 channels
    Note* sequence;
};

// Clock
const byte CPQN = 24;  // Midi Clock
const byte PPQN = 96;  // Pulse MTC
int bpm = 120; // could be byte?
byte beat_chr = 0; // 1 for filled heart
unsigned int pulse_count = 0;
int microseconds_pqn[CPQN];  // So we can average the pulse lengths
unsigned long last_clock_pulse = micros();
unsigned long microseconds_per_pulse;
unsigned long next_clock_pulse = last_clock_pulse + microseconds_per_pulse;

const unsigned long oneMinuteInMicroseconds = 60000000;
byte timeSignatureNumerator = 4; // For show only, use the actual time signature numerator
byte timeSignatureDenominator = 4; // For show only, use the actual time signature denominator

// State
char mode[] = "SEQ";  // or DRM or PLY?
bool playing = false;
unsigned int beat = 0;
int transpose = -12;
bool legato = true;
bool internal_clock_source = true;

// Dummy sequence stuff
Note notes[] { // 64?
    {60, 100},
    {64, 100},
    {65, 100},
    {67, 120},
    {71, 127},
    {72, 110},
    {71, 100},
    {67, 100},
    {65, 100},
    {64, 100},
};

Sequence seq_1 = {
    1,
    1,
    notes,
};

void setup() {
    // Set up the custom characters
    lcd.createChar(0, empty_heart);
    lcd.createChar(1, heart);
    lcd.createChar(2, play);

    // Set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.write("  MIDIEVOL v0.1");
    lcd.setCursor(0, 1);
    lcd.write("Initialising...");

    // Set up the physical buttons
    pinMode(playPin, INPUT_PULLUP);
    pinMode(stopPin, INPUT_PULLUP);

    // Debounce the buttons
    debouncer_play.attach(playPin);
    debouncer_play.interval(1); // interval in ms
    debouncer_stop.attach(stopPin);
    debouncer_stop.interval(1); // interval in ms

    // Set up midi
    MIDI.begin(MIDI_CHANNEL_OMNI);  // Listen to all incoming messages
    MIDI.setHandleStart(handleStart);
    MIDI.setHandleContinue(handleContinue);
    MIDI.setHandleStop(handleStop);
    MIDI.setHandleSongPosition(handleSongPosition);
    MIDI.setHandleClock(handleClock);

    // Set up Serial for MIDI
    Serial1.begin(31250);
    // Set up Serial for debugging
    Serial.begin(115200);

    reset_midi();
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    // Clear UI
    lcd.setCursor(0, 0);
    lcd.write("                ");
    lcd.setCursor(0, 1);
    lcd.write("                ");
    // Draw the ui for the first time
    draw_ui();
}

void loop() {
    MIDI.read();

    // If internal clock and playing
    if (internal_clock_source && playing) {
        unsigned long now = micros();
        if (next_clock_pulse <= now) {
            next_clock_pulse = now + microseconds_per_pulse;

            // Fire clock pulse
            using namespace midi;
            MIDI.sendRealTime(Clock);

            handleClock();
        }
    }

    // Check physical buttons
    if (debouncer_play.update()) {
        if (debouncer_play.fell()) {
            if (playing) {
                handlePause();
            } else {
                handleStart();
            }
            ui_dirty = true;
        }
    }
    if (debouncer_stop.update()) {
        if (debouncer_stop.fell()) {
            handleStop();
            beat = 0;
        }
    }

    // Redraw the UI if necessary
    if (ui_dirty) {
        draw_ui();
    }
}

void reset_midi() {
    //debug("Resetting");
    // Reset each channel
    for (int i = 0; i <= 16; i++) {
        // Reset each note on each channel
        for (int j = 0; j <= 128; j++) {
            MIDI.sendNoteOff(j, 0, i);
        }
    }

    for (int i = 0; i < CPQN; i++) {
        microseconds_pqn[i] = 0;
    }
}

void draw_ui() {
    ui_dirty = false;
    // Write the mode
    lcd.setCursor(0, 0);
    lcd.print(mode);

    // Write Play State
    lcd.setCursor(10, 0);
    if (playing) {
        lcd.write(byte(2));  // Play button
    } else {
        lcd.print(" ");
    }

    // Write the bpm at the top right of the display
    lcd.setCursor(12, 0);
    lcd.write(byte(beat_chr));
    lcd.setCursor(13, 0);
    if (bpm < 100) {
        lcd.print(" ");
    }
    lcd.print(bpm);

    // Current song position
    lcd.setCursor(0, 1);
    lcd.print("b");
    lcd.setCursor(1, 1);
    lcd.print(beat);
}

void handleStart() {
    if (internal_clock_source) {
        next_clock_pulse = micros() + microseconds_per_pulse;
    }

    play_note();
    playing = true;
    ui_dirty = true;
};

void handleContinue() {
    play_note();
    playing = true;
    ui_dirty = true;
};

void handlePause() {
    playing = false;
    ui_dirty = true;
};

void handleStop() {
    playing = false;
    ui_dirty = true;
    beat = 0;
    reset_midi();
};

void handleSongPosition(unsigned int beats) {
    beat = beats;
    ui_dirty = true;
};

//void handleNoteOff(byte channel, byte note, byte velocity);
//void handleNoteOn(byte channel, byte note, byte velocity);
//void handleAfterTouchPoly(byte channel, byte note, byte pressure);
//void handleControlChange(byte channel, byte number, byte value);
//void handleProgramChange(byte channel, byte number);
//void handleAfterTouchChannel(byte channel, byte pressure);
//void handlePitchBend(byte channel, int bend);
//void handleSystemExclusive(byte* array, unsigned size);
//void handleTimeCodeQuarterFrame(byte data);
//void handleSongSelect(byte songnumber);
//void handleTuneRequest(void);
//void handleActiveSensing(void);
//void handleSystemReset(void);

void handleClock() {
    // Calculate BPM from 24ppqn
    unsigned long now = micros();
    unsigned long pulse_len;
    int new_bpm;

    microseconds_pqn[pulse_count % CPQN] = now - last_clock_pulse;
    last_clock_pulse = now;

    // Handle playing notes at the right time
    pulse_count += 1;
    if (pulse_count % 6 == 0) {
        beat += 1;  // Increment the sequences beat counter
        play_note();
        ui_dirty = true;
    }

    // Handle the beating clock
    if (pulse_count == CPQN) {
        beat_chr = !beat_chr; // Swap the beat character
        ui_dirty = true;
        pulse_count = 0;

        // If using an external clock, work out the bpm
        if (internal_clock_source == false) {
            // Only calculate the current bpm once per beat
            // Average pulse length
            unsigned long sum = 0L;
            for (int i = 0 ; i < CPQN ; i++) {
                if (microseconds_pqn[i] > 0) {
                    sum += microseconds_pqn[i];
                }
            }
            pulse_len = sum / CPQN;

            new_bpm = bpm_from_pulse_len(pulse_len);
            if (new_bpm != bpm) {
                bpm = new_bpm;
                ui_dirty = true;
            }
        }
    }
}

int bpm_from_pulse_len(unsigned long pulse_len) {
    return (oneMinuteInMicroseconds / (pulse_len * CPQN)) * (timeSignatureDenominator / 4.0);
}

unsigned long pulse_len_from_bpm(int bpm) {
    return oneMinuteInMicroseconds / (bpm * CPQN);
}

void play_note() {
    int current_note = beat % (sizeof(notes) / sizeof(Note));
    int last_note;
    if (current_note > 0) {
        last_note = current_note - 1;
    } else {
        last_note = (sizeof(notes) / sizeof(Note)) - 1;
    }

    if (legato) {
        MIDI.sendNoteOn(notes[current_note].note + transpose, notes[current_note].velocity, 1);
        MIDI.sendNoteOff(notes[last_note].note + transpose, notes[last_note].velocity, 1);
    } else {
        MIDI.sendNoteOff(notes[last_note].note + transpose, notes[last_note].velocity, 1);
        MIDI.sendNoteOn(notes[current_note].note + transpose, notes[current_note].velocity, 1);

    }
    lcd.setCursor(10, 1);
    lcd.print(current_note);

    lcd.setCursor(13, 1);
    lcd.print(notes[current_note].note + transpose);
};
