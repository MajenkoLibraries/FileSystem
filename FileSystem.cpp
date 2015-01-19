#include <FileSystem.h>

int FileSystem::parsePath(const char *path, char **parts) {
	int part = 0;
	char *data = strdup(path);
	char *tok = strtok(data, "/");
	while (tok) {
		parts[part++] = tok;
		tok = strtok(NULL, "/");
	}
	return part;
}
