#include <functional>
#include <string>

#ifdef __linux__ 
#include "efsw/efsw.hpp"
#include <memory>
#elif _WIN64 || _WIN32
#include "usb_monitor.h"
#endif 




#ifdef __linux__

class Monitor {
public:
	Monitor(std::function<void(std::string)> mount_action,
		std::function<void(std::string)> unmount_action,
		int nsec_mount_waiting = 30);

	Monitor(std::string path, std::function<void(std::string)> mount_action,
		std::function<void(std::string)> unmount_action,
		int nsec_mount_waiting = 30);

	~Monitor();

	void SetWaitTime(int new_nsec_mount_waiting);

private:
	bool get_username(std::string& username);

	class UpdateListener : public efsw::FileWatchListener {
	public:
		UpdateListener(int nsec_mount_waiting, std::string watch_path,
			std::function<void(std::string)> mount_action,
			std::function<void(std::string)> unmount_action);

	
		UpdateListener(int nsec_mount_waiting, std::string device_name,
			std::string watch_path,
			std::function<void(std::string)> mount_action,
			std::function<void(std::string)> unmount_action);

	int nsec_mount_waiting;
	std::string device_name;
	std::string watch_path;
	std::function<void(std::string)> mount_action;
	std::function<void(std::string)> unmount_action;

	void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action,
		std::string oldFilename = "");
	int check_if_mounted(const std::string& parent_dir, const std::string& mountpoint_dir);

	};
private:
	std::unique_ptr<efsw::FileWatcher> fileWatcher;
	std::unique_ptr <UpdateListener> listener;
	efsw::WatchID watchID;
	int nsec_mount_waiting;
	std::string device_name;
	std::string watch_path;
	std::function<void(std::string)> mount_action;
	std::function<void(std::string)> unmount_action;

	void start_monitoring(std::string watch_path);
	void start_monitoring_no_device(std::string watch_path);
};


#elif _WIN32 || _WIN64

class Monitor {
public:

	Monitor(std::function<void(std::string)> mount_action,
		std::function<void(std::string)> unmount_action);

	Monitor(std::string watch_path, std::function<void(std::string)> mount_action,
		std::function<void(std::string)> unmount_action);

	~Monitor();

private:
	usb_monitor* mon;

	int nsec_mount_waiting;
	std::string watch_path;
	std::function<void(char)> mount_action;
	std::function<void(char)> unmount_action;


	void unmount_failed(std::string  letter);

	static void device_added(std::string letter);

	static void device_removed(std::string letter);

	static bool device_safe_removed(char letter);


	static void device_remove_failed(std::string letter);

	void start_monitoring();
};
#endif 
