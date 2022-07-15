// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module defines functions

#pragma once

#include <time.h>
#include <stdio.h>
#include<windows.h>
#include <inttypes.h>
#include <sys/timeb.h>

#define ONE_SEC 1000
#define FIVE_MSEC 5

// Linked list data
typedef struct
{
    bool valid;
    unsigned short width;
    unsigned short temp;
    uint64_t timestamp;
} Pulse;

// Linked list node
struct Node
{
    Pulse pulse;
    struct Node* next;
};

// Function declarations
uint64_t GetSystemTime();
char* CreateLogFileTimeStamp();
struct Node* MakeNode(Pulse pulse);
void InsertPulse(struct Node** head_ptr, struct Node* new_node_ptr);
void DeleteStalePulses(struct Node* head_ptr, Pulse pulse, FILE* fptr);
void PrintList(struct Node* node_ptr, FILE* fptr);
double FindMedian(Node* head_ptr);
Pulse GeneratePulse(unsigned short lower, unsigned short upper);
unsigned short ConvertPulseWidthToTemperature(unsigned short pulse_width);
unsigned short ConvertTemperatureToPulseWidth(unsigned short temp_val);
bool IsTimeout(uint64_t current_time, uint64_t start_time, uint64_t limit_time);

void PrintInt(int val, FILE* fptr);
void PrintFloat(double val, FILE* fptr);
void PrintStr(const char* str, FILE* fptr);
void PrintStrInt(const char* str, int val, FILE* fptr);