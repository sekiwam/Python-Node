//#include <uv.h>
#include <string>

#include "PyVars.h"


using namespace v8;
using namespace std;


PyVars::PyVars() {

}

PyVars::~PyVars() = default;

PyObject *PyVars::CustomScriptDict;
PyObject *PyVars::pyfunction_dict;
PyObject *PyVars::pyobj_dict;

PyObject *PyVars::JsUvEventLoop;
PyObject *PyVars::JsPromiseWrapper;
PyObject *PyVars::BackgroundTaskRunner;
PyObject *PyVars::JsExceptionHolder;
PyObject *PyVars::add_venv_localpath;
PyObject *PyVars::print_override;
PyObject *PyVars::print_warn;
PyObject *PyVars::background_decorator;
PyObject *PyVars::package_dir = nullptr;
PyObject *PyVars::exec_path = nullptr;
PyObject *PyVars::print_line_number;

PyObject *PyVars::create_js_class;
PyObject *PyVars::js_class;


PyObject *PyVars::undefined;



PyObject *PyVars::error_trace;



PyObject *PyVars::js_error;
PyObject *PyVars::wrap_coro;
PyObject *PyVars::d_block;

PyObject *PyVars::wait_for;


PyTypeObject *PyVars::JsExceptionHolderTypeObject;



PyObject *PyVars::JsException;
