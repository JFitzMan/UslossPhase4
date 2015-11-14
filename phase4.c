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
int semTermWrite;
int semReadLines;
int procTable_mutex;

//terminal mailboxes
int charInbox[USLOSS_TERM_UNITS];
int charOutbox[USLOSS_TERM_UNITS];
int readMbox[USLOSS_TERM_UNITS];
int writeMbox[USLOSS_TERM_UNITS];

//disk info arrays and sems
static procPtr diskQ [USLOSS_DISK_UNITS][2];
static int tracks[USLOSS_DISK_UNITS];
static int diskArm[USLOSS_DISK_UNITS];
static int diskPosition[USLOSS_DISK_UNITS];
int semDiskQ[USLOSS_DISK_UNITS];

int lineAmount[USLOSS_TERM_UNITS] = {0,0,0,0};
char line [10][MAXLINE];


//process table
struct procSlot procTable[MAXPROC];

int debugflag4 = 1;

static int ClockDriver(char *);
static int DiskDriver(char *);
static int TermDriver(char *);
static int TermReader(char *);
static int TermWriter(char *);



static procPtr SleepList;

void
start3(void)
{
    char	name[128];
    char    termbuf[10];
    char    buf[128];
    int		i;

    //Driver PIDs for later quitting
    int		clockPID;

    int     termDriverPID[USLOSS_TERM_UNITS];
    int     termReaderPID[USLOSS_TERM_UNITS];
    int     termWriterPID[USLOSS_TERM_UNITS];
    int     diskDriverPID[USLOSS_DISK_UNITS];

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
    systemCallVec[SYS_TERMREAD]  = termRead;
    systemCallVec[SYS_TERMWRITE] = termWrite;
    systemCallVec[SYS_DISKSIZE] = diskSize;

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
    semTermWrite = semcreateReal(1);
    semReadLines = semcreateReal(1);
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

    
    for (i = 0; i < USLOSS_DISK_UNITS; i++) {
        sprintf(buf, "%d", i);
        diskDriverPID[i] = fork1(name, DiskDriver, buf, USLOSS_MIN_STACK, 2);
        if (diskDriverPID[i] < 0) {
            USLOSS_Console("start3(): Can't create term driver %d\n", i);
            USLOSS_Halt(1);
        }
    }
    sempReal(semRunning);
    sempReal(semRunning);

    

    

    /*
    * create terminal mailboxes
    */
    int result [] = {-1};
    for (i = 0; i < USLOSS_TERM_UNITS; i++) {
        charInbox[i]  = MboxCreate(0, sizeof(result));
        charOutbox[i] = MboxCreate(0, sizeof(result));
        readMbox[i]   = MboxCreate(10, sizeof(char)*MAXLINE+1);
        writeMbox[i]  = MboxCreate(0, sizeof(char)*MAXLINE+1);
    }


    /*
     * Create terminal device drivers.
     */
    for (i = 0; i < USLOSS_TERM_UNITS; i++) {
        
        sprintf(termbuf, "%d", i);
        sprintf(name, "%s", "Terminal Driver ");
        name[16] = i;
        termDriverPID[i] = fork1(name, TermDriver, termbuf, USLOSS_MIN_STACK, 2);
        if (termDriverPID[i] < 0) {
            USLOSS_Console("start3(): Can't create terminal driver %d\n", i);
            USLOSS_Halt(1);
        }
        sempReal(semRunning);
        

        sprintf(termbuf, "%d", i);
        sprintf(name, "%s", "Terminal Reader ");
        name[16] = i;
        termReaderPID[i] = fork1(name, TermReader, termbuf, USLOSS_MIN_STACK, 2);
        if (termReaderPID[i] < 0) {
            USLOSS_Console("start3(): Can't create terminal reader %d\n", i);
            USLOSS_Halt(1);
        }
        sempReal(semRunning);

        sprintf(termbuf, "%d", i);
        sprintf(name, "%s", "Terminal Writer ");
        name[16] = i;
        termWriterPID[i] = fork1(name, TermWriter, termbuf, USLOSS_MIN_STACK, 2);
        if (termWriterPID[i] < 0) {
            USLOSS_Console("start3(): Can't create terminal writer %d\n", i);
            USLOSS_Halt(1);
        }
        sempReal(semRunning);
        
    }

    

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * I'm assuming kernel-mode versions of the system calls
     * with lower-case first letters.
     */
    pid = spawnReal("start4", start4, NULL, 4 * USLOSS_MIN_STACK, 3);
    pid = waitReal(&status);

    if (debugflag4)
        USLOSS_Console("start3(): test is over, cleaning up drivers\n");

    /*
     * Zap the device drivers
     */
    zap(clockPID);  // clock driver
    join(&status);
   

    //terminal drivers
    int message [] = {-2};
    for (i = 0; i < USLOSS_TERM_UNITS; i++) {
        //termDrivers
        updateTermForQuit(i);
        zap(termDriverPID[i]);
        join(&status);

        //termReaders
        MboxSend(charInbox[i], message, sizeof(message));
        join(&status);

        //termWriters
        MboxSend(writeMbox[i], message, sizeof(message));
        MboxCondSend(charOutbox[i], message, sizeof(message));
        join(&status);

    }
    //disk drivers
    for (i = 0; i < USLOSS_DISK_UNITS; i++) {
        diskArm[i] = -1;
        semvReal(semDiskQ[i]);
        join(&status);
    }

    // eventually, at the end:
    quit(0);
    
}
/*
//disk info arrays and sems
static procPtr diskQ [USLOSS_DISK_UNITS][2];
static int tracks[USLOSS_DISK_UNITS];
static int diskArm[USLOSS_DISK_UNITS];
static int diskPosition[USLOSS_DISK_UNITS];
int semDiskQ[USLOSS_DISK_UNITS];
*/

static int
DiskDriver(char *arg)
{
    int unit = atoi( (char *) arg);     // Unit is passed as arg.

    //get the disk size
    USLOSS_DeviceRequest request;
    request.opr = USLOSS_DISK_TRACKS;
    int size = 0;
    request.reg1 = &size;
    USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &request);
    int status = 0;
    waitDevice(USLOSS_DISK_DEV, unit, &status);
    if (debugflag4)
        USLOSS_Console("    DiskDriver%d(): DiskSize = %d \n", unit, size);

    //initialize trackSize
    tracks[unit] = size;

    //initialize diskArm
    diskArm[unit] = 0;
    diskPosition[unit] = QUEUE1;

    //initialize read and write queues
    diskQ[unit][QUEUE1] = NULL;
    diskQ[unit][QUEUE2] = NULL;

    //initialize semaphores
    semDiskQ[unit] = semcreateReal(0);

    //enable interrupts
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    semvReal(semRunning);

    while(!isZapped() && diskArm[unit] != -1){
        //wait until there is a request generated
        sempReal(semDiskQ[unit]);

        if(diskQ[unit][diskPosition[unit]] == NULL){
            if(diskPosition[unit] == QUEUE1)
                diskPosition[unit] = QUEUE2;
            else
                diskPosition[unit] = QUEUE1;
        }

    }
     

    quit(0);
    return 0;
}


/*
* The argument specifies which terminal the driver is for
* Synchs with parent via semRunning
* enables intterupts
* Whie(not zapped)
*   Wait for next terminal interrupt.
*       (the interrupt can signal both a char for reading and a char for writing)
*   Check if there is a char to be read
*       Send the status register to the charInbox
*   Check if there is a char to be wrote
*       Send the status register to the charOutbox
*/
static int
TermDriver(char *arg){

    int result;
    int unit = atoi( (char *) arg); 
    int status;
    int message [] = {0};

    semvReal(semRunning);
    if (debugflag4)
        USLOSS_Console("    TermDriver%d(): TermDriver %d starting\n", unit+1, unit+1);

    //enable interrupts
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    //call the receive interrupt definition
    long control = 0;
    control = USLOSS_TERM_CTRL_RECV_INT(control);
    //turn receive interrupts on for the terminal unit
    result = USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void *)control);

    while(! isZapped()) {
        result = waitDevice(USLOSS_TERM_DEV, unit, &status);
        /*if (debugflag4)
            USLOSS_Console("    TermDriver%d(): TermDriver %d returned from wait device\n", unit+1, unit+1);*/

        //if WaitDeivce returned 0, process was zapped! Quit
        if (result != 0) {
            quit(0);
            return 0;
        }

        //check to see if a character was received
        if(USLOSS_TERM_STAT_RECV(status) == USLOSS_DEV_BUSY){
            //let TermReader know there is a character to read
            message[0] = status;
            MboxSend(charInbox[unit], message, sizeof(message));
        }
        if(USLOSS_TERM_STAT_XMIT(status) == USLOSS_DEV_READY){
            //let TermWriter know there is a character to write
            message[0] = status;
            
            MboxCondSend(charOutbox[unit], message, sizeof(message));
        }
    }

    if (debugflag4)
        USLOSS_Console("    TermDriver%d(): Zapped! Quitting\n", unit+1);
    

    quit(0);
    return 0;

}
/*
* synchs with start3 after starting via semRunning
* While it isn't zapped
*   Wait  on the charInbox for next char to be sent
*   Build the line buffer
*   Sends completed lines to readMbox
*   
*   ensures readMbox only has a max of 10 lines at a time
*/
static int
TermReader(char *arg){

    int unit = atoi( (char *) arg); 
    int me = getpid();
    int result [] = {-1};
    char temp[88];


    int curLineIndex = 0;
    char newLine[MAXLINE+1];

    procTable[me].privateMbox = MboxCreate(0, sizeof(result));

    //initialize line buff
    int i;
    for (i = 0; i < MAXLINE+1; i++){
        newLine[i] = '\0';
    }


    semvReal(semRunning);
    if (debugflag4)
        USLOSS_Console("    TermReader(): TermReader %d starting\n", unit+1);

    //enable interrupts
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    while(! isZapped()) {
        //wait for next char to arrive
        MboxReceive(charInbox[unit], result, sizeof(result));

        //a result of -2 means that it was zapped!
        if (result[0] == -2){
            if (debugflag4)
                    USLOSS_Console("    TermReader(): TermReader %d  zapped! quitting\n", unit+1);
            quit(0);
            return 0;
        }

        //store character in received variable
        char received = USLOSS_TERM_STAT_CHAR(result[0]);
        if (debugflag4)
            USLOSS_Console("    TermReader(): TermReader %d  received a character: %c\n", unit+1, received);

        //check to see if current line is full first
        if (curLineIndex == MAXLINE){
            //finish off this line with a newline
            newLine[curLineIndex] = '\n';
            //send line to mailbox
            if (lineAmount[unit] <10){
                if (debugflag4)
                    USLOSS_Console("    TermReader(): TermReader %d  sending line!\n", unit+1);
                MboxSend(readMbox[unit], &newLine, MAXLINE+1);
                lineAmount[unit]++;
            }
            else{
                //make room for next line, get rid of oldest line
                MboxReceive(readMbox[unit], &temp, MAXLINE);
                //send new line
                MboxSend(readMbox[unit], &newLine, MAXLINE);
            }

            //clear out the newLine buffer
            curLineIndex = 0;
            for (i = 0; i <= MAXLINE; i++){
                newLine[i] = '\0';
            }
            //write new char in first spot of newLinebuffer
            newLine[curLineIndex] = received;
            curLineIndex++;
        }

        //check to see if its a newline character
        else if(received == '\n'){
            newLine[curLineIndex] = '\n';
            //send line to mailbox
            if (lineAmount[unit] <10){
                if (debugflag4)
                    USLOSS_Console("    TermReader(): TermReader %d  sending line!\n", unit+1);
                MboxSend(readMbox[unit], &newLine, MAXLINE);
                lineAmount[unit]++;
            }
            else{
                if (debugflag4)
                    USLOSS_Console("    TermReader(): TermReader %d  receiving then sending line!\n", unit+1);
                //make room for next line, get rid of oldest line
                MboxReceive(readMbox[unit], &temp, MAXLINE);
                //send new line
                MboxSend(readMbox[unit], &newLine, MAXLINE);

            }

            curLineIndex = 0;
            for (i = 0; i < MAXLINE; i++){
                newLine[i] = '\0';
            }
 
        }
        // if none of the above cases happen, simply write the character
        else{
            newLine[curLineIndex] = received;
            curLineIndex++;
        }

        
    }// end while

    if (debugflag4)
        USLOSS_Console("    TermReader%d(): Zapped! Quitting\n", unit+1);
    
    quit(0);
    return 0;

}
/*
* 
*/
static int
TermWriter(char *arg){

    int unit = atoi( (char *) arg);
    int me = getpid(); 
    int result [] = {-1};
    procTable[me].privateMbox = MboxCreate(0, sizeof(result));


    semvReal(semRunning);
    if (debugflag4)
        USLOSS_Console("    TermWriter(): TermWriter %d starting\n", unit);

    while(! isZapped()) {
        //wait for pid to arrive from termWriteReal
        MboxReceive(writeMbox[unit], result, sizeof(result));

        //result of -2 means that it was zapped!
        if (result[0] == -2){
            if (debugflag4)
                USLOSS_Console("    TermWriter(): TermWriter %d  zapped! quitting\n", unit);
            quit(0);
            return 0;
        }

        //result contains the PID of the process doing the writing.
        int pidToUnblock = result[0];
        if (debugflag4)
            USLOSS_Console("    TermWriter(): TermWriter %d  got pid %d\n", unit, pidToUnblock);
        //get line to write
        char lineToWrite [MAXLINE+1];
        MboxReceive(writeMbox[unit], lineToWrite, MAXLINE+1);
        if (debugflag4)
            USLOSS_Console("TermWriter%d(): got line %s\n", unit, lineToWrite);

        //turn on transmit interrupts
        long control = 0;
        control = USLOSS_TERM_CTRL_XMIT_INT(control);
        //turn receive interrupts on for the terminal unit
        USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void *)control);

        //iterate over each character, write one at a time
        int status;
        int i = 0;
        while (i < MAXLINE && lineToWrite[i] != '\n' ){
            MboxReceive(charOutbox[unit], result, sizeof(result));

            status = result[0];
            if (debugflag4)
                USLOSS_Console("    TermWriter%d(): Received status %d\n", unit, status);
            control = 0;
            control =  USLOSS_TERM_CTRL_CHAR(control, lineToWrite[i]);
            control = USLOSS_TERM_CTRL_XMIT_INT(control);
            control =  USLOSS_TERM_CTRL_XMIT_CHAR(control);

            USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void*)control);

            i++;  
        }

        MboxReceive(charOutbox[unit], result, sizeof(result));

            status = result[0];
            if (debugflag4)
                USLOSS_Console("    TermWriter%d(): Received status %d\n", unit, status);
            control = 0;
            control =  USLOSS_TERM_CTRL_CHAR(control, '\n');
            control = USLOSS_TERM_CTRL_XMIT_INT(control);
            control =  USLOSS_TERM_CTRL_XMIT_CHAR(control);

            USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void*)control);


        //unblock waiting process after lines been written
        MboxSend(procTable[pidToUnblock].privateMbox, NULL, 0);



        
    }// end while

    if (debugflag4)
        USLOSS_Console("    TermWriter%d(): Zapped! Quitting\n", unit);
    
    quit(0);
    return 0;
}

/*
* synchs with start3 via semRunning
* enables interrupts (clock interrupts)
* While(not zapped)
*    Wait for next clock interrupt (wait device call)
*    Wakes up procs on the SleepList that are ready
*/
static int
ClockDriver(char *arg)
{
    int result;
    int status;

    // Let the parent know we are running and enable interrupts.
    semvReal(semRunning);
    if (debugflag4)
        USLOSS_Console("    ClockDriver(): Starting\n");

    //enable interrupts
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);

    // Infinite loop until we are zap'd
    while(! isZapped()) {
        //send to wait device mailbox
	    result = waitDevice(USLOSS_CLOCK_DEV, 0, &status);
        if (debugflag4)
            USLOSS_Console("    ClockDriver(): checking for procs to wake up\n");
            //USLOSS_Console("All work and no play makes Jordan a dull boy\n");
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
    return 0;
}

/*
* interface with Sleep and sleepReal.
* Starts in kernel mode, switches to user before returning
*/
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
    return;
}

/*
* adds process to sleep list
* wakes up and returns when proc is woken up
* Wake up is done when clockDriver sents to the privateMbox of the proc
*/
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

void
diskSize(systemArgs *args){

    int unit = (int)args->arg1;
    if (debugflag4)
        USLOSS_Console("diskSize(): calling diskReadReal\n");
    int sectorSize = 0;
    int trackSize = 0;
    int diskSize = 0;
    diskSizeReal(unit, &sectorSize, &trackSize, &diskSize);
    args->arg1 = (void *) ( (long) sectorSize);
    args->arg2 = (void *) ( (long) trackSize);
    args->arg3 = (void *) ( (long) diskSize);

}

void diskSizeReal(int unit, int *sectorSize, int *trackSize, int *diskSize){
    *sectorSize = USLOSS_DISK_SECTOR_SIZE;
    *trackSize = USLOSS_DISK_TRACK_SIZE;
   
    *diskSize = trackSize[unit];

}

void 
diskWrite(systemArgs *args){

    if (debugflag4)
        USLOSS_Console("diskWrite: extracting arguments\n");

    void *toTransfer = args->arg1;
    int sectorsToWrite = (int)args->arg2;
    int startingTrack = (int) args->arg3;
    int startingSector = (int) args->arg4;
    int unit = (int) args->arg5;

    int status = diskWriteReal(toTransfer, sectorsToWrite, startingTrack, startingSector, unit);
    if(status != 0){
        USLOSS_Console("diskWrite(): problem writing to disk\n");
    }
    args->arg1 = (void *) ( (long) status);


}

int
diskWriteReal(void *toTransfer, int sectorsToWrite, int startingTrack, int startingSector, int unit){


}


/*
* Interfaces with TermRead and termReadReal. 
* Starts in kernel mode, switches to user before returning
*/
void
termRead(systemArgs *args){

    //extract arguments from syargs
    //invalid args were already checked for
    char * buffer = (char*) args->arg1;
    int maxSize = (int) args->arg2;
    int unit = (int) args->arg3;

    int numChars = termReadReal(buffer, maxSize, unit);

    args->arg2 = (void *) ( (long) numChars);

    setToUserMode();
    return;

}
/*
* turns on terminal recieve interrupts
* Receives a line from the readMbox. 
* Copies that line into the buffer via provided address.
* Returns the amount of chars copied into the buffer
*/
int
termReadReal(char* buffer, int maxSize, int unit){

    //recieve a new line from the readMbox to a temp buf
    char tempBuf [MAXLINE];
    if (debugflag4)
        USLOSS_Console("termReadReal(): receiving from readMbox, waiting for new line\n");

    //call the receive interrupt definition
    long control = 0;
    control = USLOSS_TERM_CTRL_RECV_INT(control);

    //turn receive interrupts on for the terminal unit
    USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void *)control);

    MboxReceive(readMbox[unit], tempBuf, MAXLINE);
    lineAmount[unit]--;
    if (debugflag4)
        USLOSS_Console("termReadReal(): got tempbuf: %s\n", tempBuf);

    //just in case the buffer provided wasn't big enough for the whole line
    int i = 0;
    while(tempBuf[i] != '\0' && i < maxSize){
        buffer[i] = tempBuf[i];
        i++;
    }
    //return the amount of characters read
    return i;
}
/*
* Interfaces with TermWrite and termWriteReal
* Handles arguments
*/
void 
termWrite(systemArgs *args){

    //extract arguments from syargs
    //invalid args were already checked for
    char * lineToWrite = (char*) args->arg1;
    int lineSize = (int) args->arg2;
    int unit = (int) args->arg3;
    if (debugflag4)
        USLOSS_Console("termWrite(): calling termWriteReal\n");


    int numCharsWritten = termWriteReal(lineToWrite, lineSize, unit);


    args->arg2 = (void *) ( (long) numCharsWritten);

    setToUserMode();

    return;

}

int 
termWriteReal(char* lineToWrite, int lineSize, int unit){
    //get pid
    int me = getpid();
    //setup private mbox
    if (procTable[me].privateMbox == -1){
        procTable[me].privateMbox = MboxCreate(0, 50);
    }
    //send pid to writeMbox
    int message [] = {me};
    if (debugflag4)
        USLOSS_Console("termWriteReal(): sending pid to term %d\n", unit);
    //semaphore to ensure the terminal gets the pid and the line of the same proc.
    sempReal(semTermWrite);
    MboxSend(writeMbox[unit], message, sizeof(message));
    if (debugflag4)
        USLOSS_Console("termWriteReal(): sending line to term %d\n", unit);
    //send line to write to terminal
    MboxSend(writeMbox[unit], lineToWrite, lineSize);
    if (debugflag4)
        USLOSS_Console("termWriteReal(): blocking\n");
    //bloxk until message is completely written
    semvReal(semTermWrite);

    MboxReceive(procTable[me].privateMbox, NULL, 0);
    if (debugflag4)
        USLOSS_Console("termWriteReal(): done\n");


    return lineSize;
}

/*
* utility function, will halt if kernel mode is called
*/
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

/*
* utility function to be called one time in start3
* removed it from start3 because it was very long
*/
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

/*
* get a process table slot ready for bed
*/
void setUpSleepSlot(int pid){
    procTable[pid].pid = pid;
    procTable[pid].status = SLEEPING;
    procTable[pid].privateMbox = MboxCreate(0,50);
}

/*
* wakes up the proc at the front of the SleepList
* updates the SleepList
*/
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

/*
* changes simulation back to user mode
*/
void setToUserMode(){
    unsigned int psr = USLOSS_PsrGet();
    psr = psr >> 1;
    psr = psr << 1;
    USLOSS_PsrSet(psr);
}
/*
* adds to sleep list, and maintains the ordered queue
*/
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
/*
* Appends a new line to the term.in file for the specified terminal
*/
void
updateTermForQuit(int unit){
    FILE *filePtr;
    char fileNameBuf[20];
    sprintf(fileNameBuf, "%s%d%s", "term", unit, ".in");
    if (debugflag4)
        USLOSS_Console("updateTermForQuit(): build string for file name: %s\n",fileNameBuf);
    //open the file, a means we are going to be appending
    filePtr = fopen (fileNameBuf, "a");
    int result = fprintf(filePtr, "End of USLOSS Simulation\n");
    if (result <= 0){
        USLOSS_Console("updateTermForQuit(): error writting to  file %s\n", fileNameBuf);

    }
    fclose(filePtr);
}
