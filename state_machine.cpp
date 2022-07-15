// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module implements state machine

#include "warning.h"
#include "state_machine.h"

// The state function array holds a function pointer to the function which gets called for each state. 
// In addition, a printable state name is stored for debugging purposes. 
// Each row in the state function array is defined by a struct:
typedef struct 
{
    const char* name;
    void (*func)(FILE* fptr);
} STATE_FUNCTION_ROW_STRUCT;

// Maps a state to its state transition function, which should be called when the state transitions into this state.
// The order of the members must be in sync with the STATE_ENUM member declaration order.
// This array has to stay in sync with the STATE_ENUM enumeration. That is, there must be the same number 
// of rows in state_function as there are states in STATE_ENUM, and they must be in the same order.

static STATE_FUNCTION_ROW_STRUCT state_function[] = 
{
    // State name           // State function
    { "STATE_WARNING_ON",   &Warning_On },
    { "STATE_WARNING_OFF",  &Warning_Off }
};

// This following code defines a row in the state transition matrix (the state transition matrix is just an array of this structure). 
// This structure contains the current state, an event, and the state to transition to.
typedef struct 
{
    STATE_ENUM curr_state;
    EVENT_ENUM event;
    STATE_ENUM next_state;
} STATE_TRANSITION_MATRIX_ROW_STRUCT;

// The state transition matrix is the heart of this state machine methodology. 
// It specifies what the next state should be, given the current state and the event that just occurred. 
// It is built as an array of the curr_state/event/next_state structure defined above.
static STATE_TRANSITION_MATRIX_ROW_STRUCT state_transition_matrix[] = 
{
    // Current state       // Event                 // Next state
    // Current state       // Event                 // Next state
    { STATE_WARNING_OFF,    EVENT_WARNING_ON,       STATE_WARNING_ON  },
    { STATE_WARNING_ON,     EVENT_WARNING_OFF,      STATE_WARNING_OFF }
};

// Purpose: This function initializes state machine
void Init(STATE_MACHINE_STRUCT* state_machine) 
{
    // printf("Init warnings state machine.\r\n");
    state_machine->curr_state = STATE_WARNING_OFF;
}

// Purpose: This function retrieves current state
STATE_ENUM GetCurrentState(STATE_MACHINE_STRUCT* state_machine)
{
    return state_machine->curr_state;
}

// Purpose: This function retrieves state name
const char* GetStateName(STATE_ENUM state)
{
    return state_function[state].name;
}

// Purpose: This function governs transition between states.
// All of the logic is controlled by the state transition matrix above. 
// Essentially, the function gets passed in an event, e.g. the current event, 
// and then runs through the state transition matrix row by row to find a 
// pre-defined state and event pair that match the current state and event. 
// If found then it transitions to the specified next state and then calls 
// the state function pointed to by the function pointer.

// The event would typically come as a result of some other operation or trigger. 
// However, the EVENT_NONE can be passed if no event has occurred, which is useful 
// if Transition() is invoked every loop cycle, but events are generated less frequently.
void Transition(STATE_MACHINE_STRUCT *state_machine, EVENT_ENUM event, FILE* fptr)
{
    // Iterate through the state transition matrix, checking if there is both a match with the current state and the event
    for (int i = 0; i < sizeof(state_transition_matrix) / sizeof(state_transition_matrix[0]); i++) 
    {
        if (state_transition_matrix[i].curr_state == state_machine->curr_state) 
        {
            if ((state_transition_matrix[i].event == event) || (state_transition_matrix[i].event == EVENT_ANY)) 
            {
                // Transition to the next state
                state_machine->curr_state =  state_transition_matrix[i].next_state;

                // Call the function associated with transition
                (state_function[state_machine->curr_state].func)(fptr);
                break;
            }
        }
    }
}