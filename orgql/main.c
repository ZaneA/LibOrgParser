//
// OrgQL
// Written by Zane Ashby (zane.a@demonastery.org)
//
// Reads org-mode files using LibOrgParser and allow SQL queries to be run against them
//
// TODO
// Make this a library and binary
// Output plugins?
//

#include "sqlite3.h"
#include "../liborgparser/orgparser.h"
#include <time.h>

#include <windows.h>

#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"


char *output = NULL;
char *current_file = NULL;


static int OP_parse_reltime_wrapper(lua_State *L)
{
	lua_pushnumber(L, OP_parse_reltime((char*)luaL_checkstring(L, 1)));
	return 1;
}

static int parse_query(char *input, char **filelist, char **query, char **output)
{
	lua_State *L = luaL_newstate();

	static const luaL_reg oql_func[] = {
		{ "parseTime", OP_parse_reltime_wrapper },
		{ NULL, NULL }
	};

	luaL_register(L, "oql", oql_func);

	luaopen_string(L);
	luaopen_table(L);

	if (luaL_dofile(L, "oql.lua") != 0) {
		fprintf(stderr, "Error parsing Lua script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);

		return 1;
	}

	lua_getglobal(L, "parse");

	lua_pushstring(L, input);
	if (lua_pcall(L, 1, 3, 0) != 0) {
		fprintf(stderr, "Error calling Lua script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);

		return 1;
	}

	*output = (char*)strdup((char*)lua_tostring(L, -1));
	lua_pop(L, 1);

	*filelist = (char*)strdup((char*)lua_tostring(L, -1));
	lua_pop(L, 1);

	*query = (char*)strdup((char*)lua_tostring(L, -1));
	lua_pop(L, 1);

	lua_close(L);

	return 0;
}


// This function takes a single "task" and stores it in the DB for later
static void add_new(OPTASK task, void *userData)
{
	sqlite3 *db = userData;

	char *query = sqlite3_mprintf("INSERT INTO todo VALUES (NULL, %Q, %d, %d, %Q, %Q, %lu, %lu, %lu, %d, %Q)",
		current_file, task.id, task.parent_id, task.heading, task.body, task.deadline, task.closed, task.scheduled, task.level, task.tags);

	char *zErrMsg = NULL;
	int rc = sqlite3_exec(db, query, NULL, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error inserting into temporary database: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	sqlite3_free(query);
}


// SQLite callback for printing, should probably be changed
static int sqlite_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;

	if (output && !stricmp(output, "CSV")) {
		for (i = 0; i < argc; i++) {
			int c;
			for (c = 0; c < strlen(argv[i]); c++) {
				if (argv[i][c] == '\n') {
					argv[i][c] = ' ';
				}
				if (argv[i][c] == ',') {
					argv[i][c] = '.';
				}
			}

			printf("%s", argv[i]);
			if (i + 1 < argc) {
				printf(",");
			}
		}

		printf("\n");
	} else {
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

#define SetColor(color) SetConsoleTextAttribute(hConsole, color)
#define COLOR_NORMAL 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COLOR_ID 	FOREGROUND_BLUE
#define COLOR_HEADING 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_BODYTEXT 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COLOR_EXTRA_H 	FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_EXTRA_B 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY

#define IFFINDCOL(NAME) for (int i = 0; i < argc; i++)\
				if (!strcmp(azColName[i], NAME))

		SetColor(COLOR_ID);
		IFFINDCOL("id")
			printf("%-4i", atoi(argv[i]));

		SetColor(COLOR_HEADING);
		IFFINDCOL("heading")
			printf("%s\n", argv[i]);

		SetColor(COLOR_BODYTEXT);
		IFFINDCOL("bodytext") {
			if (strlen(argv[i]) > 0)
				printf("\"%s\"\n", argv[i]);
		}

		IFFINDCOL("tags") {
			if (strlen(argv[i]) > 0) {
				SetColor(COLOR_EXTRA_H);
				printf("( Tags ) ");
				SetColor(COLOR_EXTRA_B);
				printf("%s\n", argv[i]);
			}
		}

		IFFINDCOL("deadline") {
			if (atol(argv[i]) != 0) {
				SetColor(COLOR_EXTRA_H);
				time_t time = atol(argv[i]);
				printf("( Deadline ) ");
				SetColor(COLOR_EXTRA_B);
				printf("%s\n", ctime(&time));
			}
		}

		IFFINDCOL("closed") {
			if (atol(argv[i]) != 0) {
				SetColor(COLOR_EXTRA_H);
				time_t time = atol(argv[i]);
				printf("( Closed ) ");
				SetColor(COLOR_EXTRA_B);
				printf("%s\n", ctime(&time));
			}
		}

		IFFINDCOL("scheduled") {
			if (atol(argv[i]) != 0) {
				SetColor(COLOR_EXTRA_H);
				time_t time = atol(argv[i]);
				printf("( Scheduled ) ");
				SetColor(COLOR_EXTRA_B);
				printf("%s\n", ctime(&time));
			}
		}

		SetColor(COLOR_NORMAL);
	}

	return 0;
}

static int sqlite_write_callback(void *outfile, int argc, char **argv, char **azColName)
{
	OPTASK task = { 0 };

	task.id = atoi(argv[2]);
	task.parent_id = atoi(argv[3]);
	sprintf(task.heading, "%s", argv[4]);
	sprintf(task.body, "%s", argv[5]);
	task.deadline = atoi(argv[6]);
	task.closed = atoi(argv[7]);
	task.scheduled = atoi(argv[8]);
	task.level = atoi(argv[9]);
	sprintf(task.tags, "%s", argv[10]);

	OP_write_task((OPFILE*)outfile, task);

	return 0;
}


int main(int argc, char **argv)
{
	char command[256] = { 0 };

	gets(command);

	int rc = 0; // For SQLite error reporting
	char *zErrMsg = NULL;
	sqlite3 *db = NULL;

	OPFILE *outfile = NULL;

	char *builtquery = NULL;
	char *file_list = NULL;

	// Call Lua script to parse command line
	if (parse_query(command, &file_list, &builtquery, &output) != 0) {
		return 1;
	}

	//
	// Create database
	//

	if (output && !stricmp(output, "DB")) {
		rc = sqlite3_open("oql.db", &db);
	} else {
		rc = sqlite3_open(":memory:", &db);
	}

	if (rc) {
		fprintf(stderr, "Error creating DB: %s\n", zErrMsg);
		goto cleanup;
	}


	//
	// Create table
	//

	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS todo (sqlid INTEGER PRIMARY KEY AUTOINCREMENT, file TEXT, id INTEGER, parent INTEGER, heading TEXT, bodytext TEXT, deadline INTEGER, closed INTEGER, scheduled INTEGER, level INTEGER, tags TEXT)", NULL, 0, &zErrMsg);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error creating table: %s\n", zErrMsg);
		goto cleanup;
	}


	//
	// Parse each file specified on the command line
	//

	if (file_list) {
		char *temp = (char*)strdup(file_list);

		current_file = strtok(temp, ",|;");
		while (current_file != NULL) {
			OPFILE *file = OP_open(current_file);

			while (OP_read_task(file, add_new, (void*)db));

			OP_close(file);

			if (!strncmp(builtquery, "UPDATE", 6) || !strncmp(builtquery, "INSERT", 6)) {
				if (strlen(builtquery) > 0) {
					rc = sqlite3_exec(db, builtquery, sqlite_callback, 0, &zErrMsg);
				}
				if (rc != SQLITE_OK) {
					fprintf(stderr, "Error in SQL query: %s\n%s\n", builtquery, zErrMsg);
					free(temp);
					goto cleanup;
				}

				char backup[64] = { 0 };
				sprintf(backup, "%s.bak", current_file);

#ifdef WIN32
				CopyFile(current_file, backup, FALSE);
				DeleteFile(current_file);
#endif

				outfile = OP_open(current_file);

				char temp_query[256] = { 0 };
				sprintf(temp_query, "SELECT * FROM todo WHERE file = '%s'", current_file);

				rc = sqlite3_exec(db, temp_query, sqlite_write_callback, (void*)outfile, &zErrMsg);

				if (rc != SQLITE_OK) {
					fprintf(stderr, "Error in SQL query: %s\n%s\n", temp_query, zErrMsg);
					OP_close(outfile);
					free(temp);
					goto cleanup;
				}

				OP_close(outfile);
			}

			current_file = strtok(NULL, ",|;");
		}

		free(temp);
	}


	//
	// Execute SQL query built from command line
	//

	if (builtquery && strlen(builtquery) > 0) {
		rc = sqlite3_exec(db, builtquery, sqlite_callback, 0, &zErrMsg);
	}
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error in SQL query: %s\n%s\n", builtquery, zErrMsg);
		goto cleanup;
	}


cleanup:
	if (file_list)
		free(file_list);
	if (builtquery)
		free(builtquery);
	if (output)
		free(output);

	if (zErrMsg)
		sqlite3_free(zErrMsg);

	if (db)
		sqlite3_close(db);

	return 0;
}
