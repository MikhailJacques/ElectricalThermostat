// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module simulates electrical thermostat measuring and reporting superficial system

// Algorithm:
// Create three threads
// First thread generates electrical pulses at a variable rate within the predefined range of [5, 80] (milliseconds)
// Second thread generates warnings in the intermittent (on/off) fashion at a predefined frequency of 5 milliseconds
// Main thread processes electrical pulses created by the first thread and commands the second thread to 
// generate warnings when temperature exceeds the predefined limit of 70 degrees Celsius for longer than 1 second

// Definition of a median:
// The median is the middle value in a list ordered from smallest to largest.
// If the elements of the list are arranged in order, then, the middle value which divides 
// the items into two parts with equal number of items on either side is called the median.

#include "functions.h"
#include "state_machine.h"

// Generated pulse signal is shared by two threads
static Pulse pulse = { false, 0, 0.0, 0 };

// Warning flag is shared by two threads
static bool warnings_on_flag = false;

CRITICAL_SECTION pulse_cs;
CRITICAL_SECTION print_cs;
CRITICAL_SECTION warning_cs;

DWORD WINAPI GeneratePulses(LPVOID ptr);
DWORD WINAPI GenerateWarnings(LPVOID ptr);

// Events to command threads to exit
HANDLE pulses_event_handle = CreateEvent(nullptr, true, false, nullptr);
HANDLE warnings_event_handle = CreateEvent(nullptr, true, false, nullptr);

int main(void)
{
    // ----- Configuration parameters -----
    const unsigned long warning_threshold = 1000;               // Milliseconds
    const unsigned short polling_time_interval = 1;             // Milliseconds
    const unsigned long measurement_duration_limit = 10000;     // Milliseconds (10 seconds)
    const unsigned short pulse_width_warning_threshold = 58;    // Milliseconds (pulse width 58 is equivalent to 70 degrees Celsius)

    // ----- Runtime parameters -----
    DWORD pulses_thread_id, warnings_thread_id;
    HANDLE pulses_thread_handle, warnings_thread_handle;

    uint64_t warning_alert_timestamp = GetSystemTime();
    bool warning_alert_flag = false;            // Stores warning alert           
    double pulse_width_median = 0;              // Stores calculated pulse width median
    FILE* fptr = NULL;                          // File pointer
    struct Node* head_ptr = NULL;               // Start with an empty list
    char* log_file_name = (char*)malloc(sizeof(char) * 30);
    Pulse new_pulse = { false, 0, 0.0, 0 };
    

    // Create log file with a unique timestamp
    if (log_file_name != NULL)
    {
        memset(log_file_name, 0, sizeof(char) * 30);
        strcpy(log_file_name, "log_");
        strcat(log_file_name, CreateLogFileTimeStamp());
        strcat(log_file_name, ".txt");
        fptr = fopen(log_file_name, "w");
    } 

    // Use current system time as seed for random number generator 
    srand((unsigned int)GetSystemTime());

    InitializeCriticalSection(&pulse_cs);
    InitializeCriticalSection(&warning_cs);
    InitializeCriticalSection(&print_cs);

    // Parameters: default security attributes, default stack size, thread function, 
    // parameter to thread function, default creation flags
    // Return: thread identifier
    pulses_thread_handle = CreateThread(NULL, 0, GeneratePulses, fptr, 0, &pulses_thread_id);
    warnings_thread_handle = CreateThread(NULL, 0, GenerateWarnings, fptr, 0, &warnings_thread_id);

    if ((pulses_thread_handle != NULL) && (warnings_thread_handle != NULL))
    {
        uint64_t start_time = GetSystemTime();

        while (GetSystemTime() < (start_time + measurement_duration_limit))
        {
            EnterCriticalSection(&pulse_cs);
            new_pulse = pulse;
            pulse.valid = false;
            LeaveCriticalSection(&pulse_cs);

            // Check to see whether a new pulse has arrived
            if (new_pulse.valid == true)
            {
                new_pulse.valid = false;

                EnterCriticalSection(&print_cs);
                DeleteStalePulses(head_ptr, new_pulse.timestamp, fptr);
                LeaveCriticalSection(&print_cs);

                InsertPulse(&head_ptr, MakeNode(new_pulse));

                EnterCriticalSection(&print_cs);
                PrintList(head_ptr, fptr);
                LeaveCriticalSection(&print_cs);

                pulse_width_median = FindMedian(head_ptr);

                EnterCriticalSection(&print_cs);
                PrintStr("Median:", fptr);
                PrintTemp(ConvertPulseWidthToTemp(pulse_width_median), fptr);
                LeaveCriticalSection(&print_cs);

                uint64_t current_time = GetSystemTime();

                // Check to see whether the temperature median has exceeded the temperature warning threshold
                if (pulse_width_median > pulse_width_warning_threshold)
                {
                    warning_alert_flag = true;  

                    EnterCriticalSection(&print_cs);
                    PrintStr(" - Alert duration", fptr);
                    PrintInt((int)(current_time - warning_alert_timestamp), fptr);
                    PrintStr("\n", fptr);
                    LeaveCriticalSection(&print_cs);
                }
                else
                {
                    warning_alert_flag = false;
                    warning_alert_timestamp = current_time;

                    EnterCriticalSection(&print_cs);
                    PrintStr("\n", fptr);
                    LeaveCriticalSection(&print_cs);
                }

                EnterCriticalSection(&warning_cs);
                warnings_on_flag = ((warning_alert_flag == true) 
                    && (IsTimeout(current_time, warning_alert_timestamp, warning_threshold))) ? true : false;
                LeaveCriticalSection(&warning_cs); 
            }

            Sleep(polling_time_interval);
        }

        // Command threads to exit
        SetEvent(pulses_event_handle);
        SetEvent(warnings_event_handle);

        // Wait for the threads to finish
        WaitForSingleObject(pulses_thread_handle, INFINITE);
        WaitForSingleObject(warnings_thread_handle, INFINITE);

        // Close thread handles
        CloseHandle(pulses_thread_handle);
        CloseHandle(warnings_thread_handle);
    }

    if (fptr != NULL)
        fclose(fptr);

    return 0;
}

// This function simulates generation of temperature sensor electrical signals
// It runs in a dedicated thread of execution
DWORD WINAPI GeneratePulses(LPVOID ptr)
{
    // ----- Configuration parameters -----
    const unsigned short pulse_width_lower_limit = 30;  // Milliseconds
    const unsigned short pulse_width_upper_limit = 80;  // Milliseconds
    const unsigned short pulse_interval = 20;           // Milliseconds

    unsigned short pulse_width = 0;
    double pulse_temp = 0;

    FILE* fptr = (FILE*)ptr;

    uint64_t start_time = GetSystemTime();

    while (true)
    {
        // Check exit event 
        if (WaitForSingleObject(warnings_event_handle, 0) == WAIT_OBJECT_0)
            return 0;

        pulse_width = GeneratePulseWidth(pulse_width_lower_limit, pulse_width_upper_limit);
        pulse_temp = ConvertPulseWidthToTemp(pulse_width);

        // Simulate signal width generation delay
        Sleep(pulse_width);

        EnterCriticalSection(&pulse_cs);

        pulse.valid = true; 
        pulse.width = pulse_width;
        pulse.temp = pulse_temp;

        // Simulate exact pulse arrival time
        pulse.timestamp = GetSystemTime();

        LeaveCriticalSection(&pulse_cs);

        EnterCriticalSection(&print_cs);
        PrintStr("New:   ", fptr);
        PrintTemp(pulse_temp, fptr);
        PrintStr("\n", fptr);
        LeaveCriticalSection(&print_cs);

        // Simulate pulse inter-arrival time
        Sleep(pulse_interval);
    }

    return 0;
}


// This function simulates intermittent activation of the warnings in the on/off fashion
// It runs in a dedicated thread of execution
DWORD WINAPI GenerateWarnings(LPVOID ptr)
{
    // ----- Configuration parameters -----
    const unsigned short warning_duration = 5;          // Milliseconds

    FILE* fptr = (FILE*)ptr;

    // State machine object that governs the intermittent activation of the warnings
    STATE_MACHINE_STRUCT state_machine;

    Init(&state_machine);

    while (true)
    {
        // Check exit event 
        if (WaitForSingleObject(warnings_event_handle, 0) == WAIT_OBJECT_0)
            return 0;

        EnterCriticalSection(&warning_cs);

        if (warnings_on_flag == true)
        {
            switch (GetCurrentState(&state_machine))
            {
                case STATE_WARNING_ON:

                    EnterCriticalSection(&print_cs);
                    Warning_On(fptr);
                    LeaveCriticalSection(&print_cs);

                    Transition(&state_machine, EVENT_WARNING_OFF);
                    break;

                case STATE_WARNING_OFF:

                    EnterCriticalSection(&print_cs);
                    Warning_Off(fptr);
                    LeaveCriticalSection(&print_cs);

                    Transition(&state_machine, EVENT_WARNING_ON);
                    break;
            }
        }
        else
        {
            Init(&state_machine);
        }

        LeaveCriticalSection(&warning_cs);

        Sleep(warning_duration);
    }

    return 0;
}