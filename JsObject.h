#pragma once
#include "node_ref.h"

#include <stdint.h>
#include <stdlib.h>

#include <Python.h>

#include "PyVars.h"


extern PyMethodDef JsObjectMethods[];
extern void setSubscriptFuncs(PyMappingMethods * mappingMethods);


struct JsObject {
	PyObject_HEAD
	v8::Global<v8::Object> _object;
public:
	JsObject();
	~JsObject();
};


class IPyClassReg
{
public:
	IPyClassReg() = default;

	virtual ~IPyClassReg() = default;

private:
	IPyClassReg(const IPyClassReg&) = delete;
	IPyClassReg(IPyClassReg&&) = delete;
	IPyClassReg& operator =(const IPyClassReg&) = delete;
	IPyClassReg& operator =(IPyClassReg&&) = delete;
};




// singleton
class JsObjectReg : public IPyClassReg
{
public:
	~JsObjectReg() override;

	static JsObjectReg* getInstance();

	static PyObject *JsObject_new(PyObject *args, PyObject *kwds);
	
	static v8::Local<v8::Object> getTargetV8Object(JsObject *jsObject);
	static v8::Local<v8::Object> getTargetV8ObjectFromPyObject(PyObject *pyobj);
	static bool isJsObject(PyObject *pyobj);

	static PyObject *JsObject_strfunc(PyObject *arg);


	void Init();

	void on_AppendInittab();

	PyTypeObject *getJsObjectType();

private:
	JsObjectReg();

	static JsObjectReg* instance;

	struct pimpl;
	std::unique_ptr<pimpl> _pimpl;


	JsObjectReg(JsObjectReg const&) = delete;
	JsObjectReg(JsObjectReg&&) = delete;

	JsObjectReg& operator =(JsObjectReg const&) = delete;
	JsObjectReg& operator =(JsObjectReg&&) = delete;
};

