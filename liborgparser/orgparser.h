/*
 * LibOrgParser
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Parses org-mode files and passes info off to a user defined callback
 */

#ifndef __LIBORGPARSER_H
#define __LIBORGPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_DEPTH		10
#define MAX_LINE		512
#define MAX_HEADING		256
#define MAX_TAGS		64
#define MAX_BODY		1024

#define OP_TYPE_UNKNOWN		0
#define OP_TYPE_ORG		1
#define OP_TYPE_VIMOUTLINER	2

typedef struct {
	FILE *fp;
	int type;
} OPFILE;

typedef struct {
	int id;
	int parent_id;
	int level;
	time_t deadline;
	time_t closed;
	time_t scheduled;
	char heading[MAX_HEADING];
	char tags[MAX_TAGS];
	char body[MAX_BODY];
} OPTASK;

typedef void (*OPCALLBACK)(OPTASK, void*);

long OP_parse_reltime(char *buffer);
OPFILE* OP_open(char *path);
void OP_close(OPFILE *file);
int OP_read_task(OPFILE *file, OPCALLBACK callback, void *userData);
int OP_write_task(OPFILE *file, OPTASK task);

#endif
