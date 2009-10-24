/*
 * Org-monitor
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Reads org-mode files named "todo.org" at the root of Windows drives
 * and notifies of any events (Deadlines/Schedules) using Snarl
 *
 * Schedule each hour and pass "1h" as an argument
 */

/* Include the headers */
#include "../liborgparser/orgparser.h"
#include "snarl.h"
#include <time.h>

time_t reltime;

#define CHECKTIME(which) which > time(NULL) && which < time(NULL) + reltime

int display(int id, int parent, char *heading, char *bodytext, time_t deadline, time_t closed, time_t scheduled, int level, char *tags)
{
	char iconpath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, iconpath);
	strncat(iconpath, "\\icon.png", MAX_PATH);

	if (CHECKTIME(deadline)) {
		char temp[256];
		sprintf(temp, "Deadline: %s\n- %s -\n\n%s", ctime(&deadline), heading, bodytext);
		ShowMessage("OrgMonitor", temp, 0, iconpath, NULL, 0);
	}
	if (CHECKTIME(scheduled)) {
		char temp[256];
		sprintf(temp, "Scheduled: %s\n- %s -\n\n%s", ctime(&scheduled), heading, bodytext);
		ShowMessage("OrgMonitor", temp, 0, iconpath, NULL, 0);
	}
}


int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <relative-time>\n", argv[0]);
		fprintf(stderr, "Written by Zane Ashby.\n");
		fprintf(stderr, "Example: %s 1d2h\n", argv[0]);
		fprintf(stderr, "Will notify of any events occuring within 1 day and 2 hours\n");

		return 1;
	}

	reltime = parse_reltime(argv[1]);

	char drives[][8] = {
		"C:\\",
		"D:\\",
		"E:\\",
		"F:\\",
		"G:\\",
		"H:\\",
	};

	int i;
	for (i = 0; i < 6; i++) {
		char temp[32] = { 0 };
		sprintf(temp, "%stodo.org", drives[i]);

		FILE *fp = fopen(temp, "r");

		if (fp) { /* File exists */
			fclose(fp);
			parse_org_file(temp, display);
		}
	}

	return 0;
}
