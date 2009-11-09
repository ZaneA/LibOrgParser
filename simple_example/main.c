/*
 * LibOrgParser Example app
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Reads org-mode files using LibOrgParser and prints parts to stdout
 */

/* Include the header */
#include "../liborgparser/orgparser.h"

OPFILE *outfile = NULL;

void callback(OPTASK task)
{
	printf("ID:\t\t%d\n", task.id);
	printf("Parent:\t\t%d\n", task.parent_id);
	printf("Heading:\t%s\n", task.heading);
	printf("Body Text:\t%s\n", task.body);
	printf("Deadline:\t%d\n", task.deadline);
	printf("Closed:\t\t%d\n", task.closed);
	printf("Scheduled:\t%d\n", task.scheduled);
	printf("level:\t\t%d\n", task.level);
	printf("Tags:\t\t%s\n", task.tags);
	printf("\n");

	OP_write_task(outfile, task);
}

int main(int argc, char **argv)
{
	/*
	 * Check length of options (need at least a file)
	 */

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file1> [file2] .. [filen]\n", argv[0]);
		return 1;
	}


	/*
	 * Parse each file specified on the command line
	 */

	outfile = OP_open("test.org");

	int i;
	for (i = 1; i < argc; i++) {
		OPFILE *file = OP_open(argv[i]);

		while (OP_read_task(file, callback));

		OP_close(file);
	}

	OP_close(outfile);

	return 0;
}
