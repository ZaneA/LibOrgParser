static int print_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

#define SetColor(color) SetConsoleTextAttribute(hConsole, color)
#define COLOR_NORMAL 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COLOR_ID 	FOREGROUND_BLUE
#define COLOR_HEADING 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_BODYTEXT 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COLOR_EXTRA_H 	FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_EXTRA_B 	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY

	SetColor(COLOR_ID);
	printf("%-4i", atoi(argv[2]));

	SetColor(COLOR_HEADING);
	printf("%s\n", argv[4]);

	SetColor(COLOR_BODYTEXT);
	if (strlen(argv[5]) > 0)
		printf("\"%s\"\n", argv[5]);

	if (strlen(argv[10]) > 0) {
		SetColor(COLOR_EXTRA_H);
		printf("( Tags ) ");
		SetColor(COLOR_EXTRA_B);
		printf("%s\n", argv[10]);
	}

	if (atol(argv[6]) != 0) {
		SetColor(COLOR_EXTRA_H);
		time_t time = atol(argv[6]);
		printf("( Deadline ) ");
		SetColor(COLOR_EXTRA_B);
		printf("%s\n", ctime(&time));
	}

	if (atol(argv[7]) != 0) {
		SetColor(COLOR_EXTRA_H);
		time_t time = atol(argv[7]);
		printf("( Closed ) ");
		SetColor(COLOR_EXTRA_B);
		printf("%s\n", ctime(&time));
	}

	if (atol(argv[8]) != 0) {
		SetColor(COLOR_EXTRA_H);
		time_t time = atol(argv[8]);
		printf("( Scheduled ) ");
		SetColor(COLOR_EXTRA_B);
		printf("%s\n", ctime(&time));
	}

	SetColor(COLOR_NORMAL);

	return 0;
}


int main(int argc, char **argv)
{
	char query[512] = { 0 };

	for (int i = 1; i < argc; i++) {
		strcat(query, argv[i]);
		strcat(query, " ");
	}

	OQL_Run(query, // Raw input
		"oql.lua", // Lua file with transformation functions
		"parse_oql", // Transformation function to use
		print_callback); // Printing callback

	return 0;
}
