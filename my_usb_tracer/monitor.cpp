#include "monitor.h"

#include <iostream>
#ifdef __linux__ 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <chrono>
#include <pwd.h>
using namespace std::chrono_literals;

#endif 


#ifdef __linux__

Monitor::Monitor(std::function<void(std::string)> mount_action,
	std::function<void(std::string)> unmount_action,
	int nsec_mount_waiting) :
	nsec_mount_waiting{ nsec_mount_waiting },
	mount_action{ mount_action }, unmount_action{ unmount_action }
{
	std::string default_path = "/media/";
	std::string username;
	if (get_username(username)) {
		default_path += username;
	}
	else {
		std::cerr << "Constructor error. Cant find the username\n";
	}
	start_monitoring_no_device(default_path);

}


Monitor::Monitor(std::string path, std::function<void(std::string)> mount_action,
	std::function<void(std::string)> unmount_action,
	int nsec_mount_waiting) :
	nsec_mount_waiting{ nsec_mount_waiting }, mount_action{ mount_action },
	unmount_action{ unmount_action }
{
	start_monitoring(path);
}



Monitor::~Monitor() {
	fileWatcher->removeWatch(watchID);
}


bool Monitor::get_username(std::string& username) {
	uid_t uid = geteuid();
	struct passwd* pw = getpwuid(uid);
	if (pw) {
		username = std::string(pw->pw_name);
		return true;
	}
	else {
		return false;
	}
}


void Monitor::SetWaitTime(int new_nsec_mount_waiting) {
	nsec_mount_waiting = new_nsec_mount_waiting;
}


Monitor::UpdateListener::UpdateListener(int nsec_mount_waiting, std::string watch_path,
	std::string device_name,
	std::function<void(std::string)> mount_action,
	std::function<void(std::string)> unmount_action)
	: nsec_mount_waiting{ nsec_mount_waiting },
	watch_path{ watch_path },
	device_name{ device_name },
	mount_action{ mount_action },
	unmount_action{ unmount_action } {}


Monitor::UpdateListener::UpdateListener(int nsec_mount_waiting, std::string watch_path,
	std::function<void(std::string)> mount_action,
	std::function<void(std::string)> unmount_action)
	: nsec_mount_waiting{ nsec_mount_waiting },
	watch_path{ watch_path }, mount_action{ mount_action },
	unmount_action{ unmount_action } {
	device_name = "None";
}



void Monitor::UpdateListener::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action,
	std::string oldFilename)
{
	switch (action)
	{
	case efsw::Actions::Add: {

		if ((device_name == std::string(filename)) || (device_name == std::string("None"))) {
			device_name = filename;
			long long time_elapsed;
			std::chrono::steady_clock::time_point right_now;
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			bool overwait = false;
			while ((!check_if_mounted(dir, std::string(dir) + std::string(filename))))
			{
				right_now = std::chrono::steady_clock::now();
				time_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(right_now - begin).count();
				// 1 second = 1 000 000 microseconds 
				if (time_elapsed > (nsec_mount_waiting * 1000000)) {
					overwait = true;
					break;
				}
			}
			if (!overwait) {
				// If overwait == false then we have a mounted USB flash drive
				// mount_action prints the information and provides some actions 
				mount_action(watch_path + "/" + device_name);
			}
			else {
				std::cout << "Unable to mount a usb device. Time elapsed exceeds the limit\n";
			}
		}
		break;
	}
	case efsw::Actions::Delete:
		device_name = filename;
		unmount_action(watch_path + "/" + device_name);
		break;
	case efsw::Actions::Modified:
		// std::cout << "Modified\n";
		break;
	case efsw::Actions::Moved:
		//std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from (" << 
		//oldFilename << ")" << 	std::endl;
		break;
	default:
		std::cout << "Should never happen!" << std::endl;
	}
}
int Monitor::UpdateListener::check_if_mounted(const std::string& parent_dir, const std::string& mountpoint_dir) {
	struct stat mountpoint;
	struct stat parent;

	// Get the stat structure of the directory
	if (stat(mountpoint_dir.c_str(), &mountpoint) == -1) {
		perror("failed to stat mountpoint:");
		exit(EXIT_FAILURE);
	}

	// and its parent
	if (stat(parent_dir.c_str(), &parent) == -1) {
		perror("failed to stat parent:");
		exit(EXIT_FAILURE);
	}

	// Compare the st_dev fields in the results: if they are
	// equal, then both the directory and its parent belong 
	// to the same filesystem, and so the directory is not 
	// currently a mount point.
	if (mountpoint.st_dev == parent.st_dev) {
		//there is nothing mounted in that directory
		return 0;
	}
	else {
		//there is currently a filesystem mounted
		return 1;
	}
}

void Monitor::start_monitoring_no_device(std::string watch_path) {
	// Create the file system watcher instance
	// efsw::FileWatcher allow a first boolean parameter that indicates if it should start with the generic file watcher instead of the platform specific backend

	fileWatcher.reset(new efsw::FileWatcher());
	// Create the instance of your efsw::FileWatcherListener implementation

	listener.reset(new UpdateListener(nsec_mount_waiting, watch_path, mount_action, unmount_action));
	// Add a folder to watch, and get the efsw::WatchID
	// It will watch the /tmp folder recursively ( the third parameter indicates that is recursive )
	// Reporting the files and directories changes to the instance of the listener
	watchID = fileWatcher->addWatch(watch_path.c_str(), listener.get(), false);
	// Start watching asynchronously the directories
	fileWatcher->watch();
}




void Monitor::start_monitoring(std::string watch_path) {
	// Create the file system watcher instance
	// efsw::FileWatcher allow a first boolean parameter that indicates if it should start with the generic file watcher instead of the platform specific backend
	
	
	fileWatcher.reset(new efsw::FileWatcher());
	
	// Create the instance of your efsw::FileWatcherListener implementation


	// SPLIT HERE
	std::string path = watch_path;
	// possible error here
	std::size_t botDirPos = path.find_last_of("/");

	watch_path = path.substr(0, botDirPos);
	device_name = path.substr(botDirPos + 1, path.length());


	listener.reset(new UpdateListener(nsec_mount_waiting, watch_path, device_name, mount_action, unmount_action));
	
	// Add a folder to watch, and get the efsw::WatchID
	// It will watch the /tmp folder recursively ( the third parameter indicates that is recursive )
	// Reporting the files and directories changes to the instance of the listener
	watchID = fileWatcher->addWatch(watch_path.c_str(), listener.get(), false);
	// Start watching asynchronously the directories
	fileWatcher->watch();
}



#elif _WIN32 || _WIN64
static const std::string rev_slash = ":\\";
Monitor::Monitor(std::function<void(std::string)> mount_action,
	std::function<void(std::string)> unmount_action)
{

	this->mount_action = [mount_action](char c) { mount_action(std::string(1, c) + rev_slash); };
	this->unmount_action = [unmount_action](char c) { unmount_action(std::string(1, c) + rev_slash); };
	start_monitoring();
}

Monitor::Monitor(std::string watch_path, std::function<void(std::string)> mount_action,
	std::function<void(std::string)> unmount_action)
{
	this->watch_path = watch_path;
	this->mount_action = [mount_action, watch_path](char c)
	{ 
		if (watch_path == (std::string(1, c) + rev_slash))
		{
			mount_action(std::string(1, c) + rev_slash);
		}
	};
	this->unmount_action = [unmount_action, watch_path](char c)
	{ 
		if (watch_path == (std::string(1, c) + rev_slash))
		{
			unmount_action(std::string(1, c) + rev_slash);
		}
	};
	start_monitoring();
}
Monitor::~Monitor() {
	mon->stop();
}


void Monitor::unmount_failed(std::string letter) {
	std::cout << "Failed to eject device: " << letter << std::endl;
}


void Monitor::device_added(std::string letter)
{
	std::cout << "Added USB disk: " << letter << std::endl;
}

void Monitor::device_removed(std::string letter)
{
	std::cout << "UNSAFE-removed USB disk: " << letter << std::endl;
}

bool Monitor::device_safe_removed(char letter) {
	std::cout << "SAFE - removed USB disk : " << letter << std::endl;
	return 1;
}

void Monitor::device_remove_failed(std::string letter)
{
	std::cout << "Failed to eject device: " << letter << std::endl;
}

void Monitor::start_monitoring() {

	mon = usb_monitor::create();
	mon->on_device_add(mount_action);
	mon->on_device_remove(unmount_action);
	mon->on_device_safe_remove(&Monitor::device_safe_removed);
	//mon->on_device_remove_fail(&Monitor::device_remove_failed);

	mon->mount_existing_devices();

	mon->start();
	usb_monitor::message_pump();
}
#endif 
