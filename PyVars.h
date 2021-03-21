#pragma once
#include "node_ref.h"

#include <Python.h>

#include <functional>
#include <memory>



class PyVars
{
public:
	PyVars();

	~PyVars();

	static PyObject *CustomScriptDict;
	static PyObject *pyfunction_dict;
	static PyObject *pyobj_dict;

	static PyObject *JsUvEventLoop;
	static PyObject *JsPromiseWrapper;
	static PyObject *BackgroundTaskRunner;
	static PyObject *JsExceptionHolder;
	static PyObject *add_venv_localpath;
	static PyObject *print_override;
	static PyObject *js_error;
	static PyObject *wrap_coro;

	static PyObject *d_block;

	static PyObject *wait_for;

	static PyObject *JsException;
	static PyTypeObject * JsExceptionHolderTypeObject;

	static PyObject *print_warn;
	static PyObject *create_js_class;
	static PyObject *js_class;


	static PyObject *background_decorator;
	static PyObject *package_dir;
	static PyObject *exec_path;

	static PyObject *print_line_number;

	static PyObject *undefined;

	static PyObject *error_trace;





private:
	PyVars(PyVars const&) = delete;
	PyVars(PyVars&&) = delete;

	PyVars& operator =(PyVars const&) = delete;
	PyVars& operator =(PyVars&&) = delete;
};

