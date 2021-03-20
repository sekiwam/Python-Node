#pragma once

#include <unordered_map>
#include <functional>

//using namespace v8;
//#include <cstdint>

namespace python_node {

	struct WeakValue;
	struct FinalizationCbData;

	typedef std::pair<std::unordered_map<std::string, WeakValue>*, std::string> MapKeyPair;
	typedef std::uint64_t InstanceId_t;
	typedef std::unordered_map<InstanceId_t, int(*)(const std::string &key)> InstanceIdMap;

	class WeakValueMap {
	public:
		explicit WeakValueMap(int(*notifier)(const std::string &key));
		void Set(const std::string &key, v8::Local<v8::Value> val);
		void Delete(const std::string & key);
		v8::Local<v8::Value> Get(const std::string &key);

	private:
		~WeakValueMap();
		static void finalizationCb(const v8::WeakCallbackInfo<FinalizationCbData>& data);

		static v8::Persistent<v8::Function> constructor;
		static InstanceId_t instanceId;
		static InstanceIdMap* instanceMap;

		InstanceId_t id;
		std::unordered_map<std::string, WeakValue>* map;
	};
}
