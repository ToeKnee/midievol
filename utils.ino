
int bpm_from_pulse_len(unsigned long pulse_len) {
    return (oneMinuteInMicroseconds / (pulse_len * CPQN)) * (timeSignatureDenominator / 4.0);
}

unsigned long pulse_len_from_bpm(int bpm) {
    return oneMinuteInMicroseconds / (bpm * CPQN);
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
