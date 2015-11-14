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

int TermWrite(char *buffer, int bufferSize, int unitID,
                       int *numCharsRead){
	//check for illegal arguments
	if (buffer == 0 || bufferSize <= 0 || unitID < 0){
	 	if (debugflaglib4)
			USLOSS_Console("TermRead(): invalid arguments! returning\n");
		return -1;
	}

	 //build sysarg structure to pass to the syscall vec
	systemArgs sysArg;

	CHECKMODE;
	sysArg.number = SYS_TERMWRITE;
	sysArg.arg1 = buffer;
	sysArg.arg2 = (void *) ( (long) bufferSize);
	sysArg.arg3 = (void *) ( (long) unitID);

	if (debugflaglib4)
		USLOSS_Console("TermWrite(): sysarg built, calling sysvec function\n");
	USLOSS_Syscall(&sysArg);

	int bytesWritten = (int ) ((void*) sysArg.arg2);
	*numCharsRead = bytesWritten;

	//return (int ) ((void*) sysArg.arg2);

	return 0;
}

int TermRead (char *buffer, int bufferSize, int unitID,
                       int *numCharsRead){
	
	//check for illegal arguments
	if (buffer == 0 || bufferSize <= 0 || unitID < 0){
	 	if (debugflaglib4)
			USLOSS_Console("TermRead(): invalid arguments! returning\n");
		return -1;
	}

	 //build sysarg structure to pass to the syscall vec
	systemArgs sysArg;

	CHECKMODE;
	sysArg.number = SYS_TERMREAD;
	sysArg.arg1 = buffer;
	sysArg.arg2 = (void *) ( (long) bufferSize);
	sysArg.arg3 = (void *) ( (long) unitID);

	if (debugflaglib4)
		USLOSS_Console("TermRead(): sysarg built, calling sysvec function\n");
	USLOSS_Syscall(&sysArg);

	int bytesRead = (int ) ((void*) sysArg.arg2);
	*numCharsRead = bytesRead;

	//return (int ) ((void*) sysArg.arg2);

	return 0;
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
	*status = 0;
	//once it wakes up, return arg1
	return (int) sysArg.arg1;
}

int  DiskWrite(void *diskBuffer, int unit, int track, int first,
                       int sectors, int *status){
	//just checks for obvious illegal values for now, adjust as needed
	if (unit < 0 || track < 0 || first < 0 || sectors < 0){
		if (debugflaglib4)
			USLOSS_Console("DiskRead(): illegal value(s) given, returning -1\n");
		return -1;
	}

	//build sysarg structure to pass to the syscall vec
	systemArgs sysArg;

	CHECKMODE;
	sysArg.number = SYS_DISKWRITE;
	sysArg.arg1 =  diskBuffer;
	sysArg.arg2 = (void *) ( (long) sectors);
	sysArg.arg3 = (void *) ( (long) track);
	sysArg.arg4 = (void *) ( (long) first);
	sysArg.arg5 = (void *) ( (long) unit);
	USLOSS_Syscall(&sysArg);
	*status = 0;
	//once it wakes up, return arg1
	return (int) sysArg.arg1; 
}

int  DiskSize (int unit, int *sector, int *track, int *disk){
	if (unit < 0){
		if (debugflaglib4)
			USLOSS_Console("DiskSize(): illegal value(s) given, returning -1\n");
		return -1;
	}
	systemArgs sysArg;

	CHECKMODE;
	sysArg.number = SYS_DISKSIZE;
	sysArg.arg1 = (void *) ( (long) unit);
	USLOSS_Syscall(&sysArg);
	*sector = (int)sysArg.arg1;
	*track = (int)sysArg.arg2;
	*disk = (int)sysArg.arg3; 

	return 0;
}



