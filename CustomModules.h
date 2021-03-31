#pragma once

#include <functional>
#include <memory>
#include <atomic>


//typedef const std::string& resInfoType;


class CustomModuleManager
{
public:

	static bool consoleInfo(const std::string &str);

	static void pre_register(v8::Local<v8::Object> rootJs);

	static void Init(v8::Local<v8::Object> rootJs);
	static void Clean();

	static std::string get_version();

	static bool inMainThread();
};




