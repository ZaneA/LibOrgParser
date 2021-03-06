//
// Org-monitor
// Written by Zane Ashby (zane.a@demonastery.org)
//
// Reads org-mode files named "todo.org" at the root of Windows drives
// and notifies of any events (Deadlines/Schedules) using Snarl
//
// Schedule each hour and pass "1h" as an argument
//

// Include the headers
#include "../liborgparser/orgparser.h"
#include "snarl.h"
#include <time.h>

time_t reltime;

#define CHECKTIME(which) which > time(NULL) && which < time(NULL) + reltime

void display(OPTASK task)
{
	char iconpath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, iconpath);
	strncat(iconpath, "\\icon.png", MAX_PATH);

	if (CHECKTIME(task.deadline)) {
		char temp[256] = { 0 };
		sprintf(temp, "Deadline: %s\n- %s -\n\n%s", ctime(&task.deadline), task.heading, task.body);
		ShowMessage("OrgMonitor", temp, 0, iconpath, NULL, 0);
	}
	if (CHECKTIME(task.scheduled)) {
		char temp[256] = { 0 };
		sprintf(temp, "Scheduled: %s\n- %s -\n\n%s", ctime(&task.scheduled), task.heading, task.body);
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

	reltime = OP_parse_reltime(argv[1]);

	for (int i = 'C'; i <= 'Z'; i++) {
		char temp[32] = { 0 };
		sprintf(temp, "%c:\\todo.org", i);

		OPFILE *file = OP_open(temp);

		while (OP_read_task(file, display));

		OP_close(file);
	}

	return 0;
}
