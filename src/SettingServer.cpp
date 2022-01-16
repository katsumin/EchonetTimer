#include "SettingServer.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

WebServer _server(80);
String _keyList;
DataStore *_ds;

#define PREFERENCES_KEYS ("keys")

SettingServer::SettingServer()
{
}

static const char *trimKey(const char *key)
{
	// 長いIDを右詰めで短くする
	int pos = strlen(key) > 15 ? strlen(key) - 15 : 0;
	return &key[pos];
}

void preferenceWrite(const char *key, String value)
{
	Preferences preferences;
	preferences.begin("EchonetTimer");
	preferences.putString(trimKey(key), value);
	preferences.end();
}

String preferenceRead(const char *key)
{
	Preferences preferences;
	preferences.begin("EchonetTimer");
	String value = preferences.getString(trimKey(key), String(""));
	preferences.end();
	return value;
}

static String _getContentType(String filename)
{
	if (_server.hasArg("download"))
		return "application/octet-stream";
	else if (filename.endsWith(".htm"))
		return "text/html";
	else if (filename.endsWith(".html"))
		return "text/html";
	else if (filename.endsWith(".css"))
		return "text/css";
	else if (filename.endsWith(".js"))
		return "application/javascript";
	else if (filename.endsWith(".png"))
		return "image/png";
	else if (filename.endsWith(".gif"))
		return "image/gif";
	else if (filename.endsWith(".jpg"))
		return "image/jpeg";
	else if (filename.endsWith(".ico"))
		return "image/x-icon";
	else if (filename.endsWith(".xml"))
		return "text/xml";
	else if (filename.endsWith(".pdf"))
		return "application/x-pdf";
	else if (filename.endsWith(".zip"))
		return "application/x-zip";
	else if (filename.endsWith(".gz"))
		return "application/x-gzip";
	return "text/plain";
}

static void _handleFileRead(String path)
{ // ファイルのMIMEタイプを取得
	String contentType = _getContentType(path);

	// SPIFFSにファイルが存在するか確認
	if (SPIFFS.exists(path))
	{
		// ファイルを読み込む
		File file = SPIFFS.open(path, "r");

		// cache設定
		_server.sendHeader("Cache-Control", "public, max-age=86400");

		// 読み込んだファイルをブラウザ側に返す
		size_t sent = _server.streamFile(file, contentType);

		// ファイルクローズ
		file.close();
	}
}

static void _handleRoot()
{
	_handleFileRead("/index.html");
}

static void _handleNotFound()
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += _server.uri();
	message += "\nMethod: ";
	message += (_server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += _server.args();
	message += "\n";
	for (uint8_t i = 0; i < _server.args(); i++)
	{
		message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
	}
	_server.send(404, "text/plain", message);
}

static void _handleConfirmFile()
{
	// URIを取得
	String path = _server.uri();

	// SPIFFSにファイルが存在しているか確認
	if (SPIFFS.exists(path))
	{
		// css/site.css、js/jquery.min.js、js/eeprom.js を読み込みブラウザ側に返す
		_handleFileRead(path);
	}
	else
	{
		_handleNotFound();
	}
}

static void _handleRegisterTimer()
{
	int argc = _server.args();
	Serial.println(argc);

	// DynamicJsonDocument doc(1024);
	StaticJsonDocument<2048> doc;

	HTTPMethod method = _server.method();
	int resCode = 200;
	String res = "OK";
	switch (method)
	{
	case HTTP_POST:
	{
		String id = "";
		String settings = "";
		for (int i = 0; i < _server.args(); i++)
		{
			// Serial.print(_server.argName(i));
			// Serial.print(" : ");
			// Serial.println(_server.arg(i));
			if (_server.argName(i).equals("id"))
			{
				id = _server.arg(i);
			}
			else if (_server.argName(i).equals("settings"))
			{
				settings = _server.arg(i);
			}
		}
		if (!id.isEmpty() && !settings.isEmpty())
		{
			// Deviceへの反映
			std::map<String, Node *> *nodes = _ds->getNodes();
			auto it = nodes->find(id);
			if (it != nodes->end())
			{
				it->second->reflectSettings(settings);
			}
			// 不揮発メモリへの登録
			Serial.printf("%s: %s\n", id.c_str(), settings.c_str());
			preferenceWrite(id.c_str(), settings);
		}
	}
	break;
	case HTTP_GET:
	{
		String id = "";
		for (int i = 0; i < _server.args(); i++)
		{
			String name = _server.argName(i);
			String value = _server.arg(i);
			if (name.equals("key"))
			{
				id = value;
			}
		}
		if (id.isEmpty())
		{
			_server.send(resCode, "application/json", "{}");
		}
		else
		{
			Serial.printf("id : %s\n", id.c_str());
			String setting = preferenceRead(id.c_str());
			_server.send(resCode, "application/json", setting);
		}
	}
	default:
		break;
	}
	_server.send(resCode, "text/plain", res);
}

static void _handleKeyList()
{
	_keyList = preferenceRead(PREFERENCES_KEYS);
	_keyList.replace(",", "\",\"");
	_keyList = "[\"" + _keyList + "\"]";
	Serial.printf("_handleKeyList: %s\n", _keyList.c_str());
	_server.send(200, "application/json", _keyList);
}

void SettingServer::begin(DataStore *ds)
{
	_ds = ds;
	_keyList = preferenceRead(PREFERENCES_KEYS);
	if (!_keyList.isEmpty())
	{
		// 設定済み
		Serial.printf("nvs設定(keys) :%s\n", _keyList.c_str());
		_keyList.replace(",", "\",\"");
		_keyList = "[\"" + _keyList + "\"]";
	}
	else
	{
		_keyList = "";
		preferenceWrite(PREFERENCES_KEYS, _keyList);
	}

	SPIFFS.begin();

	_server.on("/", _handleRoot);
	_server.on("/keys", _handleKeyList);
	_server.on("/register", _handleRegisterTimer);
	_server.onNotFound(_handleConfirmFile);
	_server.begin();
}

void SettingServer::exec()
{
	_server.handleClient();
}
