#pragma once

#include <Python.h>

#include "node_ref.h"
//#include "PyVars.h"


class CallLaterReg
{
public:
	static PyObject *start_call_later(PyObject *self, PyObject *args);
	static void Init(v8::Local<v8::Object> rootJs);


private:
	CallLaterReg();

	CallLaterReg(CallLaterReg const&) = delete;
	CallLaterReg(CallLaterReg&&) = delete;

	CallLaterReg& operator =(CallLaterReg const&) = delete;
	CallLaterReg& operator =(CallLaterReg&&) = delete;
};


