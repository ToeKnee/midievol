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
unsigned long status_timeout = micros();

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
byte sequence_id = 0;
struct Sequence {
    byte id; // Save location - 0 indexed
    byte channel; // 16 channels
    byte length; // up to 64
    byte beat_division; // BeatDivision
    byte note_length; // BeatDivision
    bool note_length_from_sequence;
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

Note sequences_notes[16][64] {
{
    {60, 100, quarter},
    {REST, 100, quarter},
    {60, 100, quarter},
    {TIE, 120, quarter},
    {TIE, 100, quarter},
    {TIE, 100, quarter},
    {30, 100, quarter},
    {90, 120, quarter},
},
{
    {60, 100, quarter},
    {61, 100, quarter},
    {62, 100, quarter},
    {63, 120, quarter},
    {64, 100, quarter},
    {65, 100, quarter},
    {66, 100, quarter},
    {67, 120, quarter},
},
};
Sequence sequences[16] {
    {
        1,
        1,
        7,  // 0 indexed
        quarter,
        quarter,
        true,
    },
    {
        1,
        10,
        7,  // 0 indexed
        quarter,
        quarter,
        true,
    },
};

// Address and sizes needed for loading / saving
int size_of_sequence = sizeof(sequences[0]) + sizeof(sequences_notes[0]);

void setup() {
    // Set up the custom characters
    lcd.createChar(0, empty_heart);
    lcd.createChar(1, heart);
    lcd.createChar(2, play);
    lcd.createChar(3, shift_char);

    // Set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.print(F("  MIDIEVOL v0.1"));
    lcd.setCursor(0, 1);
    lcd.print(F("Initialising..."));

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

    // Set up the drums
    init_drums();

    // Set up the kill list
    init_kill_list();

    // Reset midi
    panic();

    // Work out the pulse length
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    // Set the mode
    mode = sequencer;

    // Clear UI
    lcd.clear();
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
    handleButtons();

    // Check the 4 x 4 matrix
    handleEncoder(0, encoder_0->getValue());
    handleEncoder(1, encoder_1->getValue());
    handleEncoder(2, encoder_2->getValue());
    handleEncoder(3, encoder_3->getValue());

    handleEncoder(4, encoder_4->getValue());
    handleEncoder(5, encoder_5->getValue());
    handleEncoder(6, encoder_6->getValue());
    handleEncoder(7, encoder_7->getValue());

    // And the buttons
    handleEncoderButton(0, encoder_0->getButton());
    handleEncoderButton(1, encoder_1->getButton());
    handleEncoderButton(2, encoder_2->getButton());
    handleEncoderButton(3, encoder_3->getButton());

    handleEncoderButton(4, encoder_4->getButton());
    handleEncoderButton(5, encoder_5->getButton());
    handleEncoderButton(6, encoder_6->getButton());
    handleEncoderButton(7, encoder_7->getButton());

    // Redraw the UI if necessary
    if (ui_dirty) {
        draw_ui();
    }
}
