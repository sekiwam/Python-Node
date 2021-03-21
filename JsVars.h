#pragma once
#include "node_ref.h"

#include <Python.h>

#include <functional>
#include <memory>




class JsVars
{
public:
	JsVars();
	~JsVars();

	void Init(v8::Isolate *isolate, v8::Local<v8::Object> global_obj);



	static JsVars* getInstance();
	static v8::Isolate *getIsolate();

	v8::Local<v8::Object> getJsGlobal();

	v8::Global<v8::Object> _js_toplevel_var_getter;

	v8::Global<v8::Object> _current_this_context;
	//v8::Global<v8::Object> _current_args;
	//v8::FunctionCallbackInfo<v8::Value>* info = nullptr;


	v8::Global<v8::Value> _Number;
	v8::Global<v8::Value> _String;
	v8::Global<v8::Value> _Symbol;
	v8::Global<v8::Value> _BigInt;
	v8::Global<v8::Value> _Boolean;
	

private:
	static JsVars* instance;

	v8::Global<v8::Object> _jsroot_global;


	JsVars(JsVars const&) = delete;
	JsVars(JsVars&&) = delete;

	JsVars& operator =(JsVars const&) = delete;
	JsVars& operator =(JsVars&&) = delete;
};

