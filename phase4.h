/*
 * These are the definitions for phase 4 of the project (support level, part 2).
 */

#ifndef _PHASE4_H
#define _PHASE4_H

/*
 * Maximum line length
 */

#define MAXLINE         80


//define this before function prototypes because its used in a few of them
typedef struct procSlot *procPtr;

/*
 * Function prototypes for this phase.
 */


extern  int  Sleep(int seconds);
extern  int  DiskRead (void *diskBuffer, int unit, int track, int first, 
                       int sectors, int *status);
extern  int  DiskWrite(void *diskBuffer, int unit, int track, int first,
                       int sectors, int *status);
extern  int  DiskSize (int unit, int *sector, int *track, int *disk);
extern  int  TermRead (char *buffer, int bufferSize, int unitID,
                       int *numCharsRead);
extern  int  TermWrite(char *buffer, int bufferSize, int unitID,
                       int *numCharsRead);

//utilities
extern  int  start4(char *);
extern  int  inKernelMode(char *procName);
extern  void initializeProcTable();
extern  void setUpSleepSlot(int pid);
extern  void setToUserMode();
extern  void addToSleepList(procPtr toAdd);
extern  void wakeUpNextProc();

//interface with system calls
extern  void sleep(systemArgs *args);
extern  int  sleepReal(int seconds);
extern  void termRead(systemArgs *args);

#define ERR_INVALID             -1
#define ERR_OK                  0

//proc table definitions
#define JOIN_BLOCKED 1;
#define READY        2;
#define WAIT_BLOCKED 3;
#define SLEEPING     4;



struct procSlot {
	int			pid;
	char*		name;
	int			status;
	int			priority;
	procPtr		nextProc;
	int			privateMbox;
	int			termCode;
	int 		timeToWakeUp;


};

#endif /* _PHASE4_H */
