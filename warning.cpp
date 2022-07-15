// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module implements functions used by state machine

#include "warning.h"

// The state functions are the functions which are called when the current 
// state and event matches a pair in the state transition matrix. 
// While the rest of the state machine controls the high-level flow, 
// the state functions are the guts of the state machine and 
// are the functions which actually do something.
// These state functions below are only really stubs, 
// and do not do much. Real life state functions do real life things.

void Warning_On(FILE* fptr)
{
    printf("\tWarning On\n");

    if (fptr != NULL)
        fprintf(fptr, "\tWarning On\n");
}

void Warning_Off(FILE* fptr)
{
    printf("\tWarning Off\n");

    if (fptr != NULL)
        fprintf(fptr, "\tWarning Off\n");
}