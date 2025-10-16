#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

const int SEED_VALUE = 200;  // Seed value for reading from file

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
typedef struct Process {
    int A;                         // A: Arrival time of the process
    int B;                         // B: Upper Bound of CPU burst times of the given random integer list
    int C;                         // C: Total CPU time required
    int M;                         // M: Multiplier of CPU burst time
    int processID;                 // The process ID given upon input read
    int currentCPUTimeRun;        // The current amount of CPU time the process has run for
    int currentIOBlockedTime;     // The current amount of time the process has
    int currentWaitingTime;       // The current amount of time the process has spent waiting
    int finishingTime;            // The time the process finished
    int currentCPUBurst;          // The current CPU burst time remaining
    int currentIOBurst;           // The current IO burst time remaining

    // Add other fields as needed
} _process;

int TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed

int TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished

int CURRENT_CYCLE = 0;             // The current cycle of the scheduler

int TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0; // The total number of cycles spent in the blocked state

// Additional variables as needed



// reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
int getRandNumFromFile(int line, FILE* random_num_file_ptr){
    int end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (int) atoi(str);
    }

    // fail-safe return
    return (int) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
int randomOS(int upper_bound, int process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    int unsigned_rand_int = (int) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    int returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/



void printInput(_process process_list[]);

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    int i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process  (helper function, you may need to adjust variables accordingly)
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    int i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data (helper function, you may need to adjust variables accordingly)
 * process_list The original processes inputted in array form
 */
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
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
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
} // End of the print summary data function


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

    // read the processes from the file
    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++) {
        fscanf(file_ptr, " (%d %d %d %d)", &process_list[i].A, &process_list[i].B, &process_list[i].C, &process_list[i].M);
        process_list[i].processID = i;
    }

    fclose(file_ptr);
    printInput(process_list);
    









    printf("Success!\n");
    return 0;
}

// FUNCTION DEFINITIONS ---------------------------------------------------------------------------------

// prints to standard output the original input
void printInput(_process process_list[])
{
    printf("\nInput: %i", TOTAL_CREATED_PROCESSES);

    int i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" (%d %d %d %d)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
}