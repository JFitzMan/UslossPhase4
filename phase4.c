#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include "phase4.h"
#include "provided_prototypes.h"
#include <stdlib.h> /* needed for atoi() */
#include <stdio.h>
#include <string.h>
#include <usyscall.h>


int semRunning;
int procTable_mutex;
//process table
struct procSlot procTable[MAXPROC];

int debugflag4 = 1;

static int ClockDriver(char *);
static int DiskDriver(char *);

void
start3(void)
{
    char	name[128];
    char    termbuf[10];
    int		i;
    int		clockPID;
    int		pid;
    int		status;

    /*
     * Check kernel mode here.
     */
    inKernelMode("start3");
    if (debugflag4)
        USLOSS_Console("start3(): started in kernel mode\n");

    /*
     * Initialize data structures
     */

    //link new system calls to syscall vec
    systemCallVec[SYS_SLEEP] = sleep;

    //initialize global mailbox for proc table editing
    procTable_mutex = MboxCreate(1, 0);

    //initialize phase 4 process table
    initializeProcTable();

    /*
     * Create clock device driver 
     * I am assuming a semaphore here for coordination.  A mailbox can
     * be used instead -- your choice.
     */
    semRunning = semcreateReal(0);
    clockPID = fork1("Clock driver", ClockDriver, NULL, USLOSS_MIN_STACK, 2);
    if (clockPID < 0) {
	   USLOSS_Console("start3(): Can't create clock driver\n");
	   USLOSS_Halt(1);
    }
    if (debugflag4)
        USLOSS_Console("start3(): Started clock driver process, about to wait for it\n");
    /*
     * Wait for the clock driver to start. The idea is that ClockDriver
     * will V the semaphore "semRunning" once it is running.
     */

    sempReal(semRunning);
    if (debugflag4)
        USLOSS_Console("start3(): synched up with clock driver\n");

    /*
     * Create the disk device drivers here.  You may need to increase
     * the stack size depending on the complexity of your
     * driver, and perhaps do something with the pid returned.
     */

     /*
    for (i = 0; i < USLOSS_DISK_UNITS; i++) {
        sprintf(buf, "%d", i);
        pid = fork1(name, DiskDriver, buff, USLOSS_MIN_STACK, 2);
        if (pid < 0) {
            USLOSS_Console("start3(): Can't create term driver %d\n", i);
            USLOSS_Halt(1);
        }
    }
    sempReal(semRunning);
    sempReal(semRunning);

    */

    /*
     * Create terminal device drivers.
     */


    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case first letters.
     */
    pid = spawnReal("start4", start4, NULL, 4 * USLOSS_MIN_STACK, 3);
    pid = waitReal(&status);

    /*
     * Zap the device drivers
     */
    zap(clockPID);  // clock driver

    // eventually, at the end:
    quit(0);
    
}

static int
ClockDriver(char *arg)
{
    int result;
    int status;

    // Let the parent know we are running and enable interrupts.
    if (debugflag4)
        USLOSS_Console("ClockDriver(): Waking up start3 with semvReal()\n");
    semvReal(semRunning);
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    // Infinite loop until we are zap'd
    while(! isZapped()) {
	result = waitDevice(USLOSS_CLOCK_DEV, 0, &status);
	if (result != 0) {
	    return 0;
	}
	/*
	 * Compute the current time and wake up any processes
	 * whose time has come.
	 */
    }
    return 0;
}

static int
DiskDriver(char *arg)
{
    int unit = atoi( (char *) arg); 	// Unit is passed as arg.
    return 0;
}

void
sleep(systemArgs *args){

    int seconds = args->arg1;

    if (debugflag4)
        USLOSS_Console("sleep(): got %d seconds from Sleep\n", seconds);
}

//utility function, will halt if kernel mode is called
int 
inKernelMode(char *procName)
{
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
      USLOSS_Console("Kernel Error: Not in kernel mode, may not run %s()\n", procName);
      USLOSS_Halt(1);
      return 0;
    }
    else{
      return 1;
    }
}

//utility function to be called one time in start3
//removed it from start3 because it was very long
void
initializeProcTable(){
    //inititalize process table
    for(int i = 0; i < MAXPROC; i++){
        procTable[i].pid = -1;
        procTable[i].parentPid = -1;
        procTable[i].nextChild = NULL;
        procTable[i].nextSib = NULL;
        procTable[i].priority = -1;
        procTable[i].start_func = NULL;
        procTable[i].name = NULL;
        procTable[i].arg = NULL;
        procTable[i].stack_size = -1;
        procTable[i].privateMbox = -1;
        procTable[i].termCode = -1;
        procTable[i].status = -1;
    }
    //Manually add in entries for sentinel, start1, and start2
    procTable[1].name = "sentinel";
    procTable[1].priority = 6;
    procTable[1].pid = 1;
    procTable[1].status = READY;
    procTable[2].name = "start1";
    procTable[2].priority = 1;
    procTable[2].pid = 2;
    procTable[2].status = JOIN_BLOCKED;
    procTable[3].name = "start2";
    procTable[3].priority = 1;
    procTable[3].pid = 3;
    procTable[3].status = JOIN_BLOCKED;

    //get start3 set in in proc table

    procTable[getpid()].pid = getpid();
    procTable[getpid()].parentPid = 3;  //start2 PID
    procTable[getpid()].name = "start2";
    procTable[getpid()].status = READY;
    procTable[getpid()].stack_size = USLOSS_MIN_STACK;
    procTable[getpid()].priority = 1;
    procTable[getpid()].nextChild = NULL;
    procTable[getpid()].nextSib = NULL;
    procTable[getpid()].privateMbox = MboxCreate(0,sizeof(int[2]));
    procTable[getpid()].termCode = -1;

} //end initializeProcTable

