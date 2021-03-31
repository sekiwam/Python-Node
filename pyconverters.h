#pragma once
#include <Python.h>

#include <functional>
#include <memory>

#include "PyVars.h"

//typedef const std::string& resInfoType;

extern PyObject* JsValue_to_PyObject(v8::Local<v8::Value> ownerValue, const char *access_name, v8::Local<v8::Value> local_value);
extern PyObject* JsValue_to_PyObject(v8::Local<v8::Value> local_value);


extern PyObject *resolve_pyholder(v8::Local<v8::Value> jsobj);

extern v8::Local<v8::Value> PyObject_to_JsValue_with_bind(PyObject *handle);

extern v8::Local<v8::Value> PyObject_to_JsValue(PyObject *handle);

extern PyObject *static_get_dir(v8::Local<v8::Object> targetLocalObject);

extern PyObject *richcmpfunc3(v8::Local<v8::Object> a, v8::Local<v8::Object> b, int op);



class PyConverters
{
public:
	static void pre_register(v8::Local<v8::Object> rootJs);

	static void Init(v8::Local<v8::Object> rootJs);
	static void Clean();


	//static PyObject *JsException;
};
