void draw_ui() {
    ui_dirty = false;
    // Write the mode
    lcd.setCursor(0, 0);
    if (mode == SEQUENCER) {
        lcd.print(F("SEQ "));
    } else if (mode == DRUM) {
        lcd.print(F("DRM "));
    }
    if (sequence_id < 10) {
        lcd.print(F("  "));
    } else if (sequence_id < 100) {
        lcd.print(F(" "));
    }
    lcd.print(sequence_id + 1);


    // Write Play State
    lcd.setCursor(10, 0);
    if (playing) {
        lcd.write(byte(2));  // Play button
    } else {
        lcd.print(F(" "));
    }

    // Display Shift status
    lcd.setCursor(11, 0);
    if (shift) {
        lcd.write(byte(3));
    } else {
        lcd.print(F(" "));
    }

    // Write the bpm at the top right of the display
    lcd.setCursor(12, 0);
    lcd.write(byte(beat_chr));
    lcd.setCursor(13, 0);
    if (bpm < 10) {
        lcd.print(F("  "));
    } else if (bpm < 100) {
        lcd.print(F(" "));
    }
    lcd.print(bpm);

    if (mode == SEQUENCER) {
        // Display Status
        lcd.setCursor(0, 1);
        while (status_display.length() <= 10) {
            status_display += " ";
        }
        lcd.print(status_display);

        // Display last note edited
        lcd.setCursor(12, 1);
        String note_display;
        note_name(note_display, sequences_notes[sequence_id][display_note].note);
        lcd.print(note_display);
    } else if (mode == DRUM) {
        // Display Status
        if (status_timeout > micros()) {
            lcd.setCursor(0, 1);
            while (status_display.length() <= 16) {
                status_display += " ";
            }
            lcd.print(status_display);
        }
    }

    if (cursor_display) {
        lcd.setCursor(cursor_x, cursor_y);
        lcd.cursor();
    }
}
