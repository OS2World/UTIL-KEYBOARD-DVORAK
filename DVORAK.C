#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dvorak.h"

static VOID PrintText(CHAR **ppcText);
static SHORT ProcessFile(FILE *fp, ULONG ulIndex);
static SHORT ProcessEntry(FILE *fp, IndexEntry *oEntry);
static SHORT Assignments(KeyDef *oOrgTable, KeyDef *oNewTable, INT iEntryCnt);

static CHAR *pcStartUp[] =
{
	"DVORAK -- This program will change your OS/2 DCP file to use a Dvorak keyboard",
	"configuration as your default. To change back from the Dvorak keyboard",
	"please type 'keyb XXX' where XXX is another country that is not 'US'.",
	"",
	"Note: This program was written by Andrew Stern and tested on the file",
	"keyboard.dcp 4/26/93.  I wrote this program because I was told that IBM had no",
	"support for a Dvorak keyboard in their operating system. Hopefully they will",
	"correct this oversight in a future release.",
	"(C) 1994 By Andrew Stern",
	"Additional credit goes to Ned Konz who wrote SWAPDCP.",
	"Please feel free to distribute and modify anything you want as long as you",
	"include this source file unmodified with your distribution.",
	"Andrew Stern at CS: 71022,3257",
	0
};

/*	"DEVINFO=KBD,US,C:\\OS2\\KEYBOARD.DCP */

static CHAR *cpHelp[] =
{
	"Usage: DVORAK filename.DCP",
	0
};

static SHORT sAssignKey[MAX_ASSIGN][2] = {12, 26, 13, 27, 16, 40, 17, 51, 18, 52, 19, 25, 20, 21, 21, 33, 22, 34,
 23, 46, 24, 19, 25, 38, 26, 53, 27, 13, 31, 24, 32, 18, 33, 22, 34, 23,
 35, 32, 36, 35, 37, 20, 38, 49, 39, 31, 40, 12, 44, 39, 45, 16, 46, 36,
 47, 37, 48, 45, 49, 48, 51, 17, 52, 47, 53, 44};

INT main(INT argc, CHAR *argv[])
{
	CHAR	*pcFileName;
	FILE	*fp;
	ULONG	ulIndex;
	SHORT	sRtn;

	PrintText(pcStartUp);

	if (argc != 2) {
		PrintText(cpHelp);
		return(1);
	}

	pcFileName = *++argv;
	if ((fp = fopen(pcFileName, "r+b")) == NULL) {
		printf("ERROR: Could not open file %s.\n", pcFileName);
		return(1);
	}

	/* Read location of index */
	ulIndex = 0L;
	fread(&ulIndex, sizeof(ulIndex), 1, fp);
	sRtn = ProcessFile(fp, ulIndex);

	fclose(fp);
	return(sRtn);
}

static VOID PrintText(CHAR **ppcText)
{
	CHAR **ppcStr;

	for (ppcStr = ppcText; *ppcStr; ppcStr++) {
		printf("%s\n", *ppcStr);
	}
}

static SHORT ProcessFile(FILE *fp, ULONG ulIndex)
{
	WORD			wNumEntries;
	WORD			wEntryIndex;
	IndexEntry	*oEntry;
	INT			wNumRead;
	SHORT			sRtn;

	fseek(fp, (ULONG) ulIndex, SEEK_SET);
	wNumEntries = 0;
	fread(&wNumEntries, sizeof(wNumEntries), 1, fp);

	oEntry = calloc(wNumEntries, sizeof(IndexEntry));
	if (oEntry == NULL) {
		printf("ERROR: Unable to create memory for index of entries.\n");
		return(1);
	}

	if ((wNumRead = fread(oEntry, sizeof(IndexEntry), wNumEntries, fp)) != wNumEntries) {
		printf("ERROR: Unable to read all the entry tables.\n");
		return(1);
	}

	sRtn = 0;
	for (wEntryIndex = 0; wEntryIndex < wNumEntries; wEntryIndex++) {
		if (oEntry[wEntryIndex].HeaderLocation) {
			sRtn |= ProcessEntry(fp, oEntry + wEntryIndex);
		}
	}

	free(oEntry);
	return(sRtn);
}

static SHORT ProcessEntry(FILE *fp, IndexEntry *oEntry)
{
	LONG		lTablePosition;
	XHeader	*oHeader;
	VOID		*oOrgKeys;
	VOID		*oNewKeys;
	SHORT		sRtn;

	oHeader = calloc(sizeof(XHeader), 1);
	if (oHeader == NULL) {
		printf("ERROR: Unable to create memory for entry's header.\n");
		return(1);
	}

	fseek(fp, (LONG) oEntry->HeaderLocation, SEEK_SET);
	if (fread(oHeader, sizeof(XHeader), 1, fp) != 1)  {
		printf("ERROR: can't read table header.\n" );
		return(1);
	}

	if ((oHeader->Country[0] == 'S') && (oHeader->Country[1] == 'U')) {
		lTablePosition = ftell(fp);

		oOrgKeys = calloc(oHeader->XtableLen, 1);
		oNewKeys = calloc(oHeader->XtableLen, 1);

		if ((oOrgKeys == NULL) || (oNewKeys == NULL)) {
			printf("ERROR: Unable to create memory for entry's keys.\n");
			return(1);
		}

		if (fread(oOrgKeys, oHeader->XtableLen, 1, fp) != 1) {
			printf("ERROR: Can't read key table.\n" );
			return(1);
		}
		memcpy(oNewKeys, oOrgKeys, oHeader->XtableLen);

		sRtn = Assignments((KeyDef *) oOrgKeys, (KeyDef *) oNewKeys, oHeader->EntryCount);

		fseek(fp, lTablePosition, SEEK_SET);
		if (fwrite(oNewKeys, oHeader->XtableLen, 1, fp) != 1) {
			printf("ERROR: Can't write key table.\n" );
			return(1);
		}

		free(oHeader);
		free(oOrgKeys);
		free(oNewKeys);

		return(sRtn);
	} else {
		return(0);
	}
}

static SHORT Assignments(KeyDef *oOrgKeys, KeyDef *oNewKeys, INT iEntryCnt)
{
	SHORT sNumberOfAssignments;
	SHORT sKeyAssignIndex;
	SHORT sKeyOrg;
	SHORT sKeyNew;
	SHORT	sRtn;

	sRtn = 0;
	sNumberOfAssignments = (sizeof(sAssignKey) / sizeof(SHORT)) >> 1;

	for (sKeyAssignIndex = 0; sKeyAssignIndex < sNumberOfAssignments; sKeyAssignIndex++) {
		sKeyNew = sAssignKey[sKeyAssignIndex][0] - 1;
		sKeyOrg = sAssignKey[sKeyAssignIndex][1] - 1;

		if ((sKeyOrg < iEntryCnt) && (sKeyNew < iEntryCnt) && (sKeyOrg > 0) && (sKeyNew > 0)) {
			oNewKeys[sKeyNew] = oOrgKeys[sKeyOrg];
		}
	}
	
	return(sRtn);
}

