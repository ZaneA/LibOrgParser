/*
 * LibOrgParser
 * Written by Zane Ashby (zane.a@demonastery.org)
 *
 * Parses org-mode files and passes info off to a user defined callback
 */

#ifndef __LIBORGPARSER_H
#define __LIBORGPARSER_H

#include <stdio.h>
#include <time.h>

typedef int (*add_new_callback)(int, int, char*, char*, time_t, time_t, time_t, int, char*);

void parse_org_file(char *path, add_new_callback add_new);

#endif
