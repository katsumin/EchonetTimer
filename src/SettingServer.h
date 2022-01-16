#ifndef _SETTING_SERVER_H_
#define _SETTING_SERVER_H_
#include "WebServer.h"
#include "Preferences.h"
#include <vector>
#include "DataStore.h"

class SettingServer
{
private:
public:
	SettingServer();

	void begin(DataStore *ds);
	void exec();
};

#endif