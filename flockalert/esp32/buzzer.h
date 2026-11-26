#ifndef BUZZER_H
#define BUZZER_H

// ============================================================
// BUZZER CHIMES
// ============================================================
//
// Plays a short chime on the speaker GPIO when an ALPR alert starts.
// Use a PASSIVE piezo buzzer: it can produce different pitches.
// An ACTIVE buzzer only makes one fixed tone, so the chime will
// sound the same no matter which tone is selected.
//
// Playback is non-blocking. updateBuzzer() must be called from loop().

constexpr int CHIME_MAX_NOTES = 8;

struct ChimeNote {
  uint16_t frequencyHz;   // 0 = rest (silence)
  uint16_t durationMs;
};

struct ChimeSequence {
  uint8_t noteCount;
  ChimeNote notes[CHIME_MAX_NOTES];
};

// Order must match CHIME_TONE_NAMES in config.h.
const ChimeSequence chimeSequences[CHIME_TONE_COUNT] = {

  // 0: Two-tone (ding-dong)
  { 2, { { 1319, 180 }, { 988, 320 } } },

  // 1: Rising arpeggio
  { 4, { { 523, 110 }, { 659, 110 }, { 784, 110 }, { 1047, 260 } } },

  // 2: Warble (alternating pitch)
  { 6, { { 880, 120 }, { 620, 120 }, { 880, 120 },
         { 620, 120 }, { 880, 120 }, { 620, 120 } } },

  // 3: Quick double beep
  { 3, { { 2400, 120 }, { 0, 100 }, { 2400, 120 } } }
};


// ============================================================
// PLAYER STATE
// ============================================================

bool chimePlaying = false;

uint8_t chimeNoteIndex = 0;

unsigned long chimeNoteStartMs = 0;


// ============================================================
// PLAYER
// ============================================================

static void playChimeNote() {

  const ChimeSequence& seq =
    chimeSequences[runtimeSettings.chimeTone];

  const ChimeNote& note =
    seq.notes[chimeNoteIndex];

  if (note.frequencyHz > 0) {
    tone(runtimeSettings.speakerPin, note.frequencyHz);
  } else {
    noTone(runtimeSettings.speakerPin);
  }

  chimeNoteStartMs = millis();
}


// Called when a new ALPR alert starts. Does nothing when the
// buzzer is disabled in configuration.
void startChime() {

  if (!runtimeSettings.buzzerEnabled) {
    return;
  }

  chimeNoteIndex = 0;

  chimePlaying = true;

  playChimeNote();
}


// Advances the chime. Call every loop().
void updateBuzzer() {

  if (!chimePlaying) {
    return;
  }

  const ChimeSequence& seq =
    chimeSequences[runtimeSettings.chimeTone];

  const ChimeNote& note =
    seq.notes[chimeNoteIndex];

  // Unsigned subtraction stays correct when millis() wraps.
  if (millis() - chimeNoteStartMs < note.durationMs) {
    return;
  }

  chimeNoteIndex++;

  if (chimeNoteIndex >= seq.noteCount) {

    noTone(runtimeSettings.speakerPin);

    chimePlaying = false;

    return;
  }

  playChimeNote();
}

#endif
