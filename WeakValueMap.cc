#include "node_ref.h"
#include <unordered_map>
#include <cstdint>
#include <string>

#include "WeakValueMap.h"

namespace python_node {
	struct WeakValue;


	struct FinalizationCbData {
		InstanceId_t weakValueMapInstanceId;
		MapKeyPair* pair;

		FinalizationCbData(InstanceId_t weakValueMapInstanceId, MapKeyPair* pair)
			: weakValueMapInstanceId(weakValueMapInstanceId), pair(pair) {
		}
		~FinalizationCbData() {}
	};


	struct WeakValue {
		MapKeyPair* pair = nullptr;
		v8::UniquePersistent<v8::Value> value;
		WeakValue(
            std::unordered_map<std::string, WeakValue>* m,
            std::string s,
            v8::UniquePersistent<v8::Value>&& v
        )
        : value(std::move(v))
        {
			this->pair = new MapKeyPair(m, s);
		}
		
        ~WeakValue() {
			if (this->pair != nullptr) {
				delete this->pair;
				this->pair = nullptr;
			}
		}
		WeakValue() { }
		WeakValue(WeakValue&& o) {
			std::swap(this->pair, o.pair);
			this->value = std::move(o.value);
		}
	};



	WeakValueMap::WeakValueMap(
		int(*test)(const std::string &key)
	) {
		this->map = new std::unordered_map<std::string, WeakValue>;
		this->id = WeakValueMap::instanceId++;
		WeakValueMap::instanceMap->insert(std::make_pair(this->id, test));
	}

	WeakValueMap::~WeakValueMap() {
		delete this->map;
		this->map = nullptr;
		WeakValueMap::instanceMap->erase(this->id);
	}

	void WeakValueMap::finalizationCb(const v8::WeakCallbackInfo<FinalizationCbData>& data) {
		auto *d = data.GetParameter();
		auto id = d->weakValueMapInstanceId;
		// Check that WeakValueMap hasn't been deleted
		if (WeakValueMap::instanceMap->count(id) != 0) {
			auto *p = d->pair;

			//v8::Isolate* isolate = v8::Isolate::GetCurrent();

			auto disposer = WeakValueMap::instanceMap->at(id);
			if (disposer) {
				disposer(p->second);
			}

			//g. (p->second);// first->at(p->second).value.Get(isolate));
			p->first->erase(p->second);
		}
		delete d;
	}

	void WeakValueMap::Set(const std::string &key, v8::Local<v8::Value> argValue) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();

		WeakValueMap* obj = this;// ObjectWrap::Unwrap<WeakValueMap>(args.Holder());

		//Delete from the map if the value to insert is undefined
		if (argValue->IsUndefined()) return WeakValueMap::Delete(key);

		//std::string key = std::string(*v8::String::Utf8Value(args[0]->ToString()));
		v8::UniquePersistent<v8::Value> value(isolate, argValue);

		WeakValue val(obj->map, key, std::move(value));
		FinalizationCbData* cbData = new FinalizationCbData(obj->id, val.pair);
		val.value.SetWeak(cbData, WeakValueMap::finalizationCb, v8::WeakCallbackType::kParameter);
		obj->map->insert(std::make_pair(key, std::move(val)));

		//args.GetReturnValue().Set(args.Holder());
	}

	void WeakValueMap::Delete(const std::string &key) {
		WeakValueMap* obj = this;
		obj->map->erase(key);
		//args.GetReturnValue().Set(args.Holder());
	}


	struct GetResult {
		v8::UniquePersistent<v8::Value> persistentValue;

	};

	v8::Local<v8::Value> WeakValueMap::Get(const std::string &key) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		WeakValueMap* obj = this;

		if (obj->map->count(key) == 1) {
			return (*(obj->map))[key]
                .value.Get(isolate);
		}
		return v8::Undefined(isolate);
	}


	InstanceIdMap* WeakValueMap::instanceMap = new InstanceIdMap();
	InstanceId_t WeakValueMap::instanceId = 0;
}
