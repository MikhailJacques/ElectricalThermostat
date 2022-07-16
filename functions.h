// AUTHOR:		Mikhail Jacques
// PROJECT:		Electrical Thermostat
// DOCUMENT:	Electrical Thermostat
// DESCRIPTION: This module defines functions

#pragma once

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include<windows.h>
#include <inttypes.h>
#include <sys/timeb.h>

#define ONE_SEC 1000

#define PRINTF_MODE

// Linked list data
typedef struct
{
    bool valid;
    unsigned short width;
    double temp;
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
void DeleteStalePulses(struct Node* head_ptr, uint64_t timestamp, FILE* fptr);
void DeleteStalePulse(struct Node** head_ptr, uint64_t timestamp, FILE* fptr);
void PrintList(struct Node* node_ptr, FILE* fptr);
double FindMedian(Node* head_ptr);
unsigned short GeneratePulseWidth(unsigned short lower, unsigned short upper);
double ConvertPulseWidthToTemp(double pulse_width);
unsigned short ConvertTemperatureToPulseWidth(unsigned short temp_val);
bool IsTimeout(uint64_t current_time, uint64_t start_time, uint64_t limit_time);

void PrintInt(int val, FILE* fptr);
void PrintTemp(double val, FILE* fptr);
void PrintStr(const char* str, FILE* fptr);

void Warning_On(FILE* fptr);
void Warning_Off(FILE* fptr);
void Warning_On_Op(void);
void Warning_Off_Op(void);