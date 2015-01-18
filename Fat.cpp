#include <FileSystem.h>


Fat::Fat(BlockDevice &dev, uint8_t partition) {
	_dev = &dev;
	_part = partition & 0x03;
	_type = 0;
}

bool Fat::begin() {
	if (!_dev->initialize()) {
		return false;
	}

	uint8_t buffer[512];

	struct bootblock *bb = (struct bootblock *)buffer;

	if (_dev->readRelativeBlock(_part, 0, buffer, sizeof(struct bootblock)) != sizeof(struct bootblock)) {
		errno = -10;
		return false;
	}

	if (!strncmp((const char *)bb->ident, "FAT16", 5)) {
		Serial.println("It's fat16");
		_type = 16;
	} else 	if (!strncmp((const char *)bb->ident, "FAT32", 5)) {
		Serial.println("It's fat32");
		_type = 32;
	} else {
		Serial.println("It's not identified");
		errno = -20; //EINVAL;
		return false;
	}

	_root_block = (bb->nfats * bb->fat_size) + 1;
	_cluster_size = bb->cluster_size;
	_data_start = _root_block + ((bb->nroot * sizeof(struct fat_dirent)) / 512);
	return true;
}

int Fat::open(const char *pathname, int flags) {
	return -1;
}

int Fat::open(const char *pathname, int flags, mode_t mode) {
	return -1;
}

int Fat::creat(const char *pathname, int flags, mode_t mode) {
	return -1;
}

int Fat::openat(int dirfd, const char *pathname, int flags) {
	return -1;
}

int Fat::openat(int dirfd, const char *pathname, int flags, mode_t mode) {
	return -1;
}

int Fat::close(int fd) {
	return -1;
}

size_t Fat::write(int fd, const void *buf, size_t count) {
	return 0;
}

size_t Fat::read(int fd, void *buf, size_t count) {
	return 0;
}

int Fat::access(const char *pathname, int mode) {
	return -1;
}

int Fat::accessat(int dirfd, const char *pathname, int mode) {
	return -1;
}

int Fat::chdir(const char *path) {
	return -1;
}

int Fat::unlink(const char *pathname) {
	return -1;
}

int Fat::unlinkar(int dirfd, const char *pathname) {
	return -1;
}

int Fat::rmdir(const char *pathname) {
	return -1;
}

int Fat::rmdirat(int dirfd, const char *pathname) {
	return -1;
}

int Fat::mkdir(const char *pathname, mode_t mode) {
	return -1;
}

int Fat::mkdirat(int dirfd, const char *pathname, mode_t mode) {
	return -1;
}

int Fat::rename(const char *oldpath, const char *newpath) {
	return -1;
}

int Fat::renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
	return -1;
}
