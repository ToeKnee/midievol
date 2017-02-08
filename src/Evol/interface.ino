void timerIsr() {
    encoder_0->service();
    encoder_1->service();
    encoder_2->service();
    encoder_3->service();

    encoder_4->service();
    encoder_5->service();
    encoder_6->service();
    encoder_7->service();

    encoder_8->service();
    encoder_9->service();
    encoder_10->service();
    encoder_11->service();
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
                } else if (encoder == 3) {  // Handle Sequence Length Changes
                    adjustSequenceLength(value);
                } else if (encoder == 4) {  // Handle Load position
                    adjustSequenceIndex(value);
                } else if (encoder == 5) {  // Handle Save position
                    adjustSequenceIndex(value);
                }
            }
        } else if (mode == drum) { // Drum mode
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
                }
            }
        }
    }
}


void handleEncoderButton(byte encoder, ClickEncoder::Button button) {
    if (button != ClickEncoder::Open) {
        Serial.print(encoder);
        Serial.print("  " );
        Serial.println(button);
        if (mode == drum) {
            if (button == ClickEncoder::Clicked) {
                // Cycle the mode
                if (drum_track_edit_mode[encoder] == BEATS) {
                    drum_track_edit_mode[encoder] = ROTATION;
                } else if (drum_track_edit_mode[encoder] == ROTATION) {
                    drum_track_edit_mode[encoder] = NOTE;
                } else if (drum_track_edit_mode[encoder] == NOTE) {
                    drum_track_edit_mode[encoder] = BEATS;
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
                if (mode == sequencer) {
                    mode = drum;
                    sequence_id = 0;
                } else if (mode == drum) {
                    mode = song;
                } else {
                    mode = sequencer;
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
