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
extern  void updateTermForQuit(int unit);

//interface with system calls
extern  void sleep(systemArgs *args);
extern  int  sleepReal(int seconds);

extern  void termRead(systemArgs *args);
extern  int termReadReal(char* buffer, int maxSize, int unit);

extern  void termWrite(systemArgs *args);
extern  int  termWriteReal(char* lineToWrite, int lineSize, int unit);

extern  void diskSize(systemArgs *args);
extern  void diskSizeReal(int unit, int *sectorSize, int *trackSize, int *diskSize);

extern  void diskWrite(systemArgs *args);
extern  int  diskWriteReal(char *toTransfer, int sectorsToWrite, int startingTrack, int startingSector, int unit);

extern  void diskRead(systemArgs *args);
extern  int  diskReadReal(char *toTransfer, int sectorsToRead, int startingTrack, int startingSector, int unit);

extern  int insertDiskRequest(int pid, int unit, int totalSectors, int startingSector);
extern  void addToDiskQ(procPtr toAdd, int unit, int position);


#define ERR_INVALID             -1
#define ERR_OK                  0

//proc table definitions
#define JOIN_BLOCKED 1
#define READY        2
#define WAIT_BLOCKED 3
#define SLEEPING     4

#define QUEUE1  0
#define QUEUE2  1



struct procSlot {
	int			pid;
	char*		name;
	int			status;
	int			priority;
	procPtr		nextProc;
	int			privateMbox;
	int			termCode;
	int 		timeToWakeUp;
	USLOSS_DeviceRequest req;	
	int 		startingTrack;
	int         curTrack;
	int 		totalSectors;



};

#endif /* _PHASE4_H */
