//#include <uv.h>
#include <string>
#include <unordered_map>

//#include <chrono>
#include <stdio.h>


#include "JsVars.h"


using namespace v8;
using namespace std;


JsVars::JsVars() {

}

JsVars::~JsVars() = default;

v8::Isolate *_isolate;
JsVars* JsVars::instance = 0;


void JsVars::Init(v8::Isolate *isolate, v8::Local<v8::Object> global_obj)
{
	_isolate = isolate;
	auto context = _isolate->GetCurrentContext();

	{
		auto prop_name = v8::String::NewFromUtf8(_isolate, "Number", v8::NewStringType::kNormal).ToLocalChecked();
		auto result = global_obj.As<v8::Object>()->Get(context, prop_name);
		if (result.IsEmpty() == false) {
			this->_Number.Reset(_isolate, result.ToLocalChecked());
		}
	}

	{
		auto prop_name = v8::String::NewFromUtf8(_isolate, "String", v8::NewStringType::kNormal).ToLocalChecked();
		auto result = global_obj.As<v8::Object>()->Get(context, prop_name);
		if (result.IsEmpty() == false) {
			this->_String.Reset(_isolate, result.ToLocalChecked());
		}
	}

	{
		auto prop_name = v8::String::NewFromUtf8(_isolate, "Symbol", v8::NewStringType::kNormal).ToLocalChecked();
		auto result = global_obj.As<v8::Object>()->Get(context, prop_name);
		if (result.IsEmpty() == false) {
			this->_Symbol.Reset(_isolate, result.ToLocalChecked());
		}
	}

	{
		auto prop_name = v8::String::NewFromUtf8(_isolate, "BigInt", v8::NewStringType::kNormal).ToLocalChecked();
		auto result = global_obj.As<v8::Object>()->Get(context, prop_name);
		if (result.IsEmpty() == false) {
			this->_BigInt.Reset(_isolate, result.ToLocalChecked());
		}
	}

	{
		auto prop_name = v8::String::NewFromUtf8(_isolate, "Boolean", v8::NewStringType::kNormal).ToLocalChecked();
		auto result = global_obj.As<v8::Object>()->Get(context, prop_name);
		if (result.IsEmpty() == false) {
			this->_Boolean.Reset(_isolate, result.ToLocalChecked());
		}
	}


	//JsVars::getInstance();

	this->_jsroot_global.Reset(_isolate, global_obj);
}


v8::Isolate *JsVars::getIsolate()
{
	return _isolate;
}


v8::Local<v8::Object> JsVars::getJsGlobal()
{
	return this->_jsroot_global.Get(_isolate);
}


JsVars* JsVars::getInstance()
{
	if (instance == 0) {
		instance = new JsVars();
	}

	return instance;
}

