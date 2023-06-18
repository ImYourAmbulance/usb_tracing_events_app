#include "monitor.h"
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#ifdef __linux__
	#define DEFAULT_PATH "/media/support/FAT32FLASH"
#elif _WIN64 || _WIN32
	#define DEFAULT_PATH "X:\\"
#endif

void mount_action(std::string watch_path) {
	std::cout << watch_path << " has event ADDED\n";
}

void unmount_action(std::string watch_path) {
	std::cout << watch_path << " has event DELETED\n";
}

void copy_files_when_mounted(std::string watch_path) {
	std::cout << watch_path << " has event ADDED\n";
	std::string src = "D:\\Data.txt";
	std::string dst = watch_path + "Data_copied.txt";
	fs::copy(src, dst, fs::copy_options::overwrite_existing);
}

int main() {
	//Monitor m(mount_action, unmount_action);
	Monitor m(std::string(DEFAULT_PATH), mount_action, unmount_action);
	while (1) {
		continue;
	}
	return 0;
}

X:\ has event ADDED
X : \ has event DELETED
