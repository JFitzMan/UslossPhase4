/*
 *  File:  libuser4.c
 *
 *  Description:  Contains functions to build sysArgs struct
 *                to send to the real version of the functions
 */

#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <libuser.h>
#include <usyscall.h>
#include <usloss.h>

 int debugflaglib4 = 1;

#define CHECKMODE {						\
	if (USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) { 				\
	    USLOSS_Console("Trying to invoke syscall from kernel\n");	\
	    USLOSS_Halt(1);						\
	}						\
}

int  Sleep(int seconds){
	if (debugflaglib4)
		USLOSS_Console("Sleep(): At beginning\n");
	return 0;
}

int  DiskRead (void *diskBuffer, int unit, int track, int first, 
                       int sectors, int *status){
	return 0;
}

int  DiskWrite(void *diskBuffer, int unit, int track, int first,
                       int sectors, int *status){
	return 0;
}

int  DiskSize (int unit, int *sector, int *track, int *disk){
	return 0;
}

int TermRead (char *buffer, int bufferSize, int unitID,
                       int *numCharsRead){
	return 0;
}

int TermWrite(char *buffer, int bufferSize, int unitID,
                       int *numCharsRead){
	return 0;
}