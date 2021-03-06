
typedef enum SequenceTrackInitMode {
    EMPTY_SEQUENCE,
    SIMPLE_SEQUENCE,
    RANDOM_SEQUENCE,
};
SequenceTrackInitMode sequence_track_init_mode = EMPTY_SEQUENCE;


void init_sequencer() {
    for (int i = 0; i < 8; i++){
        sequences[i].id = i;
        sequences[i].channel = i + 1;
        sequences[i].length = 64;
        sequences[i].beat_division = QUARTER;

        for (int j = 0; j < 64; j++){
            sequences_notes[i][j].note = random(129);
            sequences_notes[i][j].velocity = random(100, 127);
            sequences_notes[i][j].note_length = QUARTER;
            sequence_edit_mode[j] = SEQUENCE_NOTE;
        }
    }

    // Load sequence 1
    loadSequence(true);
}

void initSequenceChoice(byte value) {
    // Update the value with the current state
    value += int(sequence_track_init_mode);
    if (value > 127) {  // Handle wrapping backwards
        value = 4;
    } else if (sequence_track_init_mode > 4) {  // Handle wrapping forwards
        value = 0;
    }
    // Assign the value
    sequence_track_init_mode = value;

    if (sequence_track_init_mode == EMPTY_SEQUENCE) {
        status_display = F("Init: Empty");
    } else if (sequence_track_init_mode == SIMPLE_SEQUENCE) {
        status_display = F("Init: Simple");
    } else {
        status_display = F("Init: Random");
    }
    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void initSequence() {
    if (sequence_track_init_mode == EMPTY_SEQUENCE) {
        emptySequence();
        status_display = F("Empty Sequence");
    } else if (sequence_track_init_mode == SIMPLE_SEQUENCE) {
        simpleSequence();
        status_display = F("Simple Sequence");
    } else {
        randomSequence();
        status_display = F("Random Sequence");
    }

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void emptySequence() {
    sequences[0].length = 64;
    for (int i = 0; i < 64; i++){
        sequences_notes[0][i].note = 129;
        sequences_notes[0][i].velocity = 120;
        sequences_notes[0][i].note_length = QUARTER;
    }
}

void simpleSequence() {
    sequences[0].length = 64;
    for (int i = 0; i < 64; i++){
        sequences_notes[0][i].note = 36 + i;
        sequences_notes[0][i].velocity = 120;
        sequences_notes[0][i].note_length = QUARTER;
    }
}

void randomSequence() {
    sequences[0].length = 64;
    for (int i = 0; i < 64; i++){
        sequences_notes[0][i].note = random(129);
        sequences_notes[0][i].velocity = 120;
        sequences_notes[0][i].note_length = QUARTER;
    }
}

void adjustBPM(byte adjustment) {
    bpm += adjustment;
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    status_display = F("BPM: ");
    status_display += bpm;

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void adjustBeatDivision(byte adjustment) {
    sequences[sequence_id].beat_division += adjustment;

    // Handle looping round the values
    if (sequences[sequence_id].beat_division > 127) {
        sequences[sequence_id].beat_division = 5;
    }

    status_display = F("Division: ");
    // There are 6 possible beat divisions. Choose one.
    sequences[sequence_id].beat_division = sequences[sequence_id].beat_division % 6;
    if (sequences[sequence_id].beat_division == 0) {
        status_display += F("1/1");
    } else if (sequences[sequence_id].beat_division == 1) {
        status_display += F("1/2");
    } else if (sequences[sequence_id].beat_division == 2) {
        status_display += F("1/4");
    } else if (sequences[sequence_id].beat_division == 3) {
        status_display += F("1/8");
    } else if (sequences[sequence_id].beat_division == 4) {
        status_display += F("1/16");
    } else if (sequences[sequence_id].beat_division == 5) {
        status_display += F("1/32");
    }

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void adjustSequenceIndex(byte adjustment, bool loading) {
    sequences[sequence_id].id += adjustment;

    if (loading) {
        status_display = F("Load Seq: ");
    } else {
        status_display = F("Save Seq: ");
    }
    status_display += sequences[sequence_id].id + 1;  // Display off by one.

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void adjustSequenceChannel(byte adjustment) {
    sequences[sequence_id].channel += adjustment;

    if (sequences[sequence_id].channel > 200) { // Looping << backwards
        sequences[sequence_id].channel = 15;
    } else if (sequences[sequence_id].channel > 15) { // Looping >> forwards
        sequences[sequence_id].channel = 0;
    }

    status_display = F("Channel: ");
    status_display += sequences[sequence_id].channel + 1;  // Display off by one.

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}


void setNoteLengthFromBeatDivision() {
    for (byte i = 0; i < 64; i++) {
        sequences_notes[sequence_id][i].note_length = sequences[sequence_id].beat_division;
    }

    status_display = F("Set Gate Length");

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void adjustSequenceLength(byte adjustment) {
    sequences[sequence_id].length += adjustment;

    status_display = F("Steps: ");
    status_display += sequences[sequence_id].length + 1;  // Display off by one.

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void update_note(byte note, byte value) {
    if (sequence_edit_mode[note] == SEQUENCE_NOTE) {
        sequences_notes[sequence_id][note].note += value;
        if (sequences_notes[sequence_id][note].note > 200) { // Looping << backwards
            sequences_notes[sequence_id][note].note = 129;
        } else if (sequences_notes[sequence_id][note].note > 129) {  // Looping >> forwards. 127 Midi notes + 2 special cases
            sequences_notes[sequence_id][note].note = 0;
        }
    } else if (sequence_edit_mode[note] == SEQUENCE_VELOCITY) {
        sequences_notes[sequence_id][note].velocity += value;
        if (sequences_notes[sequence_id][note].velocity > 200) { // Looping << backwards
            sequences_notes[sequence_id][note].velocity = 127;
        } else if (sequences_notes[sequence_id][note].velocity > 127) { // Looping >> forwards
            sequences_notes[sequence_id][note].velocity = 0;
        }
    } else if (sequence_edit_mode[note] == SEQUENCE_NOTE_LENGTH) {
        value += int(sequences_notes[sequence_id][note].note_length);
        if (value > 200) { // Looping << backwards
            value = 5;
        } else if (value > 127) { // Looping >> forwards
            value = 0;
        }
        sequences_notes[sequence_id][note].note_length = value;
    }
    displaySequenceNoteStatus(note);
}


unsigned int getSequenceAddress(byte id) {
    return size_of_sequence * id;
}

void loadSequence(bool quiet) {
    if (!quiet) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Loading"));
        lcd.setCursor(0, 1);
        lcd.print(F("Sequence "));
        lcd.print(sequences[sequence_id].id + 1);
    }
    // TODO: Optimisation: Use sequential reads
    unsigned int address = getSequenceAddress(sequences[sequence_id].id);
    int offset = 0;

    // Sequence
    sequences[sequence_id].channel = myEEPROM.read(address + offset);
    offset += 1;
    sequences[sequence_id].length = myEEPROM.read(address + offset);
    offset += 1;
    sequences[sequence_id].beat_division = myEEPROM.read(address + offset);
    // We have removed note_length and note_length_from_sequence,
    // these bytes can be reused without having to lose the currenct
    // save structure
    offset += 3;

    // Sequence Notes
    for (byte i = 0; i < 64; i++) {
        sequences_notes[sequence_id][i].note = myEEPROM.read(address + offset);
        offset += 1;
        sequences_notes[sequence_id][i].velocity = myEEPROM.read(address + offset);
        offset += 1;
        sequences_notes[sequence_id][i].note_length = myEEPROM.read(address + offset);
        offset += 1;
    }
    ui_dirty = true;
}

void saveSequence() {
    // TODO: Optimisation: Read before write
    // TODO: Optimisation: Use sequential writes
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Saving"));
    lcd.setCursor(0, 1);
    lcd.print(F("Sequence "));
    lcd.print(sequences[sequence_id].id  + 1);
    unsigned int address = getSequenceAddress(sequences[sequence_id].id);
    int offset = 0;

    // Sequence
    myEEPROM.write(address + offset, sequences[sequence_id].channel);
    offset += 1;
    myEEPROM.write(address + offset, sequences[sequence_id].length);
    offset += 1;
    myEEPROM.write(address + offset, sequences[sequence_id].beat_division);
    // We have removed note_length and note_length_from_sequence,
    // these bytes can be reused without having to lose the currenct
    // save structure
    offset += 3;

    // Sequence Notes
    for (byte i = 0; i < 64; i++) {
        myEEPROM.write(address + offset, sequences_notes[sequence_id][i].note);
        offset += 1;
        myEEPROM.write(address + offset, sequences_notes[sequence_id][i].velocity);
        offset += 1;
        myEEPROM.write(address + offset, sequences_notes[sequence_id][i].note_length);
        offset += 1;
    }

    ui_dirty = true;
}

void displaySequenceNoteStatus(byte note) {
    // Update status display
    // Note
    note_name(status_display, sequences_notes[sequence_id][note].note);

    // Velocity
    status_display += F(" V");
    if (sequences_notes[sequence_id][note].velocity < 10) {
        status_display += F("  ");
    } else if (sequences_notes[sequence_id][note].velocity < 100) {
        status_display += F(" ");
    }
    status_display += sequences_notes[sequence_id][note].velocity;

    // Note Length
    status_display += F(" L");
    // There are 6 possible beat divisions. Choose one.
    sequences_notes[sequence_id][note].note_length = sequences_notes[sequence_id][note].note_length % 6;
    if (sequences_notes[sequence_id][note].note_length == 0) {
        status_display += F("1/1");
    } else if (sequences_notes[sequence_id][note].note_length == 1) {
        status_display += F("1/2");
    } else if (sequences_notes[sequence_id][note].note_length == 2) {
        status_display += F("1/4");
    } else if (sequences_notes[sequence_id][note].note_length == 3) {
        status_display += F("1/8");
    } else if (sequences_notes[sequence_id][note].note_length == 4) {
        status_display += F("1/16");
    } else if (sequences_notes[sequence_id][note].note_length == 5) {
        status_display += F("1/32");
    }

    // Put a cursor under the current mode
    if (sequence_edit_mode[note] == SEQUENCE_NOTE) {
        cursor_x = 0;
        cursor_y = 1;
    } else if (sequence_edit_mode[note] == SEQUENCE_VELOCITY) {
        cursor_x = 5;
        cursor_y = 1;
    } else if (sequence_edit_mode[note] == SEQUENCE_NOTE_LENGTH) {
        cursor_x = 10;
        cursor_y = 1;
    }
    cursor_display = true;

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}
