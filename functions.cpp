// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module implements functions

#include "functions.h"

#define PRINTF_MODE

// Purpose: This function retrieves current system time in milliseconds resolution
uint64_t GetSystemTime()
{
#if defined(_WIN32) || defined(_WIN64)
    struct _timeb timebuffer;
    _ftime(&timebuffer);
    return (uint64_t)(((timebuffer.time * ONE_SEC) + timebuffer.millitm));
#else
    struct timeb timebuffer;
    ftime(&timebuffer);
    return (uint64_t)(((timebuffer.time * ONE_SEC) + timebuffer.millitm));
#endif
}

// Purpose: This function creates timestamp for log file
char* CreateLogFileTimeStamp()
{
    time_t rawtime = time(NULL);
    struct tm* ptm = localtime(&rawtime);
    char* timestamp = (char*)malloc(sizeof(char) * 20);

    if (timestamp != NULL)
    {
        sprintf(timestamp, "%04d_%02d_%02d_%02d_%02d_%02d",
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    }

    return timestamp;
}

// Purpose: This utility function creates a new node
struct Node* MakeNode(Pulse pulse)
{
    struct Node* new_node_ptr = (struct Node*)malloc(sizeof(struct Node));

    if (new_node_ptr != NULL)
    {
        new_node_ptr->pulse = pulse;
        new_node_ptr->next = NULL;
    }

    return new_node_ptr;
}

// Purpose: This function inserts a new node in the list in the ascending order 
// Params: A pointer to the head of a list and an element to add
void InsertPulse(struct Node** head_ptr, struct Node* new_node_ptr)
{
    if (new_node_ptr != NULL)
    {
        // Special case for the head
        if ((*head_ptr == NULL) || ((*head_ptr)->pulse.temp >= new_node_ptr->pulse.temp))
        {
            new_node_ptr->next = *head_ptr;
            *head_ptr = new_node_ptr;
        }
        else
        {
            // Locate the node before the place of insertion
            struct Node* curr_ptr = *head_ptr;

            while ((curr_ptr->next != NULL) && (curr_ptr->next->pulse.temp < new_node_ptr->pulse.temp))
            {
                curr_ptr = curr_ptr->next;
            }

            new_node_ptr->next = curr_ptr->next;
            curr_ptr->next = new_node_ptr;
        }
    }
}

// Purpose: This function traverses a list and deletes nodes with stale data, if any are found
// Nodes with stale data have their timestamp values smaller by at least 1000 milliseconds 
// than that of the new node to be inserted
// Params: A pointer to the head of a list and a new node with the youngest timestamp
void DeleteStalePulses(struct Node* head_ptr, Pulse pulse, FILE* fptr)
{
    if (head_ptr == NULL)
        return;

    PrintStr("Stale: ", fptr);

    // Until the head data is equal to the key move the head pointer
    while ((head_ptr != NULL) && ((head_ptr->pulse.timestamp + ONE_SEC) < pulse.timestamp))
    {
        head_ptr = head_ptr->next;
    }

    // struct Node* curr_ptr = head_ptr;
    struct Node* curr_ptr = head_ptr, *prev_ptr = head_ptr;  

    // while (curr_ptr != NULL && curr_ptr->next != NULL)
    while (curr_ptr != NULL)  
    {
        if ((curr_ptr->pulse.timestamp + ONE_SEC) < pulse.timestamp)
        {
            // curr_ptr->next = curr_ptr->next->next;
            prev_ptr->next = curr_ptr->next;
            
            PrintInt(curr_ptr->pulse.temp, fptr);
        }
        else
        {
            // curr_ptr = curr_ptr->next;
            prev_ptr = curr_ptr;       
        }

        // Can you see an issue in this block of code?
        curr_ptr = curr_ptr->next;
    }

    PrintStr("\n", fptr);
}

// Purpose: This function prints contents of a list starting from the given node
// Params: A pointer to a node in a list
void PrintList(struct Node* node_ptr, FILE* fptr)
{
#ifdef PRINTF_MODE
    PrintStr("List:  ", fptr);

    while (node_ptr != NULL)
    {
        PrintInt(node_ptr->pulse.temp, fptr);
        node_ptr = node_ptr->next;
    }

    PrintStr("\n", fptr);

#endif
}

// Purpose: This function finds a median value in a list
// Params: A pointer to the head of a list
double FindMedian(Node* head_ptr)
{
    double median = 0;

    Node* slow_ptr = head_ptr;
    Node* fast_ptr = head_ptr;
    Node* slow_ptr_prev = head_ptr;

    if (head_ptr != NULL)
    {
        while ((fast_ptr != NULL) && (fast_ptr->next != NULL))
        {
            fast_ptr = fast_ptr->next->next;
            slow_ptr_prev = slow_ptr;
            slow_ptr = slow_ptr->next;
        }

        // If the below condition evaluates to true, the linked list contains odd number of nodes.
        // Hence, simply return the middle element
        if (fast_ptr != NULL)
        {
            median = slow_ptr->pulse.temp;
        }
        else // The linked list contains even number of nodes
        {
            median = double(slow_ptr->pulse.temp + slow_ptr_prev->pulse.temp) / 2.0;
        }
    }

    return median;
}

// Purpose: This function simulates generation of electrical pulse signal width by a 
// temperature measuring sensor that is converted to a corresponding temperature value
// Input: Valid signal range boundaries (positive integers)
// Output: Electrical pulse temperature (milliseconds) and a timestamp
Pulse GeneratePulse(unsigned short lower, unsigned short upper)
{
    unsigned short width = (rand() % (upper - lower + 1)) + lower;
    unsigned short temperature = ConvertPulseWidthToTemperature(width);

    return { false, width, temperature, 0 };
}

// Purpose: This function converts an electrical pulse width to a corresponding temperature value
// Input: Electrical pulse width (milliseconds)
// Output: Temperature value (degrees Celsius)
unsigned short ConvertPulseWidthToTemperature(unsigned short pulse_width)
{
    const unsigned short offset = 5;
    const double scale_factor = 1.3333;

    return (unsigned short)(pulse_width - offset) * scale_factor;
}

// Purpose: This function converts a temperature value to an electrical pulse width
// Input: Temperature value (degrees Celsius)
// Output: Electrical pulse width (milliseconds)
unsigned short ConvertTemperatureToPulseWidth(unsigned short temp_val)
{
    const unsigned short offset = 5;
    const double scale_factor = 0.75;

    return (unsigned short)(temp_val * scale_factor) + offset;
}

// Purpose: This function checks to see whether predefined time limit has been reached or surpassed (timeout expired)
// Output: If time limit has been reached or supassed, it returns true; false otherwise
bool IsTimeout(uint64_t current_time, uint64_t start_time, uint64_t limit_time)
{
    return ((current_time - start_time) >= limit_time);
}

void PrintInt(int val, FILE* fptr)
{
#ifdef PRINTF_MODE
    printf(" %d", val);
    if (fptr != NULL)
        fprintf(fptr, " %d", val);
#endif  
}

void PrintFloat(double val, FILE* fptr)
{
#ifdef PRINTF_MODE
    printf(" %4.1f", val);
    if (fptr != NULL)
        fprintf(fptr, " %4.1f", val);
#endif  
}

void PrintStr(const char* str, FILE* fptr)
{
#ifdef PRINTF_MODE
    printf(str);
    if (fptr != NULL)
        fprintf(fptr, str);

#endif  
}

void PrintStrInt(const char* str, int val, FILE* fptr)
{
#ifdef PRINTF_MODE
    printf(str, val);
    if (fptr != NULL)
        fprintf(fptr, str, val);

#endif  
}