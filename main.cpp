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

#include "warning.h"
#include "functions.h"
#include "state_machine.h"

// Generated pulse signal is shared by two threads
static Pulse pulse = { 0, 0, 0 };

// Warning flag is shared by two threads
static bool warnings_flag = false;

CRITICAL_SECTION pulses_cs;
CRITICAL_SECTION warnings_cs;
CRITICAL_SECTION prints_cs;

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
    const unsigned short temp_warning_threshold = 70;           // Degrees Celsius (equivalent of pulse width 58)

    // ----- Runtime parameters -----
    DWORD pulses_thread_id, warnings_thread_id;
    HANDLE pulses_thread_handle, warnings_thread_handle;

    uint64_t warning_alert_timestamp = GetSystemTime();
    bool warning_alert_flag = false;            // Stores warning alert           
    double temp_median = 0;                     // Stores calculated temperature median
    FILE* fptr = NULL;                          // File pointer
    struct Node* head_ptr = NULL;               // Start with an empty list
    char* log_file_name = (char*)malloc(sizeof(char) * 30);

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
    srand(GetSystemTime());

    InitializeCriticalSection(&pulses_cs);
    InitializeCriticalSection(&warnings_cs);
    InitializeCriticalSection(&prints_cs);

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
            EnterCriticalSection(&pulses_cs);

            // Check to see whether a new pulse has arrived
            if (pulse.valid == true)
            {
                pulse.valid = false;

                EnterCriticalSection(&prints_cs);
                DeleteStalePulses(head_ptr, pulse, fptr);
                LeaveCriticalSection(&prints_cs);

                InsertPulse(&head_ptr, MakeNode(pulse));

                PrintList(head_ptr, fptr);

                temp_median = FindMedian(head_ptr);

                EnterCriticalSection(&prints_cs);
                PrintStr("Median:", fptr);
                PrintFloat(temp_median, fptr);
                LeaveCriticalSection(&prints_cs);

                uint64_t current_time = GetSystemTime();

                // Check to see whether the temperature median has exceeded the temperature warning threshold
                if (temp_median > temp_warning_threshold)
                {
                    warning_alert_flag = true;  

                    EnterCriticalSection(&prints_cs);
                    PrintStr(" - Alert duration", fptr);
                    PrintInt((int)(current_time - warning_alert_timestamp), fptr);
                    PrintStr("\n", fptr);
                    LeaveCriticalSection(&prints_cs);
                }
                else
                {
                    warning_alert_flag = false;
                    warning_alert_timestamp = current_time;

                    EnterCriticalSection(&prints_cs);
                    PrintStr("\n", fptr);
                    LeaveCriticalSection(&prints_cs);
                }

                EnterCriticalSection(&warnings_cs);
                warnings_flag = ((warning_alert_flag == true) 
                    && (IsTimeout(current_time, warning_alert_timestamp, warning_threshold))) ? true : false;
                LeaveCriticalSection(&warnings_cs);
            }

            LeaveCriticalSection(&pulses_cs);

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

    FILE* fptr = (FILE*)ptr;

    uint64_t start_time = GetSystemTime();

    while (true)
    {
        // Check exit event 
        if (WaitForSingleObject(warnings_event_handle, 0) == WAIT_OBJECT_0)
            return 0;

        EnterCriticalSection(&pulses_cs);

        pulse = GeneratePulse(pulse_width_lower_limit, pulse_width_upper_limit);

        EnterCriticalSection(&prints_cs);
        PrintStr("New:   ", fptr);
        PrintInt(pulse.temp, fptr);
        PrintStr("\n", fptr);
        LeaveCriticalSection(&prints_cs);

        // Simulate signal width generation delay
        Sleep(pulse.width);

        // Simulate exact pulse arrival time
        pulse.timestamp = GetSystemTime();
        pulse.valid = true;

        LeaveCriticalSection(&pulses_cs);

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
    const unsigned short warning_duration = 4;          // Milliseconds

    FILE* fptr = (FILE*)ptr;

    // State machine object that governs the intermittent activation of the warnings
    STATE_MACHINE_STRUCT state_machine;

    Init(&state_machine);

    uint64_t current_time = GetSystemTime();
    uint64_t warning_on_timestamp = current_time;       // Stores warning on timestamp
    uint64_t warning_off_timestamp = current_time;      // Stores warning off timestamp

    while (true)
    {
        // Check exit event 
        if (WaitForSingleObject(warnings_event_handle, 0) == WAIT_OBJECT_0)
            return 0;

        EnterCriticalSection(&warnings_cs);

        if (warnings_flag == true)
        {
            current_time = GetSystemTime();

            switch (GetCurrentState(&state_machine))
            {
                case STATE_WARNING_ON:

                    if (IsTimeout(current_time, warning_off_timestamp, warning_duration))
                    {
                        warning_on_timestamp = current_time;

                        EnterCriticalSection(&prints_cs);
                        Transition(&state_machine, EVENT_WARNING_OFF, fptr);
                        LeaveCriticalSection(&prints_cs);
                    }

                    break;

                case STATE_WARNING_OFF:

                    if (IsTimeout(current_time, warning_on_timestamp, warning_duration))
                    {
                        warning_off_timestamp = current_time;

                        EnterCriticalSection(&prints_cs);
                        Transition(&state_machine, EVENT_WARNING_ON, fptr);
                        LeaveCriticalSection(&prints_cs);
                    }

                    break;
            }
        }

        LeaveCriticalSection(&warnings_cs);

        Sleep(1);
    }

    return 0;
}