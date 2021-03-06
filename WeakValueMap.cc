﻿#include "node_ref.h"
#include <unordered_map>
#include <cstdint>
#include <string>

#include "WeakValueMap.h"

namespace plynth
{
    struct WeakValue;

    struct FinalizationCbData
    {
        InstanceId_t weakValueMapInstanceId;
        MapKeyPair *pair;

        FinalizationCbData(InstanceId_t weakValueMapInstanceId, MapKeyPair *pair)
            : weakValueMapInstanceId(weakValueMapInstanceId), pair(pair)
        {
        }
        ~FinalizationCbData() {}
    };


    struct WeakValue
    {
        MapKeyPair *pair = nullptr;
        v8::UniquePersistent<v8::Value> value;
        
        WeakValue(
            std::unordered_map<std::string, WeakValue> *m,
            std::string s,
            v8::UniquePersistent<v8::Value> &&v
        )
        : value(std::move(v))
        {
            this->pair = new MapKeyPair(m, s);
        }


        ~WeakValue()
        {
            if (this->pair != nullptr) {
                delete this->pair;
                this->pair = nullptr;
            }
        }

        WeakValue() {}

        WeakValue(WeakValue &&o)
        {
            std::swap(this->pair, o.pair);
            this->value = std::move(o.value);
        }
    };


    WeakValueMap::WeakValueMap(int (*test)(const std::string &key))
    {
        this->map = new std::unordered_map<std::string, WeakValue>;
        this->id = WeakValueMap::instanceId++;
        WeakValueMap::instanceMap->insert(std::make_pair(this->id, test));
    }


    WeakValueMap::~WeakValueMap()
    {
        delete this->map;
        this->map = nullptr;
        WeakValueMap::instanceMap->erase(this->id);
    }


    void WeakValueMap::finalizationCb(const v8::WeakCallbackInfo<FinalizationCbData> &data)
    {
        auto *d = data.GetParameter();
        //d->pair->first->at(d->pair->second).value.Reset();
        auto id = d->weakValueMapInstanceId;
        // Check that WeakValueMap hasn't been deleted
        if (WeakValueMap::instanceMap->count(id) != 0) {
            auto *p = d->pair;

            //v8::Isolate* isolate = v8::Isolate::GetCurrent();
            auto disposer = WeakValueMap::instanceMap->at(id);
            if (disposer) {
                disposer(p->second);
            }

            p->first->erase(p->second);
        }
        delete d;
    }


    void WeakValueMap::Set(const std::string &key, v8::Local<v8::Value> argValue)
    {
        v8::Isolate *isolate = v8::Isolate::GetCurrent();

        //Delete from the map if the value to insert is undefined
        if (argValue->IsUndefined()) {
            WeakValueMap::Delete(key);
            return;
        }

        v8::UniquePersistent<v8::Value> value(isolate, argValue);

        WeakValue val{this->map, key, std::move(value)};
        
        FinalizationCbData *cbData = new FinalizationCbData(this->id, val.pair);
        val.value.SetWeak(cbData, WeakValueMap::finalizationCb,
            v8::WeakCallbackType::kParameter);
        this->map->insert(std::make_pair(key, std::move(val)));
    }


    void WeakValueMap::Delete(const std::string &key)
    {
        this->map->erase(key);
    }


    v8::Local<v8::Value> WeakValueMap::Get(const std::string &key)
    {
        v8::Isolate *isolate = v8::Isolate::GetCurrent();

        if (this->map->count(key) == 1) {
            return (*(this->map))[key]
                .value.Get(isolate);
        }
        return v8::Undefined(isolate);
    }


    InstanceIdMap *WeakValueMap::instanceMap = new InstanceIdMap();
    InstanceId_t WeakValueMap::instanceId = 0;
}
