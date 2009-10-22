/*
 * SQLOutliner
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Reads org-mode files using LibOrgParser and allow SQL queries to be run against them
 *
 * This isn't a terribly good example. But a somewhat useful program none the less.
 * See simple_example for usage example.
 */

#include "sqlite3.h"
#include "../liborgparser/orgparser.h"
#include <time.h>


/* HAH, this one is gross */
#define SQLAPPEND(text, ...) {\
	char temp[64];\
	if (strlen(builtquery) > 0)\
		strncat(builtquery, "AND ", sizeof(builtquery));\
	if (negate) {\
		strncat(builtquery, "NOT ", sizeof(builtquery));\
		negate = 0;\
	}\
	sprintf(temp, text, ## __VA_ARGS__);\
	strncat(builtquery, temp, sizeof(builtquery)); }


/* Global variables */
sqlite3 *db = NULL;
char *zErrMsg = 0;
int parseable_format = 0; /* "Human readable" by default */
int negate = 0;


/* This function basically takes the result of parsing a single "heading" and stores it in the DB for later */
int add_new(int id, int parent, char *heading, char *bodytext, time_t deadline, time_t closed, time_t scheduled, int level, char *tags)
{
	char *query = sqlite3_mprintf("INSERT INTO file VALUES (%d, %d, %Q, %Q, %lu, %lu, %lu, %d, %Q)", id, parent, heading, bodytext, deadline, closed, scheduled, level, tags);

	int rc = sqlite3_exec(db, query, NULL, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error inserting into temporary database: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	sqlite3_free(query);
}


/* Takes a string like "1d10h" and turns it into seconds */
static long parse_reltime(char *buffer)
{
	long amount = 0;
	char temp[8];
	memset(temp, 0, sizeof(temp));

	int i, num, neg = 0;

	for (i = 0; i < strlen(buffer); i++) {
		switch (buffer[i]) {
			case 's': /* second */
				num = atoi(temp);
				amount += num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'm': /* minute */
				num = atoi(temp);
				amount += 60 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'h': /* hour */
				num = atoi(temp);
				amount += 60 * 60 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'd': /* day */
				num = atoi(temp);
				amount += 60 * 60 * 24 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'M': /* month */
				num = atoi(temp);
				amount += 60 * 60 * 24 * 30 * num; /* A month is 30 days okay. */
				memset(temp, 0, sizeof(temp));
				break;

			case 'y': /* year */
				num = atoi(temp);
				amount += 60 * 60 * 24 * 365 * num; /* Close enough */
				memset(temp, 0, sizeof(temp));
				break;

			case '-': /* Make negative */
				neg = 1;
				break;

			default: /* Better be a number so help you */
				/* TODO - Check it's actually a number */
				temp[strlen(temp)] = buffer[i];
				break;
		}
	}

	if (neg) {
		return -amount;
	} else {
		return amount;
	}
}


/* SQLite callback for printing, should probably be changed */
static int sqlite_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;

	if (parseable_format) {
		for (i = 0; i < argc; i++) {
			int c;
			for (c = 0; c < strlen(argv[i]); c++) {
				if (argv[i][c] == '\n') {
					argv[i][c] = ';';
				}
			}

			printf("%s", argv[i]);
			if (i + 1 < argc) {
				printf("|");
			}
		}

		printf("\n");
	} else {
		for (i = 0; i < argc; i++) {
			printf("%-20s %s\n", azColName[i], argv[i]);
		}

		printf("\n\n");
	}

	return 0;
}


/* This just might be the main */
int main(int argc, char **argv)
{
	int rc = 0;


	/*
	 * Check length of options (need at least a file)
	 */

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [options] <file1> [file2] .. [filen]\n", argv[0]);
		fprintf(stderr, "Written by Zane Ashby.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options are one or many of:\n");
		fprintf(stderr, "-r <sql-query>\t- Raw SQL query\n");
		fprintf(stderr, "-o <columns>\t- Columns to print\n");
		fprintf(stderr, "-h <headline>\t- Match headline\n");
		fprintf(stderr, "-b <bodytext>\t- Match body text\n");
		fprintf(stderr, "-t <tag>\t- Match tag\n");
		fprintf(stderr, "-d <time>\t- Deadline within time offset\n");
		fprintf(stderr, "-s <time>\t- Scheduled within time offset\n");
		fprintf(stderr, "-c <time>\t- Closed within time offset\n");
		fprintf(stderr, "-w <file>\t- Write database to disk\n");
		fprintf(stderr, "-n\t\t- Negate next option on command-line\n");
		fprintf(stderr, "-p\t\t- Output in an easily parseable format\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Time offset is:\n");
		fprintf(stderr, "[-]<#><smhdMy>, eg. 1d3h\n");
		goto cleanup;
	}


	/*
	 * Parse the command line
	 */

	int i;

	char rawquery[256];
	memset(rawquery, 0, sizeof(rawquery));

	char columns[64];
	memset(columns, 0, sizeof(columns));

	char builtquery[256];
	memset(builtquery, 0, sizeof(builtquery));

	char disk_path[32];
	memset(disk_path, 0, sizeof(disk_path));

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') { /* Switch? I sure hope so */
			switch (argv[i][1]) {
				case 'r': /* Raw Query */
					strncpy(rawquery, argv[++i], sizeof(rawquery));
					break;

				case 'o': /* Output */
					strncpy(columns, argv[++i], sizeof(columns));
					break;

				case 'h': /* Headline search */
					SQLAPPEND("heading LIKE '%%%s%%' ", argv[++i]);
					break;

				case 'b': /* Bodytext search */
					SQLAPPEND("bodytext LIKE '%%%s%%' ", argv[++i]);
					break;

				case 't': /* Tag search */
					SQLAPPEND("tags LIKE '%%:%s:%%' ", argv[++i]);
					break;

				case 'd': /* Deadline search */
					SQLAPPEND("deadline BETWEEN strftime('%%s', 'now') AND strftime('%%s', 'now') + %ld ", parse_reltime(argv[++i]));
					break;

				case 's': /* Scheduled search */
					SQLAPPEND("scheduled BETWEEN strftime('%%s', 'now') AND strftime('%%s', 'now') + %ld ", parse_reltime(argv[++i]));
					break;

				case 'c': /* Closed search */
					SQLAPPEND("closed BETWEEN strftime('%%s', 'now') AND strftime('%%s', 'now') + %ld ", parse_reltime(argv[++i]));
					break;

				case 'w': /* Write to disk */
					strncpy(disk_path, argv[++i], sizeof(disk_path));
					break;

				case 'n': /* Negate */
					negate = 1; /* Should negate next append */
					break;

				case 'p': /* Enable parseable format */
					parseable_format = 1;
					break;
			}
		} else {
			break;
		}
	}


	/*
	 * Create database
	 */

	if (strlen(disk_path) > 0) {
		rc = sqlite3_open(disk_path, &db);
	} else {
		rc = sqlite3_open(":memory:", &db);
	}

	if (rc) {
		fprintf(stderr, "Error creating DB: %s\n", zErrMsg);
		goto cleanup;
	}


	/*
	 * Create table
	 */

	rc = sqlite3_exec(db, "CREATE TABLE file (id INTEGER PRIMARY KEY, parent INTEGER, heading TEXT, bodytext TEXT, deadline INTEGER, closed INTEGER, scheduled INTEGER, level INTEGER, tags TEXT)", NULL, 0, &zErrMsg);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error creating table: %s\n", zErrMsg);
		goto cleanup;
	}


	/*
	 * Parse each file specified on the command line
	 */

	for (; i < argc; i++) { /* Rest of argv better be files */
		parse_org_file(argv[i], add_new);
	}


	/*
	 * Execute SQL query given from command line
	 */

	if (strlen(rawquery) > 0) {
		rc = sqlite3_exec(db, rawquery, sqlite_callback, 0, &zErrMsg);
	} else {
		char final[256];
		if (strlen(builtquery) > 0) {
			if (strlen(columns) > 0) {
				sprintf(final, "SELECT %s FROM file WHERE %s", columns, builtquery);
			} else {
				sprintf(final, "SELECT * FROM file WHERE %s", builtquery);
			}
		} else {
			if (strlen(columns) > 0) {
				sprintf(final, "SELECT %s FROM file", columns);
			} else {
				sprintf(final, "SELECT * FROM file");
			}
		}
		rc = sqlite3_exec(db, final, sqlite_callback, 0, &zErrMsg);
	}
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error in SQL query: %s\n", zErrMsg);
		goto cleanup;
	}


cleanup:
	if (zErrMsg)
		sqlite3_free(zErrMsg);

	if (db)
		sqlite3_close(db);

	return 0;
}
