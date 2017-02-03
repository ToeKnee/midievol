
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

DrumTrack drum_tracks[16];
DrumSequence drum_sequences[16] {
    {
        0,
        10,
        sixteenth,
    }
};

int step_status = 0;
bool drum_track[64];
byte remainder[64];
byte count[64];
byte calculated_drum_tracks[16][64];
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
    for (byte x=0; x < rotation; x++) {
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

// Bjorklund accessory function to build the output..
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


void randomDrumPattern() {
    // 4-16 random tracks
    byte tracks = random(4, 16);
    for (int x = 0; x < tracks; x++){
        int steps = random(64);
        euclidean_build(x, random(steps / 3), steps, random(steps), 35 + x);
    }
    // Empty out remaining tracks
    for (int i=0; i < 16 - tracks; i++) {
        euclidean_build(tracks + i, 0, 1, random(3), 35 + i);
    }
}
