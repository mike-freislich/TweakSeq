struct Note {
  byte stepNumber = 0;
  byte octave = 1;
  uint16_t pitch = 1;
  uint16_t voltage = 0;
  byte midiNote = 0;
  bool isRest = false, isTie = false;
};