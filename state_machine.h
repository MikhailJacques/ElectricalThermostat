// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module defines state machine

#pragma once

typedef enum {
    STATE_WARNING_ON,
    STATE_WARNING_OFF
} STATE_ENUM;

typedef enum {
    EVENT_ANY,
    EVENT_WARNING_ON,
    EVENT_WARNING_OFF
} EVENT_ENUM;

// This simple state machine needs to remember only one thing, the current state. 
// All the state machine’s variables are declared in the STATE_MACHINE_STRUCT.
// A pointer to this struct gets passed in as the first variable to all the state machine functions, 
// just like the 'this' object gets passed in the object-oriented world.
typedef struct 
{
    STATE_ENUM curr_state;
} STATE_MACHINE_STRUCT;

void Init(STATE_MACHINE_STRUCT* state_machine);
void Transition(STATE_MACHINE_STRUCT* state_machine, EVENT_ENUM event, FILE* fptr);
STATE_ENUM GetCurrentState(STATE_MACHINE_STRUCT* state_machine);
const char* GetStateName(STATE_ENUM state);