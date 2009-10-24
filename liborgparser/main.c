/*
 * LibOrgParser
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Parses org-mode files and passes info off to a user defined callback
 */

#include "orgparser.h"

#define MAX_DEPTH 10


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
	struct tm tm = { 0 };

	sscanf(buffer, "%d-%d-%d %*s %d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min);

	tm.tm_year -= 1900;
	--tm.tm_mon;
	tm.tm_isdst = -1;

	return mktime(&tm);
}


/* Takes a string like "1d10h" and turns it into seconds */
long parse_reltime(char *buffer)
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

			case 'w': /* week */
				num = atoi(temp);
				amount += 60 * 60 * 24 * 7 * num;
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

			default: /* Should be a number */
				if (buffer[i] > 47 && buffer[i] < 58) {
					temp[strlen(temp)] = buffer[i];
				}
				break;
		}
	}

	if (neg) {
		return -amount;
	} else {
		return amount;
	}
}


/* This function Parses an Emacs 'Org-mode' file to the best of its very uncapable ability */
void parse_org_file(char *path, add_new_callback add_new)
{
	FILE *fp = fopen(path, "r");

	if (!fp) {
		fprintf(stderr, "Failed to open %s for reading\n", path);
		return;
	}

	int id = 1;			/* Each heading has an ID */
	int id_list[MAX_DEPTH] = { 0 };	/* track MAX_DEPTH levels deep, any more and it will crash */
	int last_level = 0;
	int level = 0;
	char heading[256] = { 0 };
	char bodytext[256] = { 0 };
	char tags[64] = { 0 };
	time_t deadline_t, closed_t, scheduled_t;

	while (!feof(fp)) {
		char line[256] = { 0 };
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
				if (level < MAX_DEPTH) {
					id_list[level] = id;
				}

				memset(heading, 0, sizeof(heading));
				strcpy(heading, trim(line + level + 1, " \t\n"));

				/* Yank Tags out of this and store them seperately, then trim heading to remove whitespace, ugly */
				memset(tags, 0, sizeof(tags));
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

				memset(bodytext, 0, sizeof(bodytext));

				deadline_t = closed_t = scheduled_t = 0;
			} else {
				char *temp = trim(line, " \t");

				if (!strncmp(temp, "DEADLINE", 8)) { /* Check others too */
					deadline_t = parse_time(temp + 11);
				} else if (!strncmp(temp, "CLOSED", 6)) {
					closed_t = parse_time(temp + 9);
				} else if (!strncmp(temp, "SCHEDULED", 9)) {
					scheduled_t = parse_time(temp + 12);
				} else {
					/* Don't copy these datetime lines into the body */
					strcat(bodytext, temp);
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
