/*
 * LibOrgParser
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Parses org-mode files and passes info off to a user defined callback
 */

#include "orgparser.h"


/* String trimming function from some old code I had, probably the first reuse ever */
static char *trim(char *buffer, char *stripchars)
{
	int i = 0;

	/* Left Side */
	char *start = buffer;

left:
	for (i = 0; i < strlen(stripchars); i++) {
		if (*start == stripchars[i]) {
			start++;
			goto left;
		}
	}

	/* Right Side */
	char *end = start + strlen(start) - 1;

right:
	for (i = 0; i < strlen(stripchars); i++) {
		if (*end == stripchars[i]) {
			*end = '\0';
			--end;
			goto right;
		}
	}

	return start;
}


/* This function takes a date/time in org-mode format and returns a unix timestamp
 * (basically a poor mans strptime since that's missing in Windows) */
static time_t parse_time(char *buffer)
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));

	sscanf(buffer, "%d-%d-%d %*s %d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min);

	tm.tm_year -= 1900;
	--tm.tm_mon;

	return mktime(&tm);
}


/* This function Parses an Emacs 'Org-mode' file to the best of its very uncapable ability */
void parse_org_file(char *path, add_new_callback add_new)
{
	FILE *fp = fopen(path, "r");

	if (!fp) {
		fprintf(stderr, "Failed to open %s for reading\n", path);
		return;
	}

	/* THIS WHOLE PART IS A MINDFUCK TO ME AND I CODED IT */

	int id = 1; /* Each heading has an ID */
	int id_list[10] = {0}; /* track 10 levels deep, any more and it will crash */
	int last_level = 0;
	int level = 0;
	char heading[256];
	memset(heading, 0, 256);
	char bodytext[256];
	memset(bodytext, 0, 256);
	char tags[256];
	memset(tags, 0, 256);
	time_t deadline_t, closed_t, scheduled_t;

	while (!feof(fp)) {
		char line[256];
		fgets(line, sizeof(line), fp); /* Get Line */

		if (line[0] != '#') {
			/* Not a comment line */

			if (line[0] == '*') {
				/* Heading line */
				if (level > 0 && strlen(heading) > 0) {
					/* Already got some info */
					add_new(id, id_list[level-1], heading, trim(bodytext, "\n"), deadline_t, closed_t, scheduled_t, level, trim(tags, " \t\n"));
					id++;
				}

				/* Keep track of levels for parent -> child */
				for (level = 0; line[level] == '*'; level++);

				/* Keep track of id's at different levels to use as parent data */
				id_list[level] = id;

				memset(heading, 0, 256);
				strcpy(heading, trim(line + level + 1, " \t\n"));

				/* Yank Tags out of this and store them seperately, then trim heading to remove whitespace, ugly */
				memset(tags, 0, 256);
				if (heading[strlen(heading)-1] == ':') {
					/* Tags at the end of line? Let's hope so */
					char *pointer;
					for (pointer = heading + strlen(heading) - 1; *pointer != ' ' && *pointer != '\t'; pointer--);
					strcpy(tags, pointer);
					for (pointer = heading + strlen(heading) - 1; *pointer != ' ' && *pointer != '\t'; pointer--) {
						*pointer = '\0';
					}
					trim(heading, " \t");
				}

				memset(bodytext, 0, 256);

				deadline_t = 0;
				closed_t = 0;
				scheduled_t = 0;
			} else {
				char temp[256];
				strcpy(temp, trim(line, " \t"));
				char timestring[128];
				memset(timestring, 0, 128);

				if (!strncmp(temp, "DEADLINE", 8)) { /* Check others too */
					sprintf(timestring, "%.*s", 20, temp + 11);
					deadline_t = parse_time(timestring);
				} else if (!strncmp(temp, "CLOSED", 6)) {
					sprintf(timestring, "%.*s", 20, temp + 9);
					closed_t = parse_time(timestring);
				} else if (!strncmp(temp, "SCHEDULED", 9)) {
					sprintf(timestring, "%.*s", 20, temp + 12);
					scheduled_t = parse_time(timestring);
				} else {
					strcat(bodytext, trim(line, " \t")); /* Don't copy those datetime lines into the body */
				}
			}
		}
	}

	if (level > 0 && strlen(heading) > 0) {
		/* Already got some info */
		add_new(id, id_list[level-1], heading, trim(bodytext, "\n"), deadline_t, closed_t, scheduled_t, level, trim(tags, " \t\n"));
		id++;
	}

	fclose(fp);
}
