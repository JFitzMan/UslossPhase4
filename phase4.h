/*
 * These are the definitions for phase 4 of the project (support level, part 2).
 */

#ifndef _PHASE4_H
#define _PHASE4_H

/*
 * Maximum line length
 */

#define MAXLINE         80

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

extern  int  start4(char *);
extern  int  inKernelMode(char *procName);
extern  void initializeProcTable();

extern  void sleep(systemArgs *args);

#define ERR_INVALID             -1
#define ERR_OK                  0

#define JOIN_BLOCKED 1;
#define READY        2;
#define WAIT_BLOCKED 3;

typedef struct procSlot *procPtr;

struct procSlot {
	int			pid;
	int			parentPid;
	int 		(* start_func) (char *);
	char*		name;
	int			status;
	char*		arg;
	int			stack_size;
	int			priority;
	procPtr		nextChild;
	procPtr		nextSib;
	int			privateMbox;
	int			termCode;

};

#endif /* _PHASE4_H */
