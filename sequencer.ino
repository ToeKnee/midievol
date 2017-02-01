void adjustBPM(byte adjustment) {
    bpm += adjustment;
    microseconds_per_pulse = pulse_len_from_bpm(bpm);

    status_display = "BPM: ";
    status_display += bpm;

    ui_dirty = true;
}

void adjustBeatDivision(byte adjustment) {
    sequences[sequence_id].beat_division += adjustment;

    // Handle looping round the values
    if (sequences[sequence_id].beat_division > 127) {
        sequences[sequence_id].beat_division = 5;
    }

    status_display = "Beat: ";
    // There are 6 possible beat divisions. Choose one.
    sequences[sequence_id].beat_division = sequences[sequence_id].beat_division % 6;
    if (sequences[sequence_id].beat_division == 0) {
        status_display += "1/1";
    } else if (sequences[sequence_id].beat_division == 1) {
        status_display += "1/2";
    } else if (sequences[sequence_id].beat_division == 2) {
        status_display += "1/4";
    } else if (sequences[sequence_id].beat_division == 3) {
        status_display += "1/8";
    } else if (sequences[sequence_id].beat_division == 4) {
        status_display += "1/16";
    } else if (sequences[sequence_id].beat_division == 5) {
        status_display += "1/32";
    }

    ui_dirty = true;
}

void adjustNoteLength(byte adjustment) {
    sequences[sequence_id].note_length += adjustment;

    // Handle looping round the values
    if (sequences[sequence_id].note_length > 127) {
        sequences[sequence_id].note_length = 5;
    }

    status_display = "Gate: ";
    // There are 6 possible beat divisions. Choose one.
    sequences[sequence_id].note_length = sequences[sequence_id].note_length % 6;
    if (sequences[sequence_id].note_length == 0) {
        status_display += "1/1";
    } else if (sequences[sequence_id].note_length == 1) {
        status_display += "1/2";
    } else if (sequences[sequence_id].note_length == 2) {
        status_display += "1/4";
    } else if (sequences[sequence_id].note_length == 3) {
        status_display += "1/8";
    } else if (sequences[sequence_id].note_length == 4) {
        status_display += "1/16";
    } else if (sequences[sequence_id].note_length == 5) {
        status_display += "1/32";
    }

    ui_dirty = true;
}

void adjustSequenceIndex(byte adjustment) {
    sequences[sequence_id].id += adjustment;

    status_display = "";
    status_display += sequences[sequence_id].id + 1;  // Display off by one.

    // TODO: Remove this when save/load is implemented - need big eeprom
    sequence_id = sequences[sequence_id].id;

    ui_dirty = true;
}

void adjustSequenceLength(byte adjustment) {
    sequences[sequence_id].length += adjustment;

    status_display = "Steps: ";
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
    status_display += " ";
    status_display += sequences_notes[sequence_id][display_note].velocity;

    ui_dirty = true;
}



int getSequenceAddress(byte id) {
    return size_of_sequence * id;
}

void loadSequence() {

}

void saveSequence() {

}
