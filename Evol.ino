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
#include <ClickEncoder.h>
#include <LiquidCrystal.h>
#include <MIDI.h>
#include <TimerOne.h>

// Initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 7, 8, 9, 10);
String status_display; // Please keep <= 10 chars

// Physical buttons
const byte playPin = 6;
const byte stopPin = 5;
const byte shiftPin = 4;
Bounce debouncer_play = Bounce();
Bounce debouncer_stop = Bounce();
Bounce debouncer_shift = Bounce();

// 4 x 4 Matrix
const byte stepsPerNotch = 4;
ClickEncoder *encoder_0, *encoder_1, *encoder_2, *encoder_3;
ClickEncoder *encoder_4, *encoder_5, *encoder_6, *encoder_7;

// Create the Midi interface
struct MIDISettings : public midi::DefaultSettings {
    static const unsigned SysExMaxSize = 128;
    static const bool UseRunningStatus = false;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial1, MIDI, MIDISettings);

bool ui_dirty = false;

// Custom Characters
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

byte shift_char[8] = {
    0b00100,
    0b01010,
    0b10001,
    0b01010,
    0b01010,
    0b01010,
    0b01110,
    0b00000
};

const byte REST = 128;
const byte TIE = 129;
struct Note {
    byte note; // 0 to 127
    byte velocity;  // 0 to 127
    byte note_length; // BeatDivision
};

struct KillListNote {
    byte note;
    byte channel;
    unsigned int ticks_left;
};
typedef enum BeatDivision{
    whole,
    half,
    quarter,
    eigth,
    sixteenth,
    thirtysecondth
};
const byte beat_division_map[] = {96, 48, 24, 12, 6, 3};

struct Sequence {
    byte id;
    byte channel; // 16 channels
    Note* sequence;
    byte length; // up to 256  -  0 indexed
    byte beat_division; // BeatDivision
    byte note_length; // BeatDivision
    bool note_length_from_sequence;
};

// Clock
const byte CPQN = 24;  // Midi Clock
const byte PPQN = 96;  // Pulse MTC

byte bpm = 120;
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
typedef enum Mode{
    sequencer,
    drum,
    song,
};
Mode mode;
byte display_note;
bool shift = false;
bool playing = false;
unsigned int beat = 0;
int transpose = 0;
bool internal_clock_source = true;

// Kill list of playing notes
KillListNote kill_list_notes[128];

// Dummy sequence stuff
Note notes[256] {
    {60, 100, quarter},
    {REST, 100, quarter},
    {60, 100, quarter},
    {TIE, 120, quarter},
    {TIE, 100, quarter},
    {TIE, 100, quarter},
    {30, 100, quarter},
    {90, 120, quarter},
};

Sequence sequence = {
    1,
    1,
    notes,
    7,  // 0 indexed
    quarter,
    quarter,
    true,
};

void setup() {
    // Set up the custom characters
    lcd.createChar(0, empty_heart);
    lcd.createChar(1, heart);
    lcd.createChar(2, play);
    lcd.createChar(3, shift_char);

    // Set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.write("  MIDIEVOL v0.1");
    lcd.setCursor(0, 1);
    lcd.write("Initialising...");

    // Set up the physical buttons
    pinMode(playPin, INPUT_PULLUP);
    pinMode(stopPin, INPUT_PULLUP);
    pinMode(shiftPin, INPUT_PULLUP);

    // Set up 4 x 4 encoder matrix
    encoder_0 = new ClickEncoder(22, 23, 24, stepsPerNotch);
    encoder_0->setAccelerationEnabled(true);
    encoder_1 = new ClickEncoder(25, 26, 27, stepsPerNotch);
    encoder_1->setAccelerationEnabled(true);
    encoder_2 = new ClickEncoder(28, 29, 30, stepsPerNotch);
    encoder_2->setAccelerationEnabled(true);
    encoder_3 = new ClickEncoder(31, 32, 33, stepsPerNotch);
    encoder_3->setAccelerationEnabled(true);

    encoder_4 = new ClickEncoder(34, 35, 36, stepsPerNotch);
    encoder_4->setAccelerationEnabled(true);
    encoder_5 = new ClickEncoder(37, 38, 39, stepsPerNotch);
    encoder_5->setAccelerationEnabled(true);
    encoder_6 = new ClickEncoder(40, 41, 42, stepsPerNotch);
    encoder_6->setAccelerationEnabled(true);
    encoder_7 = new ClickEncoder(43, 44, 45, stepsPerNotch);
    encoder_7->setAccelerationEnabled(true);

    // Set up encoder timers
    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);

    // Debounce the buttons
    debouncer_play.attach(playPin);
    debouncer_play.interval(1); // interval in ms
    debouncer_stop.attach(stopPin);
    debouncer_stop.interval(1); // interval in ms
    debouncer_shift.attach(shiftPin);
    debouncer_shift.interval(1); // interval in ms

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

    // Set up the kill list
    init_kill_list();

    // Reset midi
    panic();

    // Work out the pulse length
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    // Set the mode
    mode = sequencer;

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
    if (debouncer_shift.update()) {
        if (debouncer_shift.fell()) {
            shift = true;
            ui_dirty = true;
        } else if (debouncer_shift.rose()) {
            shift = false;
            ui_dirty = true;
        }
    }

    // Check the 4 x 4 matrix
    handleEncoder(0, encoder_0->getValue());
    handleEncoder(1, encoder_1->getValue());
    handleEncoder(2, encoder_2->getValue());
    handleEncoder(3, encoder_3->getValue());

    handleEncoder(4, encoder_4->getValue());
    handleEncoder(5, encoder_5->getValue());
    handleEncoder(6, encoder_6->getValue());
    handleEncoder(7, encoder_7->getValue());


    // Redraw the UI if necessary
    if (ui_dirty) {
        draw_ui();
    }
}

void handleEncoder(byte encoder, byte value) {
    if (value != 0) {
        // Sequencer mode
        if (mode == sequencer) {
            // Default unshifted case, edit the note
            if (!shift) {
                /* For the first sixteen the encoder and the note number match.
                   Will have to offset this for the correct page when we
                   implement the full 64 steps.
                */
                update_note(encoder, value);
            } else {
                // Shift mode, each encoder has a different alternative function
                if (encoder == 0) {  // Handle BPM changes
                    adjustBPM(value);
                } else if (encoder == 1) {  // Handle Beat Division Changes
                    adjustBeatDivision(value);
                } else if (encoder == 2) {  // Handle Note Length Changes
                    adjustNoteLength(value);
                } else if (encoder == 4) {  // Handle Sequence Length Changes
                    adjustSequenceLength(value);
                }
            }
        }
    }
}

void update_note(byte note, byte value) {
    notes[note].note += value;
    if (notes[note].note == 255) { // Looped round 0 backwards
        notes[note].note = 129;
    } else if (notes[note].note > 129) {  // 127 Midi notes + 2 special cases
        notes[note].note = 0;
    }

    display_note = note;
    note_name(status_display, notes[display_note].note);
    status_display += " ";
    status_display += notes[display_note].velocity;

    ui_dirty = true;
}

void timerIsr() {
    encoder_0->service();
    encoder_1->service();
    encoder_2->service();
    encoder_3->service();

    encoder_4->service();
    encoder_5->service();
    encoder_6->service();
    encoder_7->service();
}

void panic() {
    // Reset each channel
    for (int i = 0; i < 16; i++) {
        // Reset each note on each channel
        for (int j = 0; j < 128; j++) {
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
    if (mode == sequencer) {
        lcd.print("SEQ");
    }

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
    if (bpm < 10) {
        lcd.print("  ");
    } else if (bpm < 100) {
        lcd.print(" ");
    }
    lcd.print(bpm);

    // Display Shift status
    lcd.setCursor(4, 0);
    if (shift) {
        lcd.write(byte(3));
    } else {
        lcd.write(" ");
    }

    // Display Status
    lcd.setCursor(0, 1);
    while (status_display.length() <= 10) {
        status_display += " ";
    }
    lcd.print(status_display);

    // Display last note edited
    lcd.setCursor(12, 1);
    String note_display;
    note_name(note_display, notes[display_note].note);
    lcd.print(note_display);
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
    handle_kill_all();
};

void handleStop() {
    playing = false;
    ui_dirty = true;
    beat = 0;
    handle_kill_all();
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
    if (pulse_count % beat_division_map[sequence.beat_division] == 0) {
        beat += 1;  // Increment the sequences beat counter
        play_note();
        pulse_count = 0;
        ui_dirty = true;
    }

    // Handle the kill list
    handle_kill_list();

    // Handle the beating clock
    if (pulse_count == CPQN) {
        beat_chr = !beat_chr; // Swap the beat character
        ui_dirty = true;

        // If using an external clock, work out the bpm
        if (internal_clock_source == false) {
            // Only calculate the current bpm once per beat
            // Average pulse length
            unsigned long sum = 0;
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

void adjustBPM(byte adjustment) {
    bpm += adjustment;
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    status_display = "BPM: ";
    status_display += bpm;

    ui_dirty = true;
}

void adjustBeatDivision(byte adjustment) {
    sequence.beat_division += adjustment;

    // Handle looping round the values
    if (sequence.beat_division > 127) {
        sequence.beat_division = 5;
    }

    status_display = "Beat: ";
    // There are 6 possible beat divisions. Choose one.
    sequence.beat_division = sequence.beat_division % 6;
    if (sequence.beat_division == 0) {
        status_display += "1/1";
    } else if (sequence.beat_division == 1) {
        status_display += "1/2";
    } else if (sequence.beat_division == 2) {
        status_display += "1/4";
    } else if (sequence.beat_division == 3) {
        status_display += "1/8";
    } else if (sequence.beat_division == 4) {
        status_display += "1/16";
    } else if (sequence.beat_division == 5) {
        status_display += "1/32";
    }

    ui_dirty = true;
}

void adjustNoteLength(byte adjustment) {
    sequence.note_length += adjustment;

    // Handle looping round the values
    if (sequence.note_length > 127) {
        sequence.note_length = 5;
    }

    status_display = "Gate: ";
    // There are 6 possible beat divisions. Choose one.
    sequence.note_length = sequence.note_length % 6;
    if (sequence.note_length == 0) {
        status_display += "1/1";
    } else if (sequence.note_length == 1) {
        status_display += "1/2";
    } else if (sequence.note_length == 2) {
        status_display += "1/4";
    } else if (sequence.note_length == 3) {
        status_display += "1/8";
    } else if (sequence.note_length == 4) {
        status_display += "1/16";
    } else if (sequence.note_length == 5) {
        status_display += "1/32";
    }

    ui_dirty = true;
}

void adjustSequenceLength(byte adjustment) {
    sequence.length += adjustment;

    status_display = "Steps: ";
    status_display += sequence.length + 1;  // Display off by one.

    ui_dirty = true;
}

int bpm_from_pulse_len(unsigned long pulse_len) {
    return (oneMinuteInMicroseconds / (pulse_len * CPQN)) * (timeSignatureDenominator / 4.0);
}

unsigned long pulse_len_from_bpm(int bpm) {
    return oneMinuteInMicroseconds / (bpm * CPQN);
}

void play_note() {
    byte current_note = beat % (sequence.length + 1);  // Sequence length is 0 indexed
    unsigned int ticks_left;
    if (sequence.note_length_from_sequence) {
        ticks_left = beat_division_map[sequence.note_length];
    } else {
        ticks_left = beat_division_map[notes[current_note].note_length];
    }

    // Display the playing note
    display_note = current_note;
    // Check for tied notes
    byte i = 1;  // Start at 1, 0 is current_note
    if (current_note != TIE) {
        while (notes[(beat + i) % (sequence.length + 1)].note == TIE) { // Sequence length is 0 indexed

            if (sequence.note_length_from_sequence) {
                ticks_left += beat_division_map[sequence.note_length];
            } else {
                ticks_left += beat_division_map[notes[current_note].note_length];
            }

            i++;
            if (i == 255) {
                // Hopefully there aren't 255 TIE's, but you never know.
                break;
            }
        }
    }

    // Play the note
    if (notes[current_note].note != TIE && notes[current_note].note != REST) {
        MIDI.sendNoteOn(notes[current_note].note + transpose, notes[current_note].velocity, sequence.channel);
        add_to_kill_list(notes[current_note].note + transpose, sequence.channel, ticks_left);
    }
};

void init_kill_list() {
    // Set default values for the kill list
    KillListNote kill_note {
        0,
        0,
        0
    };

    for (byte i = 0; i < sizeof(kill_list_notes) / sizeof(KillListNote); i++) {
        kill_list_notes[i] = kill_note;
    }
}


void add_to_kill_list(byte note, byte channel, unsigned int note_length_ticks) {
    for (byte i = 0; i < sizeof(kill_list_notes) / sizeof(KillListNote); i++) {
        if (kill_list_notes[i].ticks_left == 0) {
            kill_list_notes[i].note = note;
            kill_list_notes[i].channel = channel;
            kill_list_notes[i].ticks_left = note_length_ticks;
            break;
        }
    }
}

void handle_kill_list() {
    for (byte i = 0; i < sizeof(kill_list_notes) / sizeof(KillListNote); i++) {
        if (kill_list_notes[i].ticks_left == 1) {
            MIDI.sendNoteOff(kill_list_notes[i].note, 0, kill_list_notes[i].channel);
        }

        // Reduce the number of ticks
        if (kill_list_notes[i].ticks_left > 0) {
            kill_list_notes[i].ticks_left -= 1;
        }
    }
}

void handle_kill_all() {
    for (byte i = 0; i < sizeof(kill_list_notes) / sizeof(KillListNote); i++) {
        MIDI.sendNoteOff(kill_list_notes[i].note, 0, kill_list_notes[i].channel);
        kill_list_notes[i].ticks_left = 0;
    }
}

void note_name(String &note, byte note_number) {
    const String Notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    // Handle special cases first
    if (note_number == REST) {
        note = "REST";
    } else if (note_number == TIE) {
        note = "TIE ";
    } else {
        note = Notes[(note_number % 12)];
        if (note.length() == 1) {
            note += " ";
        }
        note += (note_number / 12); // Octave
        if (note.length() == 3) {
            note += " ";
        }
    }
}
