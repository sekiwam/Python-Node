#pragma once
#include "node_ref.h"

#include <stdint.h>
#include <stdlib.h>

#include <Python.h>

#include "PyVars.h"



// Not inherited from JsObject because of memory overhead for field padding
struct JsCallObject {
	PyObject_HEAD
	v8::Global<v8::Object> _owner; // owner is neccesary for "this" context in js function
	v8::Global<v8::Function> _function;
	std::string access_name;

public:
	JsCallObject();
	~JsCallObject();
};


// singleton
class JsCallObjectReg
{
public:
	~JsCallObjectReg();

	static JsCallObjectReg* getInstance();

	static PyObject *JsCallObject_New(PyObject *args, PyObject *kwds);

	static v8::Local<v8::Object> getTargetV8Object(JsCallObject *callObject);


	void Init();

	void on_append_inittab();

	PyTypeObject *getJsCallObjectType();


private:
	JsCallObjectReg();

	static JsCallObjectReg* instance;

	struct pimpl;
	std::unique_ptr<pimpl> _pimpl;


	JsCallObjectReg(JsCallObjectReg const&) = delete;
	JsCallObjectReg(JsCallObjectReg&&) = delete;

	JsCallObjectReg& operator =(JsCallObjectReg const&) = delete;
	JsCallObjectReg& operator =(JsCallObjectReg&&) = delete;
};

