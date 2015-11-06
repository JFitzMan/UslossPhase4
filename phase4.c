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

int debugflag4 = 0;

static int ClockDriver(char *);
static int DiskDriver(char *);

static procPtr SleepList;

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

    //initialize the SleepList
    SleepList = NULL;

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
    /*
     * Wait for the clock driver to start. The idea is that ClockDriver
     * will V the semaphore "semRunning" once it is running.
     */

    sempReal(semRunning);
    if (debugflag4)
        USLOSS_Console("start3(): Started clock driver and synched with it via semRunning\n");

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
    join(&status);

    // eventually, at the end:
    quit(0);
    
}

static int
ClockDriver(char *arg)
{
    int result;
    int status;

    // Let the parent know we are running and enable interrupts.
    semvReal(semRunning);
    if (debugflag4)
        USLOSS_Console("ClockDriver(): Starting\n");
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    // Infinite loop until we are zap'd
    while(! isZapped()) {
        //send to wait device mailbox
	    result = waitDevice(USLOSS_CLOCK_DEV, 0, &status);
        if (debugflag4)
            USLOSS_Console("ClockDriver(): checking for procs to wake up\n");
	    if (result != 0) {
	       return 0;
	    }

        int curTime = USLOSS_Clock();
        //wakeup loop
        while(SleepList != NULL && SleepList->timeToWakeUp < curTime){
            wakeUpNextProc();
        }

    }
    //been zapped, quitting time
    quit(0);
}

static int
DiskDriver(char *arg)
{
    int unit = atoi( (char *) arg); 	// Unit is passed as arg.
    return 0;
}

void
sleep(systemArgs *args){

    int seconds = (int)args->arg1;
    int me = getpid();

    if (debugflag4)
        USLOSS_Console("sleep(): got %d seconds from Sleep. Goodnight world\n", seconds);

    //ensure the process table is set up for this process
    if (procTable[me].pid == -1){
        setUpSleepSlot(me);
    }
    //call sleepReal
    int toReturn = sleepReal(seconds);

    if (toReturn  == -1){
        USLOSS_Console("sleep(): There was a problem, sleepReal returned -1\n");
    }
    args->arg1 = 0;
    //set back to user mode before retuning to system call Sleep
    setToUserMode();
}

//adds process to sleep list and blocks
int 
sleepReal(int seconds){

    int curTime = USLOSS_Clock();
    int me = getpid();
    //update values in the process table
    procTable[me].timeToWakeUp  = curTime + seconds*1000000;
    //add to sleepList
    addToSleepList(&procTable[me]);
    //block on private mailbox
    MboxReceive(procTable[me].privateMbox, NULL, 0);
    //if the time it was asleep is less than it was supposed to, -1
    if (debugflag4)
        USLOSS_Console("sleepReal(): waking up from sleep\n");
    if (procTable[me].timeToWakeUp < curTime + seconds*1000000)
        return -1;
    return 0;

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

    MboxSend(procTable_mutex, NULL, 0);
    //inititalize process table
    for(int i = 0; i < MAXPROC; i++){
        procTable[i].pid = -1;
        procTable[i].nextProc = NULL;
        procTable[i].priority = -1;
        procTable[i].name = NULL;
        procTable[i].privateMbox = -1;
        procTable[i].termCode = -1;
        procTable[i].status = -1;
        procTable[i].timeToWakeUp = -1;
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
    procTable[getpid()].name = "start2";
    procTable[getpid()].status = READY;
    procTable[getpid()].priority = 1;
    procTable[getpid()].privateMbox = MboxCreate(0,50);

    MboxReceive(procTable_mutex, NULL, 0);

    if (debugflag4)
        USLOSS_Console("initializeProcTable(): Completed\n");

}

//get a process table slot ready for bed
void setUpSleepSlot(int pid){
    procTable[pid].pid = pid;
    procTable[pid].status = SLEEPING;
    procTable[pid].privateMbox = MboxCreate(0,50);
}

//wakes up the proc at the front of the SleepList, and updates the SleepList
void wakeUpNextProc(){
    //grab the proc to wake up
    procPtr toWake = SleepList;
    if (debugflag4)
        USLOSS_Console("wakeUpNextProc(): waking up process %d\n", toWake->pid);
    //update the sleep list
    if (SleepList->nextProc == NULL)
        SleepList = NULL;
    else
        SleepList = SleepList->nextProc;

    //update the procs table
    toWake->status = READY;
    toWake->nextProc = NULL;
    //wake up by sending to private mbox
    MboxSend(toWake->privateMbox, NULL, 0);
}

void setToUserMode(){
    unsigned int psr = USLOSS_PsrGet();
    psr = psr >> 1;
    psr = psr << 1;
    USLOSS_PsrSet(psr);
}

//adds to sleep list, and maintains the ordered queue
void addToSleepList(procPtr toAdd){

    //list is empty, toAdd is the front
    if(SleepList == NULL){
        SleepList = toAdd;
    }
    //toAdd has the shortest nap on the list
    else if (toAdd->timeToWakeUp < SleepList->timeToWakeUp){
        toAdd->nextProc = SleepList;
        SleepList = toAdd;
    }
    //scann until toAdd fits in
    else{
        procPtr prev = NULL;
        procPtr cur;
        int added = 0;
        for (cur = SleepList; cur != NULL; cur = cur->nextProc){
            if (cur->timeToWakeUp > toAdd->timeToWakeUp){
                prev->nextProc =toAdd;
                toAdd->nextProc = cur;
                added = 1;
                break;
            }
            prev = cur;
        }//end for

        //check to see if it was  added. If not, it goes at the end
        if(!added){
            cur = SleepList;
            while (cur->nextProc != NULL){
                cur = cur->nextProc;
            }
            cur->nextProc = toAdd;
        }
    }

    if (debugflag4)
        USLOSS_Console("addToSleepList(): Process %d added to SleepList, timeToWakeUp: %d\n",toAdd->pid, toAdd->timeToWakeUp);
}

