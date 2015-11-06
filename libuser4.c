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

 int debugflaglib4 = 0;

#define CHECKMODE {						\
	if (USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) { 				\
	    USLOSS_Console("Trying to invoke syscall from kernel\n");	\
	    USLOSS_Halt(1);						\
	}						\
}

int  Sleep(int seconds){

	if (seconds <= 0){
		if (debugflaglib4)
			USLOSS_Console("Sleep(): secondsToSleep <= 0, returning -1\n");
		return -1;
	}

	//build sysarg structure to pass to the syscall vec
	systemArgs sysArg;

	CHECKMODE;
	sysArg.number = SYS_SLEEP;
	sysArg.arg1 =  (void *) ( (long) seconds);
	if (debugflaglib4)
		USLOSS_Console("Sleep(): build sysArg, sending to sleep\n");
	USLOSS_Syscall(&sysArg);
	//once it wakes up, just return
	return 0;

}

int  DiskRead (void *diskBuffer, int unit, int track, int first, 
                       int sectors, int *status){
	if (debugflaglib4)
		USLOSS_Console("DiskRead(): At beginning\n");

	//just checks for obvious illegal values for now, adjust as needed
	if (unit < 0 || track < 0 || first < 0 || sectors < 0){
		if (debugflaglib4)
			USLOSS_Console("DiskRead(): illegal value(s) given, returning -1\n");
		return -1;
	}

	//build sysarg structure to pass to the syscall vec
	systemArgs sysArg;

	CHECKMODE;
	sysArg.number = SYS_DISKREAD;
	sysArg.arg1 =  diskBuffer;
	sysArg.arg2 = (void *) ( (long) sectors);
	sysArg.arg3 = (void *) ( (long) track);
	sysArg.arg4 = (void *) ( (long) first);
	sysArg.arg5 = (void *) ( (long) unit);
	USLOSS_Syscall(&sysArg);
	//once it wakes up, return arg1
	return (int) sysArg.arg1;
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