/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define CACHE ".dmenu_cache"

static void die(const char *s);
static int qstrcmp(const void *a, const void *b);
static void scan(void);
static int uptodate(void);

static char **items = NULL;
static const char *home, *path;

int
main(void) {
	if(!(home = getenv("HOME")))
		die("no $HOME");
	if(!(path = getenv("PATH")))
		die("no $PATH");
	if(chdir(home) < 0)
		die("chdir failed");
	if(uptodate()) {
		execlp("cat", "cat", CACHE, NULL);
		die("exec failed");
	}
	scan();
	return EXIT_SUCCESS;
}

void
die(const char *s) {
	fprintf(stderr, "dmenu_path: %s\n", s);
	exit(EXIT_FAILURE);
}

int
qstrcmp(const void *a, const void *b) {
	return strcmp(*(const char **)a, *(const char **)b);
}

void
scan(void) {
	char buf[PATH_MAX];
	char *dir, *p;
	size_t i, count;
	struct dirent *ent;
	DIR *dp;
	FILE *cache;

	count = 0;
	if(!(p = strdup(path)))
		die("strdup failed");
	for(dir = strtok(p, ":"); dir; dir = strtok(NULL, ":")) {
		if(!(dp = opendir(dir)))
			continue;
		while((ent = readdir(dp))) {
			snprintf(buf, sizeof buf, "%s/%s", dir, ent->d_name);
			if(ent->d_name[0] == '.' || access(buf, X_OK) < 0)
				continue;
			if(!(items = realloc(items, ++count * sizeof *items)))
				die("malloc failed");
			if(!(items[count-1] = strdup(ent->d_name)))
				die("strdup failed");
		}
		closedir(dp);
	}
	qsort(items, count, sizeof *items, qstrcmp);
	if(!(cache = fopen(CACHE, "w")))
		die("open failed");
	for(i = 0; i < count; i++) {
		if(i > 0 && !strcmp(items[i], items[i-1]))
			continue;
		fprintf(cache,  "%s\n", items[i]);
		fprintf(stdout, "%s\n", items[i]);
	}
	fclose(cache);
	free(p);
}

int
uptodate(void) {
	char *dir, *p;
	time_t mtime;
	struct stat st;

	if(stat(CACHE, &st) < 0)
		return 0;
	mtime = st.st_mtime;
	if(!(p = strdup(path)))
		die("strdup failed");
	for(dir = strtok(p, ":"); dir; dir = strtok(NULL, ":"))
		if(!stat(dir, &st) && st.st_mtime > mtime)
			return 0;
	free(p);
	return 1;
}
