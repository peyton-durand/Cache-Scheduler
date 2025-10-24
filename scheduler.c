#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


// GLOBAL VARIABLES --------------------------------------------------------------------------------------
typedef enum {false, true} bool; // boolean type in C
typedef enum {UNSTARTED, READY, RUNNING, BLOCKED, TERMINATED} State; // states of a process

const int SEED_VALUE = 200;  // seed value for reading from file

int TOTAL_CREATED_PROCESSES = 0;                // the total number of processes constructed
int TOTAL_FINISHED_PROCESSES = 0;               // the total number of processes that have finished
int CURRENT_CYCLE = 0;                          // the current cycle of the scheduler
int TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;   // the total number of cycles spent in the blocked state

// process struct
typedef struct Process {
    int processID;                // the process ID given upon input read
    int arrival;                  // A: Arrival time of the process
    int upperBound;               // B: Upper Bound of CPU burst times of the given random integer list
    int cpuTime;                  // C: Total CPU time required
    int multiplier;               // M: Multiplier of CPU burst time

    int cpuBurst;                 // total CPU time required, from random function
    int ioBurst;                  // total IO time required, cpuTime * multiplier

    State currentState;           // The current state of the process

    int remainingCPUBurst;        // the current CPU burst time remaining
    int remainingIOBurst;         // the current IO burst time remaining
    int currentWaitingTime;       // the current amount of time the process has spent waiting


    int totalCPURunTime;          // the total amount of time the process has spent running on the CPU
    int totalIOBlockedTime;       // the total amount of time the process has spent blocked for IO
    int totalWaitingTime;         // the total amount of time the process has spent waiting

    int finishingTime;            // the time the process finished
} _process;


// FUNCTION PROTOTYPES -----------------------------------------------------------------------------------
int randomOS(int upper_bound, int process_indx, FILE* random_num_file_ptr);
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr);

void run_fcfs(_process process_list[], _process finished_process_list[]);

void printInput(_process process_list[]);
void printFinal(_process finished_process_list[]);
void printProcessSpecifics(_process process_list[]);
void printSummaryData(_process process_list[]);


// MAIN FUNCTION -----------------------------------------------------------------------------------------

// argc is the number of command line arguments
// argv is an array of strings (char pointers) representing the command line arguments
// argv[0] is the name of the program, argv[1] is the first command line argument, argv[2] is the second, etc.
int main(int argc, char *argv[])
{
    // ensure proper command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]); // fprintf allows us to print to stderr instead of stdout
        return 1;
    }

    // open the input file
    char *input_file = argv[1];
    FILE *file_ptr = fopen(input_file, "r");
    if (file_ptr == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", input_file);
        return 1;
    }

    fscanf(file_ptr, "%d", &TOTAL_CREATED_PROCESSES); // read the number of processes from the file
    _process process_list[TOTAL_CREATED_PROCESSES]; // array to hold the processes
    _process finished_process_list[TOTAL_CREATED_PROCESSES]; // array to hold the finished processes

    // read the processes from the file
    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
        fscanf(file_ptr, " (%d %d %d %d)", &process_list[i].arrival, &process_list[i].upperBound,
            &process_list[i].cpuTime, &process_list[i].multiplier);
        process_list[i].processID = i;
    }

    fclose(file_ptr);
    printInput(process_list);
    
    // pull random numbers for CPU bursts
    char *random_file = "random-numbers";
    file_ptr = fopen(random_file, "r");
    if (file_ptr == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", random_file);
        return 1;
    }
    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
        int random_number = randomOS(process_list[i].upperBound, i, file_ptr);
        printf("Process %d random number: %d\n", i, random_number);
        process_list[i].cpuBurst = random_number;
        process_list[i].ioBurst = process_list[i].cpuBurst * process_list[i].multiplier;
    }
    fclose(file_ptr);

    // run the FCFS scheduling simulation
    printf("\n-------------------------------- FCFS Scheduler --------------------------------\n");
    printInput(process_list);
    run_fcfs(process_list, finished_process_list);
    printFinal(finished_process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("------------------------------------------------------------------------------------\n");

    return 0;
}


// FUNCTION DEFINITIONS ----------------------------------------------------------------------------------


// reads a random non-negative integer X from a file named random-numbers
// returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
int randomOS(int upper_bound, int process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    int unsigned_rand_int = (int) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    int returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 

// helper function for randomOS
// gets the random number from the file at the specified line
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // Fail-safe return
    return (uint32_t) 1804289383;
}

/// FCFS scheduler
void run_fcfs(_process process_list[], _process finished_process_list[])
{
    // set defaults
    TOTAL_FINISHED_PROCESSES = 0;
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
        process_list[i].currentState = UNSTARTED;
        process_list[i].remainingCPUBurst = process_list[i].cpuBurst;
        process_list[i].remainingIOBurst = process_list[i].ioBurst;
        process_list[i].currentWaitingTime = 0;
        process_list[i].totalCPURunTime = 0;
        process_list[i].totalIOBlockedTime = 0;
        process_list[i].totalWaitingTime = 0;
        process_list[i].finishingTime = 0;
    }

    printf("\nStarting Simulation...\n");
    CURRENT_CYCLE = 0;

    while(TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES) {

        printf("Cycle %d ------------------------------------\n", CURRENT_CYCLE);

        // check for new arrivals
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].arrival == CURRENT_CYCLE) {
                process_list[i].currentState = READY;
                process_list[i].currentWaitingTime = 0;
            }
        }

        // check blocked processes
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].currentState == BLOCKED) {
                if (process_list[i].remainingIOBurst == 0) {
                    process_list[i].currentState = READY;
                    process_list[i].currentWaitingTime = 0;
                }
            }
        }

        // check running process and possibly start different process
        bool running_process = false;
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].currentState == RUNNING) {
                if (process_list[i].totalCPURunTime == process_list[i].cpuTime) {
                    process_list[i].currentState = TERMINATED;
                    process_list[i].finishingTime = CURRENT_CYCLE;
                    // record the finished process in order
                    finished_process_list[TOTAL_FINISHED_PROCESSES] = process_list[i];
                    TOTAL_FINISHED_PROCESSES++;
                    running_process = false;
                    break;
                }
                if (process_list[i].remainingCPUBurst == 0) {
                    process_list[i].currentState = BLOCKED;
                    process_list[i].remainingIOBurst = process_list[i].ioBurst;
                    running_process = false;
                    break;
                }
                running_process = true;
                break;
            }
        }
        if (!running_process) {
            // choose the READY process that has been waiting the longest (FCFS)
            int chosen_idx = -1;
            int max_wait = -1;
            for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
                if (process_list[i].currentState == READY) {
                    int wait = process_list[i].currentWaitingTime;
                    if (wait > max_wait) {
                        max_wait = wait;
                        chosen_idx = i;
                    } else if (wait == max_wait && chosen_idx != -1) {
                        // tie-breaker: prefer lower processID
                        if (process_list[i].processID < process_list[chosen_idx].processID) {
                            chosen_idx = i;
                        }
                    }
                }
            }

            if (chosen_idx != -1) {
                process_list[chosen_idx].currentState = RUNNING;
                process_list[chosen_idx].remainingCPUBurst = process_list[chosen_idx].cpuBurst;
            }
        }

        // print states of all processes
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            const char* state_str;
            switch (process_list[i].currentState) {
                case UNSTARTED: state_str = "UNSTARTED"; break;
                case READY: state_str = "READY"; break;
                case RUNNING: state_str = "RUNNING"; break;
                case BLOCKED: state_str = "BLOCKED"; break;
                case TERMINATED: state_str = "TERMINATED"; break;
                default: state_str = "UNKNOWN"; break;
            }
            printf("Process %d: %s\n", i, state_str);
        }

        // process state varible updates
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
            if (process_list[i].currentState == RUNNING) {
                process_list[i].totalCPURunTime++;
                process_list[i].remainingCPUBurst--;
            }
            else if (process_list[i].currentState == BLOCKED) {
                process_list[i].totalIOBlockedTime++;
                process_list[i].remainingIOBurst--;
                TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED++;
            }
            else if (process_list[i].currentState == READY) {
                process_list[i].currentWaitingTime++;
                process_list[i].totalWaitingTime++;
            }
        }

        // increment cycle
        CURRENT_CYCLE++;
    }
    printf("---------------------------\nFCFS Scheduling Simulation Ended.\n");
}

// prints the original input to standard out
void printInput(_process process_list[])
{
    printf("Input: %i", TOTAL_CREATED_PROCESSES);

    int i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" (%d %d %d %d)", process_list[i].arrival, process_list[i].upperBound,
            process_list[i].cpuTime, process_list[i].multiplier);
    }
    printf("\n");
}

// prints to standard output the final output
// finished_process_list is the terminated processes (in array form) in the order they each finished in
void printFinal(_process finished_process_list[])
{
    printf("\nThe (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    int i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].arrival, finished_process_list[i].upperBound,
               finished_process_list[i].cpuTime, finished_process_list[i].multiplier);
    }
    printf("\n");
}

// prints out specifics for each process  (helper function, you may need to adjust variables accordingly)
// @param process_list The original processes inputted, in array form
void printProcessSpecifics(_process process_list[])
{
    int i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].arrival, process_list[i].upperBound,
               process_list[i].cpuTime, process_list[i].multiplier);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].arrival);
        printf("\tI/O time: %i\n", process_list[i].totalIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].totalWaitingTime);
        printf("\n");
    }
}

// prints out the summary data (helper function, you may need to adjust variables accordingly)
void printSummaryData(_process process_list[])
{
    int i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    int final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].totalCPURunTime;
        total_amount_of_time_io_blocked += process_list[i].totalIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].totalWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].arrival);
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
}