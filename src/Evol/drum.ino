
struct DrumTrack {
    byte beats; // less than or equal to length
    byte length; // 64 limit
    byte rotation;
    byte note; // 0 to 127
};

struct DrumSequence {
    byte id; // Save location - 0 indexed
    byte channel; // 16 channels
    byte beat_division; // BeatDivision
};

DrumTrack drum_tracks[8];
DrumSequence drum_sequence;
const int size_of_drum_sequence = sizeof(drum_sequence) + sizeof(drum_tracks);

int step_status = 0;
bool drum_track[64];
byte remainder[64];
byte count[64];
byte calculated_drum_tracks[8][64];

typedef enum DrumTrackEditMode {
    DRUM_TRACK_BEATS,
    DRUM_TRACK_LENGTH,
    DRUM_TRACK_ROTATION,
    DRUM_TRACK_NOTE
};
DrumTrackEditMode drum_track_edit_mode[8];

typedef enum DrumTrackInitMode {
    EMPTY_DRUM_PATTERN,
    SIMPLE_DRUM_PATTERN,
    FUNKY_DRUM_PATTERN,
    SYNCOPATED_DRUM_PATTERN,
    RANDOM_DRUM_PATTERN,
};
DrumTrackInitMode drum_track_init_mode = EMPTY_DRUM_PATTERN;


void euclidean_build(byte track, byte beats, byte length, byte rotation, byte note)  {
    // Set the track details
    drum_tracks[track].beats = beats;
    drum_tracks[track].length = length;
    drum_tracks[track].rotation = rotation;
    drum_tracks[track].note = note;

    if (length > 64) {
        length = 64;
    } else if (length < 1) {
        length = 1;
    }

    if (beats > length) {
        beats = length;
    }

    int divisor = length - beats;
    sequences[1].length = length - 1;
    remainder[0] = beats;
    step_status = 0;
    int level = 0;

    do {
        count[level] = divisor / remainder[level];
        remainder[level + 1] = divisor % remainder[level];
        divisor = remainder[level];
        level = level + 1;
    } while (remainder[level] > 1);

    count[level] = divisor;

    build_string(track, level);

    // Rotate round, so always starts a beat on the first step
    if (beats > 0) {
        while (calculated_drum_tracks[track][0] == false) {
            for (byte i=0; i < length - 1; i++) {
                calculated_drum_tracks[track][i] = calculated_drum_tracks[track][i + 1];
            }
            calculated_drum_tracks[track][length -1] = false;
        }
    }

    // Handle rotation
    for (byte x=0; x < (length - rotation); x++) {
        bool first_to_last = calculated_drum_tracks[track][0];
        for (byte i=0; i < length - 1; i++) {
            calculated_drum_tracks[track][i] = calculated_drum_tracks[track][i + 1];
        }
        calculated_drum_tracks[track][length -1] = first_to_last;
    }

    // DEBUG
    /* for (byte i=0; i < length; i++) { */
    /*     Serial.print(calculated_drum_tracks[track][i]); */
    /* } */
    /* Serial.println(""); */
}

// Bjorklund accessory function to build the output.
void build_string (byte track, int level)  {
    if (level == -1) {
        calculated_drum_tracks[track][step_status] = false; //insert 0 into array
        step_status += 1;      //move to next
    }
    else if (level == -2)  {
        calculated_drum_tracks[track][step_status] = true; //insert 1 into array
        step_status += 1;      //move to next
    }
    else {
        for (int i = 0; i < count[level]; i++) {
            build_string(track, level - 1);
        }
        if (remainder[level] != 0) {
            build_string(track, level - 2);
        }
    }
}


void init_drums() {
    drum_sequence.channel = 10;
    drum_sequence.beat_division = SIXTEENTH;
    for (int x = 0; x < 8; x++){
        euclidean_build(x, 0, 16, 0, 35 + x);
        drum_track_edit_mode[x] = DRUM_TRACK_BEATS;
    }
    loadDrumSequence(true);
}


void initDrumPatternChoice(byte value) {
    // Update the value with the current state
    value += int(drum_track_init_mode);
    if (value > 127) {  // Handle wrapping backwards
        value = 4;
    } else if (drum_track_init_mode > 4) {  // Handle wrapping forwards
        value = 0;
    }
    // Assign the value
    drum_track_init_mode = value;

    if (drum_track_init_mode == EMPTY_DRUM_PATTERN) {
        status_display = F("Init: Empty");
    } else if (drum_track_init_mode == SIMPLE_DRUM_PATTERN) {
        status_display = F("Init: Simple");
    } else if (drum_track_init_mode == FUNKY_DRUM_PATTERN) {
        status_display = F("Init: Funky");
    } else if (drum_track_init_mode == SYNCOPATED_DRUM_PATTERN) {
        status_display = F("Init: Syncopated");
    } else {
        status_display = F("Init: Random");
    }
    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void initDrumPattern() {
    if (drum_track_init_mode == EMPTY_DRUM_PATTERN) {
        emptyDrumPattern();
        status_display = F("Empty Pattern");
    } else if (drum_track_init_mode == SIMPLE_DRUM_PATTERN) {
        simpleDrumPattern();
        status_display = F("Simple Pattern");
    } else if (drum_track_init_mode == FUNKY_DRUM_PATTERN) {
        funkyDrumPattern();
        status_display = F("Funky Pattern");
    } else if (drum_track_init_mode == SYNCOPATED_DRUM_PATTERN) {
        syncopatedDrumPattern();
        status_display = F("Syncopated Pattern");
    } else {
        randomDrumPattern();
        status_display = F("Random Pattern");
    }

    status_timeout = micros() + timeOut;
    ui_dirty = true;

}

void emptyDrumPattern() {
    for (int i=0; i < 8; i++) {
        euclidean_build(i, 0, 16, 0, 35 + i);
    }
}

void simpleDrumPattern() {
    euclidean_build(0, 4, 16, 0, 35);
    euclidean_build(1, 4, 16, 2, 38);
    euclidean_build(2, 1, 1, 0, 46);
    euclidean_build(3, 1, 4, 2, 42);
    euclidean_build(4, 1, 64, 0, 49);

    for (int i=5; i < 8; i++) {
        euclidean_build(i, 0, 16, 0, 35 + i);
    }
}

void funkyDrumPattern() {
    euclidean_build(0, 4, 16, 0, 35);
    euclidean_build(1, 5, 16, 2, 38);
    euclidean_build(2, 4, 16, 3, 42);
    euclidean_build(3, 12, 16, 0, 46);
    euclidean_build(5, 2, 3, 1, 53);
    euclidean_build(6, 5, 13, 0, 75);

    for (int i=7; i < 8; i++) {
        euclidean_build(i, 0, 16, 0, 35 + i);
    }
}

void syncopatedDrumPattern() {
    euclidean_build(2, 2, 3, 0, 35);
    euclidean_build(1, 2, 5, 2, 38);
    euclidean_build(2, 24, 35, 0, 46);
    euclidean_build(3, 1, 6, 3, 43);
    euclidean_build(4, 4, 11, 0, 60);

    for (int i=5; i < 8; i++) {
        euclidean_build(i, 0, 16, 0, 35 + i);
    }
}

void randomDrumPattern() {
    // 4-8 random tracks
    byte tracks = random(4, 8);
    for (int x = 0; x < tracks; x++){
        int steps = random(64);
        euclidean_build(x, random(steps / 2), steps, random(steps), 35 + x);
    }
    // Empty out remaining tracks
    for (int i=0; i < 8 - tracks; i++) {
        euclidean_build(tracks + i, 0, 1, 0, 35 + i);
    }
}


void update_drumtrack(byte encoder, byte value) {
    if (drum_track_edit_mode[encoder] == DRUM_TRACK_BEATS) {
        drum_tracks[encoder].beats += value;
        if (drum_tracks[encoder].beats > 127) { // Beats Looping << backwards
            drum_tracks[encoder].beats = drum_tracks[encoder].length;
        } else if (drum_tracks[encoder].beats > drum_tracks[encoder].length) { // Beats Looping >> forwards
            drum_tracks[encoder].beats = 0;
        }
    } else if (drum_track_edit_mode[encoder] == DRUM_TRACK_LENGTH) {
        drum_tracks[encoder].length += value;
        if (drum_tracks[encoder].length > 127) { // Length Looping << backwards
            drum_tracks[encoder].length = 64;
        } else if (drum_tracks[encoder].length > 64) { // Length Looping << forwards
            drum_tracks[encoder].length = 0;
        }

        // Ensure that beats are never longer than length
        if (drum_tracks[encoder].beats > drum_tracks[encoder].length) {
            drum_tracks[encoder].beats = drum_tracks[encoder].length;
        }
    } else if (drum_track_edit_mode[encoder] == DRUM_TRACK_ROTATION) {
        drum_tracks[encoder].rotation += value;
        if (drum_tracks[encoder].rotation > 127) { // Looping << backwards
            drum_tracks[encoder].rotation = drum_tracks[encoder].length;
        } else if (drum_tracks[encoder].rotation > drum_tracks[encoder].length) { // Looping >> forwards
            drum_tracks[encoder].rotation = 0;
        }
    } else if (drum_track_edit_mode[encoder] == DRUM_TRACK_NOTE) {
        drum_tracks[encoder].note += value;
        if (drum_tracks[encoder].note > 200) { // Looping << backwards
            drum_tracks[encoder].note = 127;
        } else if (drum_tracks[encoder].note > 127) { // Looping >> forwards
            drum_tracks[encoder].note = 0;
        }
    }
    // Build the drum pattern
    euclidean_build(
                    encoder,
                    drum_tracks[encoder].beats,
                    drum_tracks[encoder].length,
                    drum_tracks[encoder].rotation,
                    drum_tracks[encoder].note
                    );

    displayDrumTrackStatus(encoder);
}


void adjustDrumSequenceIndex(byte adjustment, bool loading) {
    drum_sequence.id += adjustment;

    if (loading) {
        status_display = F("Load Seq: ");
    } else {
        status_display = F("Save Seq: ");
    }
    status_display += drum_sequence.id + 1;  // Display off by one.

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}

void adjustDrumChannel(byte adjustment) {
    drum_sequence.channel += adjustment;

    if (drum_sequence.channel > 200) { // Looping << backwards
        drum_sequence.channel = 15;
    } else if (drum_sequence.channel > 15) { // Looping >> forwards
        drum_sequence.channel = 0;
    }

    status_display = F("Channel: ");
    status_display += drum_sequence.channel + 1;  // Display off by one.

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}


unsigned int getDrumSequenceAddress(byte id) {
    return end_of_sequences + (size_of_drum_sequence * id);
}

void loadDrumSequence(bool quiet) {
    if (!quiet) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Loading"));
        lcd.setCursor(0, 1);
        lcd.print(F("Sequence "));
        lcd.print(drum_sequence.id + 1);
    }
    // TODO: Optimisation: Use sequential reads
    unsigned int address = getDrumSequenceAddress(drum_sequence.id);
    int offset = 0;

    // Sequence
    drum_sequence.channel = myEEPROM.read(address + offset);
    offset += 1;
    drum_sequence.beat_division = myEEPROM.read(address + offset);
    offset += 1;

    // Track
    for (byte i = 0; i < 8; i++) {
        drum_tracks[i].beats = myEEPROM.read(address + offset);
        offset += 1;
        drum_tracks[i].length = myEEPROM.read(address + offset);
        offset += 1;
        drum_tracks[i].rotation = myEEPROM.read(address + offset);
        offset += 1;
        drum_tracks[i].note = myEEPROM.read(address + offset);
        offset += 1;

        // And build the tracks
        euclidean_build(
                        i,
                        drum_tracks[i].beats,
                        drum_tracks[i].length,
                        drum_tracks[i].rotation,
                        drum_tracks[i].note
                        );
    }
    ui_dirty = true;
}

void saveDrumSequence() {
    // TODO: Optimisation: Read before write
    // TODO: Optimisation: Use sequential writes
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Saving"));
    lcd.setCursor(0, 1);
    lcd.print(F("Sequence "));
    lcd.print(drum_sequence.id  + 1);
    unsigned int address = getDrumSequenceAddress(drum_sequence.id);
    int offset = 0;

    // Sequence
    myEEPROM.write(address + offset, drum_sequence.channel);
    offset += 1;
    myEEPROM.write(address + offset, drum_sequence.beat_division);
    offset += 1;

    // Sequence Notes
    for (byte i = 0; i < 8; i++) {
        myEEPROM.write(address + offset, drum_tracks[i].beats);
        offset += 1;
        myEEPROM.write(address + offset, drum_tracks[i].length);
        offset += 1;
        myEEPROM.write(address + offset, drum_tracks[i].rotation);
        offset += 1;
        myEEPROM.write(address + offset, drum_tracks[i].note);
        offset += 1;
    }

    ui_dirty = true;
}

void displayDrumTrackStatus(byte track) {
    // Update status display
    // Beats
    status_display = F("");
    if (drum_tracks[track].beats < 10) {
        status_display += F(" ");
    }
    status_display += drum_tracks[track].beats;
    status_display += F("/");
    // Length
    if (drum_tracks[track].length < 10) {
        status_display += F(" ");
    }
    status_display += drum_tracks[track].length;

    status_display += F(" R");
    // Rotation
    if (drum_tracks[track].rotation < 10) {
        status_display += F("  ");
    } else if (drum_tracks[track].rotation < 100) {
        status_display += F(" ");
    }
    status_display += drum_tracks[track].rotation;

    status_display += F(" N");
    // Note
    if (drum_tracks[track].note < 10) {
        status_display += F("  ");
    } else if (drum_tracks[track].note < 100) {
        status_display += F(" ");
    }
    status_display += drum_tracks[track].note;

    // Put a cursor under the current mode
    if (drum_track_edit_mode[track] == DRUM_TRACK_BEATS) {
        cursor_x = 1;
        cursor_y = 1;
    } else if (drum_track_edit_mode[track] == DRUM_TRACK_LENGTH) {
        cursor_x = 4;
        cursor_y = 1;
    } else if (drum_track_edit_mode[track] == DRUM_TRACK_ROTATION) {
        cursor_x = 6;
        cursor_y = 1;
    } else if (drum_track_edit_mode[track] == DRUM_TRACK_NOTE) {
        cursor_x = 11;
        cursor_y = 1;
    }
    cursor_display = true;

    status_timeout = micros() + timeOut;
    ui_dirty = true;
}
