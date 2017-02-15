void handleEncoder(byte encoder, byte value) {
    if (value != 0) {
        // Sequencer mode
        if (mode == SEQUENCER) {
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
                } else if (encoder == 3) {  // Handle Sequence Length Changes
                    adjustSequenceLength(value);
                } else if (encoder == 12) {  // Handle Load position
                    adjustSequenceIndex(value, true);
                } else if (encoder == 15) {  // Handle Save position
                    adjustSequenceIndex(value, false);
                }
            }
        } else if (mode == DRUM) { // Drum mode
            if (!shift) {
                /* For the sixteen encoders, handle setting length,
                   beats, rotation, velocity humanisation and note.
                */
                update_drumtrack(encoder, value);
            } else {
                if (encoder == 0) {  // Handle BPM changes
                    adjustBPM(value);
                } else if (encoder == 1) {  // Handle Beat Division Changes
                    adjustBeatDivision(value);  // TODO: Drums should have own beat division
                } else if (encoder == 2) {  // Handle Note Length Changes
                    randomDrumPattern();
                } else if (encoder == 12) {  // Handle Load position
                    adjustDrumSequenceIndex(value, true);
                } else if (encoder == 15) {  // Handle Save position
                    adjustDrumSequenceIndex(value, false);
                }
            }
        }
    }
}


void handleEncoderButton(byte encoder, ClickEncoder::Button button) {
    if (button != ClickEncoder::Open) {
        if (mode == SEQUENCER) {
            cursor_display = false;
            if (button == ClickEncoder::Clicked) {
                if (shift) {
                    if (encoder == 12) {
                        loadSequence(false);
                    } else if (encoder == 15) {
                        saveSequence();
                    }
                }
            }
        } else if (mode == DRUM) {
            cursor_display = false;
            if (button == ClickEncoder::Clicked) {
                if (shift) {
                    if (encoder == 12) {
                        loadDrumSequence(false);
                    } else if (encoder == 15) {
                        saveDrumSequence();
                    }
                } else {
                    // Cycle the mode
                    if (drum_track_edit_mode[encoder] == BEATS) {
                        drum_track_edit_mode[encoder] = LENGTH;
                        cursor_x = 4;
                        cursor_y = 1;
                    } else if (drum_track_edit_mode[encoder] == LENGTH) {
                        drum_track_edit_mode[encoder] = ROTATION;
                        cursor_x = 6;
                        cursor_y = 1;
                    } else if (drum_track_edit_mode[encoder] == ROTATION) {
                        drum_track_edit_mode[encoder] = NOTE;
                        cursor_x = 11;
                        cursor_y = 1;
                    } else if (drum_track_edit_mode[encoder] == NOTE) {
                        drum_track_edit_mode[encoder] = BEATS;
                        cursor_x = 1;
                        cursor_y = 1;
                    }
                    cursor_display = true;
                    ui_dirty = true;
                }
            }
        }
    }
}

void handleButtons() {
    if (debouncer_play.update()) {
        if (debouncer_play.fell()) {
            // Shift - change mode
            if (shift) {
                if (mode == SEQUENCER) {
                    mode = DRUM;
                    sequence_id = 0;
                } else if (mode == DRUM) {
                    mode = SONG;
                } else {
                    mode = SEQUENCER;
                    sequence_id = 0;
                }
            } else {
                if (playing) {
                    handlePause();
                } else {
                    handleStart();
                }
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
}
