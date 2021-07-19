#ifndef MY_UISTATE
#define MY_UISTATE

enum UIState
{
    SEQUENCER,
    SA_BANK_SELECT,
    SA_PATTERN_SELECT,
    SA_ACTION_COMPLETE,
};
UIState uiState = UIState::SEQUENCER;
UIState uiLastState = uiState;

bool uiStateChanged() {
    bool changed = uiLastState != uiState;
    uiLastState = uiState;
    return changed;
}

#endif