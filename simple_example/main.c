/*
 * LibOrgParser Example app
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Reads org-mode files using LibOrgParser and prints parts to stdout
 */

/* Include the header */
#include "../liborgparser/orgparser.h"

/* Here's our simple callback */
int display(int id, int parent, char *heading, char *bodytext, time_t deadline, time_t closed, time_t scheduled, int level, char *tags)
{
	printf("ID:\t\t%d\n", id);
	printf("Parent:\t\t%d\n", parent);
	printf("Heading:\t%s\n", heading);
	printf("Body Text:\t%s\n", bodytext);
	printf("Deadline:\t%d\n", deadline);
	printf("Closed:\t\t%d\n", closed);
	printf("Scheduled:\t%d\n", scheduled);
	printf("level:\t\t%d\n", level);
	printf("Tags:\t\t%s\n", tags);
	printf("\n");
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

	int i;
	for (i = 1; i < argc; i++) {
		/* Call parse_org_file passing a file name and a callback (see above) */
		parse_org_file(argv[i], display);
	}

	return 0;
}
