#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include "phase4.h"
#include "provided_prototypes.h"
#include <stdlib.h> /* needed for atoi() */
#include <stdio.h>
#include <string.h>

int semRunning;

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

//utility function, will halt if kernel mode is called
int inKernelMode(char *procName)
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
