//
// LibOrgParser
// Written by Zane Ashby (zane.a@demonastery.org)
//
// Parses org-mode files and passes info off to a user defined callback
//

#include "orgparser.h"


int id = 1;			// Each heading has an ID
int id_list[MAX_DEPTH] = { 0 };	// track MAX_DEPTH levels deep
int level = 0;
int first_write = 1;


// String trimming function, could be better..
static char *trim(char *buffer, char *stripchars)
{
	// Left Side
	char *start = buffer;

left:
	for (int i = 0; i < strlen(stripchars); i++) {
		if (*start == stripchars[i]) {
			start++;
			goto left;
		}
	}

	// Right Side
	char *end = start + strlen(start) - 1;

right:
	for (int i = 0; i < strlen(stripchars); i++) {
		if (*end == stripchars[i]) {
			*end = '\0';
			--end;
			goto right;
		}
	}

	return start;
}


// This function takes a date/time in org-mode format and returns a timestamp
// (basically a very poor mans strptime because it's missing in Windows)
static time_t parse_time(char *buffer)
{
	struct tm tm = { 0 };

	sscanf(buffer, "%d-%d-%d %*s %d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min);

	tm.tm_year -= 1900;
	--tm.tm_mon;
	tm.tm_isdst = -1;

	return mktime(&tm);
}


// Takes a string such as "1d10h" and turns it into a long representing seconds
long OP_parse_reltime(char *buffer)
{
	long amount = 0;
	char temp[8] = { 0 };

	int num, negate = 0;

	for (int i = 0; i < strlen(buffer); i++) {
		switch (buffer[i]) {
			case 's': // Second
				num = atoi(temp);
				amount += num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'm': // Minute
				num = atoi(temp);
				amount += 60 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'h': // Hour
				num = atoi(temp);
				amount += 60 * 60 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'd': // Day
				num = atoi(temp);
				amount += 60 * 60 * 24 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'w': // Week
				num = atoi(temp);
				amount += 60 * 60 * 24 * 7 * num;
				memset(temp, 0, sizeof(temp));
				break;

			case 'M': // Month
				num = atoi(temp);
				amount += 60 * 60 * 24 * 31 * num; // Treat a month as 31 days
				memset(temp, 0, sizeof(temp));
				break;

			case 'y': // Year
				num = atoi(temp);
				amount += 60 * 60 * 24 * 365 * num; // Close enough
				memset(temp, 0, sizeof(temp));
				break;

			case '-': // Make negative
				negate = 1;
				break;

			default: // Should be a number
				if (buffer[i] > 47 && buffer[i] < 58) {
					temp[strlen(temp)] = buffer[i];
				}
				break;
		}
	}

	if (negate) {
		return -amount;
	} else {
		return amount;
	}
}


OPFILE* OP_open(char *path)
{
	OPFILE *file = calloc(1, sizeof(OPFILE));

	if (!file) {
		return NULL;
	}

	file->fp = fopen(path, "a+");

	if (!file->fp) {
		fprintf(stderr, "Could not open file %s\n", path);
		free(file);
		return NULL;
	}

	if (!strncmp(path + strlen(path) - 3, "org", 3)) {
		file->type = OP_TYPE_ORG;
	} else if (!strncmp(path + strlen(path) - 3, "otl", 3)) {
		file->type = OP_TYPE_VIMOUTLINER;
	} else {
		file->type = OP_TYPE_UNKNOWN;
	}


	// Reset values
	id = 1;	
	memset(id_list, 0, sizeof(id_list));
	level = 0;
	first_write = 1;

	return file;
}


void OP_close(OPFILE *file)
{
	if (file && file->fp) {
		fclose(file->fp);
		free(file);
	}
}


int OP_read_task(OPFILE *file, OPCALLBACK callback, void *userData)
{
	int heading_found = 0;

	OPTASK task = { 0 };

	if (!file || !file->fp) {
		return 0;
	}

	while (!feof(file->fp))
	{
		char line[MAX_LINE] = { 0 };
		fgets(line, sizeof(line), file->fp);

		//
		// Filetype specific stuff
		//
		switch (file->type)
		{
			case OP_TYPE_ORG:
				{
					if (line[0] == '#') { // A comment
						// TODO Parse comment lines
					} else { // Not a comment..
						if (line[0] == '*' || (line[0] != '*' && !heading_found)) { // Heading line
							heading_found = 1;

							// Keep track of levels for parent -> child
							for (level = 0; line[level] == '*'; level++);

							// Keep track of id's at different levels to use as parent data
							if (level < MAX_DEPTH) {
								id_list[level] = id;
							}

							// Yank heading out of line
							char heading[MAX_HEADING] = { 0 };
							strncpy(heading, trim(line + level, " \t\n"), sizeof(heading));

							// Yank Tags out of the heading
							char tags[MAX_TAGS] = { 0 };
							if (heading[strlen(heading)-1] == ':') {
								// Tags at the end of line? Let's hope so
								char *pointer;
								for (pointer = heading + strlen(heading) - 1; *pointer != ' ' && *pointer != '\t'; pointer--);
								strcpy(tags, pointer);
								for (pointer = heading + strlen(heading) - 1; *pointer != ' ' && *pointer != '\t'; pointer--) {
									*pointer = '\0';
								}
								trim(heading, " \t");
							}

							// Copy stuff to task structure
							strncpy(task.heading, trim(heading, " \t\n"), sizeof(task.heading));
							strncpy(task.tags, trim(tags, " \t\n"), sizeof(task.tags));
							task.level = level;
						} else if (heading_found) {
							char *temp = trim(line, " \t");

							if (!strncmp(temp, "DEADLINE", 8)) {
								task.deadline = parse_time(temp + 11);
							} else if (!strncmp(temp, "CLOSED", 6)) {
								task.closed = parse_time(temp + 9);
							} else if (!strncmp(temp, "SCHEDULED", 9)) {
								task.scheduled = parse_time(temp + 12);
							} else {
								// Found some body text to append
								strncat(task.body, temp, sizeof(task.body));
							}
						}

						// If next line is heading or eof then add
						unsigned int c = fgetc(file->fp);
						ungetc(c, file->fp);
						if (c == '*' || c == EOF) {
							if (heading_found) {
								task.id = id++;
								task.parent_id = id_list[level-1];

								trim(task.body, " \t\n");

								// Add Task
								callback(task, userData);

								if (c == EOF) return 0;

								return 1;
							}

							if (c == EOF) return 0;
						}
					}
				}
				break;

			case OP_TYPE_VIMOUTLINER:
				{
					// TODO..
				}
				break;
			
			case OP_TYPE_UNKNOWN:
			default:
				return 0; // Unknown type, bail
		}
	}

	return 0;
}


int OP_write_task(OPFILE *file, OPTASK task)
{
	if (!file || !file->fp) {
		return 0;
	}

	//
	// Filetype specific stuff
	//
	switch (file->type)
	{
		case OP_TYPE_ORG:
			{
				char levelchars[MAX_DEPTH] = { 0 };
				char temp[MAX_BODY * 2] = { 0 };

				for (int i = 0; i < task.level; i++) {
					strncat(levelchars, "*", sizeof(levelchars));
				}

				// Pretty ugly.. maybe use strncat instead..
				snprintf(temp, sizeof(temp), "%s%s%s%s%s%s%s%s\n",
					(task.level == 1 && !first_write) ? "\n": "",
					levelchars,
					(task.level > 0) ? " ": "",
					task.heading,
					(strlen(task.tags) > 0) ? "\t\t" : "",
					task.tags,
					(strlen(task.body) > 0) ? "\n " : "",
					task.body);

				if (task.deadline != 0) {
					char deadlinef[MAX_LINE];
					strftime(deadlinef, sizeof(deadlinef), " DEADLINE: <%Y-%m-%d %A %H:%M>\n", localtime(&task.deadline));
					strncat(temp, deadlinef, sizeof(temp));
				}

				if (task.closed != 0) {
					char closedf[MAX_LINE];
					strftime(closedf, sizeof(closedf), " CLOSED: <%Y-%m-%d %A %H:%M>\n", localtime(&task.closed));
					strncat(temp, closedf, sizeof(temp));
				}

				if (task.scheduled != 0) {
					char scheduledf[MAX_LINE];
					strftime(scheduledf, sizeof(scheduledf), " SCHEDULED: <%Y-%m-%d %A %H:%M>\n", localtime(&task.scheduled));
					strncat(temp, scheduledf, sizeof(temp));
				}

				fwrite(temp, sizeof(char), strlen(temp), file->fp);

				first_write = 0;

				return 1;
			}
			break;

		case OP_TYPE_VIMOUTLINER:
			{
				// TODO..
			}
			break;

		case OP_TYPE_UNKNOWN:
		default:
			return 0; // Unknown type, bail
	}

	return 0;
}
