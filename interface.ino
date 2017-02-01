void timerIsr() {
    encoder_0->service();
    encoder_1->service();
    encoder_2->service();
    encoder_3->service();

    encoder_4->service();
    encoder_5->service();
    encoder_6->service();
    encoder_7->service();
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
                }
                else if (encoder == 4) {  // Handle Load position
                    adjustSequenceIndex(value);
                }
                else if (encoder == 5) {  // Handle Save position
                    adjustSequenceIndex(value);
                }
            }
        }
    }
}
