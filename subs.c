Purpose: This program allows
 a user to give commands to
 submarines in order complete
 a mission in 4 different terminals. */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

typedef void (*fp);

enum tExitCodes {
    EXIT_CRASHED, EXIT_SUCCEED
};

int Fuel, Distance;
int Terminal[10];
int n = 0;
int PID[10], ChildPID;
int Processes, Process;
FILE *fpt[10];
int Missiles;
const char *strDir = {"/dev/pts/"};

// randomizes a number
int randomize(int lower, int upper){
    int num;
    num = (rand() % (upper-lower+1)) + lower;
    
    return num;
}

// finds open terminals
void FindTerminals(void){
    int i;
    char term[30];
    
    // max of 30 terminals, check which ones are open
    for(i=0; i<30; i++){
        sprintf(term, "%s%d", strDir, i);
        fflush(stdout);
        
        // opens terminal to write to
        if ((fpt[n] = fopen(term, "w")) == NULL){
            //exit(1);
        }
        
        // assigns terminal
        else{
            Terminal[n] = i;
            n++;
        }
    }
}

// closes all terminals
void CloseTerminals(void){
    int i;
    for (i=0; i<n; i++){
        fclose(fpt[i]);
    }
}

// alarm that checks for specific needs for each submarine
void Alarm(int signum){
    static int display, add_distance, fuel_num, distance_num1, distance_num2, add_fuel;
    time_t t;
    time(&t);
    
    fuel_num = randomize(100,200);
    
    // alarm every second for fuel
    if (++add_fuel == 1){
        add_fuel = 0;
        Fuel = Fuel - fuel_num;
    }
    
    // alarm every other second for distance
    if (++add_distance == 2){
        add_distance = 0;
        // going towards target
        if (Missiles > 0){
            distance_num1 = randomize(5,10);
            Distance = Distance + distance_num1;
        }
        // returning to base
        if (Missiles == 0){
            distance_num2 = randomize(3,8);
            Distance = Distance - distance_num2;
            if (Distance < 0){
                Distance = 0;
            }
        }
    }
    
    // alarm every 3 seconds that prints status of sub
    if (++display == 3){
        display = 0;
        fprintf(fpt[Process], "Time: %s\tSub %d to base, %d gallons left, %d missiles left, %d miles from base.\n",ctime(&t), Process, Fuel, Missiles, Distance);
    }
    
    // determines if mission failed
    if ((Missiles > 0) && (Fuel <= 0)){
        fprintf(fpt[Process], "Mission Failed.\n");
        exit(EXIT_CRASHED);
    }
    
    // determines if mission succeeded
    if ((Missiles == 0) && (Distance <= 0)){
        fprintf(fpt[Process],"Mission Complete!\n");
        exit(EXIT_SUCCEED);
    }
    
    // checks to see if fuel is low
    if (Fuel < 500){
        fprintf(fpt[Process], "Sub %d running out of fuel!\n", Process);
    }
    
    // alarm every second
    alarm(1);
}

// launches missile
void Torpedo(int signum){
    int torpedo = Missiles;
    // if sub still has missiles
    if (Missiles > 0){
        torpedo = torpedo-1;
        fprintf(fpt[Process], "Missile Launched. Sub %d has %d missiles left.\n", Process, torpedo);
        if (--Missiles == 0){
            fprintf(fpt[Process], "Returning to base.\n");
        }
    }
    // if sub is out of missiles
    else fprintf(fpt[Process], "Sub %d has no ordinance.\n", Process);
}

// refeuls sub
void Refuel(int signum){
    if (Fuel == 0){
        fprintf(fpt[Process], "Sub %d dead in the water.\n", Process);
        printf("Rescue is on the way, sub %d", Process);
        exit(EXIT_CRASHED);
    }
    else{
        Fuel = randomize(1000, 5000);
        fprintf(fpt[Process], "Sub %d refueled\n", Process);
    }
}

// assigns everything to each child
void Child(void){
    ChildPID = getpid();
    // sends signals for each task
    signal(SIGALRM, (fp)Alarm);
    signal(SIGUSR2, (fp)Refuel);
    signal(SIGUSR1, (fp)Torpedo);
    // assigns initial fuel, distance, and number of missiles
    Fuel = randomize(1000, 5000);
    Missiles = randomize(6,10);
    Distance = 0;
    // alarm for every second
    alarm(1);
    while(1){
    }
}

// accounts for what is being done in terminal through commands
void mainTerminal(void){
    char input[10];
    int p;
    time_t t;
    time(&t);
    
    Processes = 3;
    // reads in user input
    while (Processes > 0){
        scanf("%s", input);
        fflush(stdin);

        if (strlen(input) == 2){
            // ascii to num
            p = atoi(&input[1]);
            if ((p > 0) && (p < 4)){
                // cases for each given command
                switch (input[0]){
                    // launch missile
                    case 'l':
                        kill(PID[p], SIGUSR1);
                        break;
                    // refuel
                    case 'r':
                        kill(PID[p], SIGUSR2);
                        break;
                    // kill child process
                    case 's':
                        kill(PID[p], 1);
                        Processes--;
                        break;

                }
            }
            else printf("Invalid sub number.\n");
        }
        else if (strlen(input) == 1){
            // terminates all proccesses
            if (input[0] == 'q'){
                for (p = 1; p <= 3; p++){
                    kill(PID[p], 1);
                    Processes--;
                }
                printf("Time: %s",ctime(&t));
                exit(1);
            }
        }
    }
}

// determines wether mission was a success or failure for each process
void missionSuccess(int p, int status){
    char exitStatus;
    
    // shifts status 8 bits to right to clear it
    exitStatus = status >> 8;
    
    // prints statement
    if (exitStatus == EXIT_SUCCEED)
        printf("Process %d's mission was a success!\n", p%10);
    else
        printf("Process %d's mission was a failure.\n", p%10);
}

// parent waits for child processes and calls missionSuccess function
void ParentF(void){
    int i;
    int child[10], parent[10];
    
    // parent waits for each child process to exit or receive signal
    parent[1] = wait(&child[1]);
    parent[2] = wait(&child[2]);
    parent[3] = wait(&child[3]);
    
    // calls missionSuccess for each child
    for (i=1; i <= 3; i++){
        missionSuccess(parent[i], child[i]);
    }
}

int main(){
    int i;
    int Parent = 1;
    time_t t;
    time(&t);
    // prints current time
    printf("\nTime: %s", ctime(&t));
    
    // needed for random number generator
    srand(time(0));
    
    // finds each terminal
    FindTerminals();
    
    // gets PID of process
    Process = 0;
    PID[Process] = getpid();
    
    for (i=1; i<=3; i++){
        if (Parent){
            Process = i;
            // forks the process so it runs simulataneously
            PID[Process] = fork();
            if (PID[i] == 0){
                Parent = 0;
                Child();
            }
        }
    }
    
    // assigns main terminal where commands will be given, otherwise terminates everything if fork returns 0
    if (Parent){
        PID[4] = fork();
        if (PID[4] == 0){
            mainTerminal();
        }
        else{
            ParentF();
            kill(PID[4], 1);
            CloseTerminals();
        }
    }
    
    return 0;
}

