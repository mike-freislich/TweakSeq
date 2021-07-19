#ifndef MY_UISTATE
#define MY_UISTATE

enum UIState
{
    SEQUENCER,
    LOADING_BANK_SELECT,
    LOADING_PATTERN_SELECT,
    LOADING_COMPLETE,
    SAVING_BANK_SELECT,
    SAVING_PATTERN_SELECT,
    SAVING_COMPLETE
};
UIState uiState = UIState::SEQUENCER;
UIState uiLastState = uiState;

bool stateChanged() {
    bool changed = uiLastState != uiState;
    uiLastState = uiState;
    return changed;
}

#endif