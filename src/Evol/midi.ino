// TODO
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


void panic() {
    // Send MIDI SystemReset
    using namespace midi;
    MIDI.sendRealTime(SystemReset);

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


void handleClock() {
    // Calculate BPM from 24ppqn
    unsigned long now = micros();
    unsigned long pulse_len;

    microseconds_pqn[pulse_count % CPQN] = now - last_clock_pulse;
    last_clock_pulse = now;

    // Handle playing notes at the right time
    pulse_count += 1;
    if (pulse_count % beat_division_map[sequences[sequence_id].beat_division] == 0) {
        if (mode == SEQUENCER) {
            play_note();
        } else if (mode == DRUM) {
            play_drums();
        }
        pulse_count = 0;

        beat += 1;  // Increment the sequences beat counter
        ui_dirty = true;
    }

    // Handle the kill list
    handle_kill_list();

    // Handle the beating clock
    if (pulse_count == (CPQN - 1)) {
        beat_chr = !beat_chr; // Swap the beat character
        ui_dirty = true;

        // If using an external clock, work out the bpm
        if (internal_clock_source == false) {
            // Only calculate the current bpm once per beat
            // Average pulse length
            int new_bpm;
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

void handleStart() {
    if (internal_clock_source) {
        next_clock_pulse = micros() + microseconds_per_pulse;
    }

    // Send MIDI Start
    using namespace midi;
    MIDI.sendRealTime(Start);

    playing = true;
    ui_dirty = true;
};

void handleContinue() {
    // Send MIDI Continue
    using namespace midi;
    MIDI.sendRealTime(Continue);

    playing = true;
    ui_dirty = true;
};

void handlePause() {
    // Send MIDI Stop
    using namespace midi;
    MIDI.sendRealTime(Stop);

    playing = false;
    ui_dirty = true;
    handle_kill_all();
};

void handleStop() {
    // Send MIDI Stop
    using namespace midi;
    MIDI.sendRealTime(Stop);

    playing = false;
    ui_dirty = true;
    beat = 0;
    handle_kill_all();
};

void handleSongPosition(unsigned int beats) {
    beat = beats;
    ui_dirty = true;
};



void play_note() {
    byte current_note = beat % (sequences[sequence_id].length + 1);  // Sequence length is 0 indexed
    unsigned int ticks_left;
    ticks_left = beat_division_map[sequences_notes[sequence_id][current_note].note_length];

    // Display the playing note
    display_note = current_note;
    // Check for tied notes
    byte i = 1;  // Start at 1, 0 is current_note
    if (current_note != TIE) {
        while (sequences_notes[sequence_id][(beat + i) % (sequences[sequence_id].length + 1)].note == TIE) { // Sequence length is 0 indexed
            ticks_left += beat_division_map[sequences_notes[sequence_id][current_note].note_length];
            i++;
            if (i == 255) {
                // Hopefully there aren't 255 TIE's, but you never know.
                break;
            }
        }
    }

    // Play the note
    if (sequences_notes[sequence_id][current_note].note != TIE && sequences_notes[sequence_id][current_note].note != REST) {
        MIDI.sendNoteOn(sequences_notes[sequence_id][current_note].note + transpose, sequences_notes[sequence_id][current_note].velocity, sequences[sequence_id].channel + 1);
        add_to_kill_list(sequences_notes[sequence_id][current_note].note + transpose, sequences[sequence_id].channel + 1, ticks_left);
    }
};

void play_drums() {
    unsigned int ticks_left = beat_division_map[drum_sequence.beat_division];
    bool display_drums = status_timeout < micros();
    if (display_drums) {
        status_timeout = next_clock_pulse + 100000;
        ui_dirty = true;
        status_display = F("");
    }
    for (int i=0; i < 16; i++) {
        byte current_note = beat % (drum_tracks[i].length);  // Sequence length is 0 indexed
        // Play the drum
        if (calculated_drum_tracks[i][current_note]) {
            if (display_drums) {
                status_display += F("x");
            }
            MIDI.sendNoteOn(drum_tracks[i].note, random(100, 127),  drum_sequence.channel + 1);
            add_to_kill_list(drum_tracks[i].note, drum_sequence.channel + 1, ticks_left);
        } else {
            if (display_drums) {
                status_display += F(".");
            }
        }
    }

    // Serial.println(status_display);
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
