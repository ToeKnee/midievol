 void init_sequencer() {
    for (int i = 0; i < 16; i++){
        sequences[i].id = i;
        sequences[i].channel = i + 1;
        sequences[i].length = 11;
        sequences[i].beat_division = QUARTER;
        sequences[i].note_length = QUARTER;
        sequences[i].note_length_from_sequence = true;

        for (int j = 0; j < 64; j++){
            sequences_notes[i][j].note = random(129);
            sequences_notes[i][j].velocity = random(100, 127);
            sequences_notes[i][j].note_length = QUARTER;
        }
    }

    // Load sequence 1
    loadSequence(true);
}

void adjustBPM(byte adjustment) {
    bpm += adjustment;
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    status_display = F("BPM: ");
    status_display += bpm;

    ui_dirty = true;
}

void adjustBeatDivision(byte adjustment) {
    sequences[sequence_id].beat_division += adjustment;

    // Handle looping round the values
    if (sequences[sequence_id].beat_division > 127) {
        sequences[sequence_id].beat_division = 5;
    }

    status_display = F("Beat: ");
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

    ui_dirty = true;
}

void adjustNoteLength(byte adjustment) {
    sequences[sequence_id].note_length += adjustment;

    // Handle looping round the values
    if (sequences[sequence_id].note_length > 127) {
        sequences[sequence_id].note_length = 5;
    }

    status_display = F("Gate: ");
    // There are 6 possible beat divisions. Choose one.
    sequences[sequence_id].note_length = sequences[sequence_id].note_length % 6;
    if (sequences[sequence_id].note_length == 0) {
        status_display += F("1/1");
    } else if (sequences[sequence_id].note_length == 1) {
        status_display += F("1/2");
    } else if (sequences[sequence_id].note_length == 2) {
        status_display += F("1/4");
    } else if (sequences[sequence_id].note_length == 3) {
        status_display += F("1/8");
    } else if (sequences[sequence_id].note_length == 4) {
        status_display += F("1/16");
    } else if (sequences[sequence_id].note_length == 5) {
        status_display += F("1/32");
    }

    ui_dirty = true;
}

void adjustSequenceIndex(byte adjustment) {
    sequences[sequence_id].id += adjustment;

    status_display = F("");
    status_display += sequences[sequence_id].id + 1;  // Display off by one.

    ui_dirty = true;
}

void adjustSequenceLength(byte adjustment) {
    sequences[sequence_id].length += adjustment;

    status_display = F("Steps: ");
    status_display += sequences[sequence_id].length + 1;  // Display off by one.

    ui_dirty = true;
}


void update_note(byte note, byte value) {
    sequences_notes[sequence_id][note].note += value;
    if (sequences_notes[sequence_id][note].note == 255) { // Looped round 0 backwards
        sequences_notes[sequence_id][note].note = 129;
    } else if (sequences_notes[sequence_id][note].note > 129) {  // 127 Midi notes + 2 special cases
        sequences_notes[sequence_id][note].note = 0;
    }

    display_note = note;
    note_name(status_display, sequences_notes[sequence_id][display_note].note);
    status_display += F(" ");
    status_display += sequences_notes[sequence_id][display_note].velocity;

    ui_dirty = true;
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
    offset += 1;
    sequences[sequence_id].note_length = myEEPROM.read(address + offset);
    offset += 1;
    sequences[sequence_id].note_length_from_sequence = myEEPROM.read(address + offset);
    offset += 1;

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
    offset += 1;
    myEEPROM.write(address + offset, sequences[sequence_id].note_length);
    offset += 1;
    myEEPROM.write(address + offset, sequences[sequence_id].note_length_from_sequence);
    offset += 1;

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
