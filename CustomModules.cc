#include "node_ref.h"

#include <string>
#include <unordered_map>



#include <queue>
#include <mutex>
#include <cstdint>
#include <Python.h>
#include <structmember.h>


#include <string>
#include <chrono>
#include <thread>

#include <sstream>


#include "JsCallObject.h"
#include "JsObject.h"
#include "JsVars.h"
#include "PyVars.h"
#include "uibox.h"

#include "call_later.h"
#include "bg_task.h"
#include "wait_promise.h"
#include "pyconverters.h"
#include "PlynthUtils.h"

#include "CustomModules.h"


using namespace v8;
using namespace std;
using namespace plynth;

static Isolate *_isolate = nullptr;

//static uv_thread_t main_thread;

static JsObject *rootJsObject = nullptr;

static std::thread::id main_tid;

/*
static bool inUIThread()
{
	return std::this_thread::get_id() == main_tid;
}
*/




constexpr int PLYNTH_MAJOR = 1;
constexpr int PLYNTH_MINOR = 0;
constexpr int PLYNTH_MICRO = 3;

static PyObject * version(PyObject *self, PyObject *args)
{
	std::string s = "";
	s = s + std::to_string(PLYNTH_MAJOR);
	s = s + std::string(".");
	s = s + std::to_string(PLYNTH_MINOR);
	s = s + std::string(".");
	s = s + std::to_string(PLYNTH_MICRO);


	return PyUnicode_FromString(s.c_str());

	Py_INCREF(Py_None);
	return Py_None;
}

std::string CustomModuleManager::get_version()
{
	std::string s = "";
	s = s + std::to_string(PLYNTH_MAJOR);
	s = s + std::string(".");
	s = s + std::to_string(PLYNTH_MINOR);
	s = s + std::string(".");
	s = s + std::to_string(PLYNTH_MICRO);


	return s;// PyUnicode_FromString(s.c_str());

}

static PyObject * undefined(PyObject *self, PyObject *args) {
	Py_INCREF(PyVars::undefined);
	return PyVars::undefined;
}


static PyObject * version_info(PyObject *self, PyObject *args)
{
	auto *py_dict = PyDict_New();

	PyDict_SetItemString(py_dict, "major", PyLong_FromLong(PLYNTH_MAJOR));
	PyDict_SetItemString(py_dict, "minor", PyLong_FromLong(PLYNTH_MINOR));
	PyDict_SetItemString(py_dict, "micro", PyLong_FromLong(PLYNTH_MICRO));
	//'alpha', 'beta', 'candidate', or 'final
	PyDict_SetItemString(py_dict, "releaselevel", PyUnicode_FromString("final"));
	PyDict_SetItemString(py_dict, "serial", PyLong_FromLong(0));


	return py_dict;


	Py_INCREF(Py_None);
	return Py_None;
}



bool CustomModuleManager::inMainThread()
{
	return std::this_thread::get_id() == main_tid;

	//return inUIThread();
}




class OnUiAsyncData {

public:
	OnUiAsyncData();

	PyObject * function;

	virtual ~OnUiAsyncData() = default;

private:

	OnUiAsyncData(OnUiAsyncData const&) = delete;
	OnUiAsyncData(OnUiAsyncData&&) = delete;
	OnUiAsyncData& operator =(OnUiAsyncData const&) = delete;
	OnUiAsyncData& operator =(OnUiAsyncData&&) = delete;
};

OnUiAsyncData::OnUiAsyncData() {

}


void _on_ui(uv_async_t* handle)
{
	OnUiAsyncData *asyncData = (OnUiAsyncData*)handle->data;
	PyObject *func = asyncData->function;
	if (PyMethod_Check(func)) {
		auto *self = PyMethod_Self(func); // Borrowed reference
		auto *function = PyMethod_GET_FUNCTION(func); // Borrowed reference
		Py_INCREF(function);

		auto *argTuple = PyTuple_New(1);
		Py_INCREF(self);
		PyTuple_SetItem(argTuple, 0, self);
		if (auto *result = PyObject_CallObject(function, argTuple)) {
			Py_DECREF(result);
		}
		Py_DECREF(argTuple);
		Py_DECREF(function);
	}
	else {
		if (auto *result = PyObject_CallObject(asyncData->function, NULL)) {
			Py_DECREF(result);
		}
	}

	Py_DECREF(asyncData->function);

	uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle2) {
		delete handle2;
	});
}

// new(js.Object, 32)
/*
static PyObject *new_construct(PyObject *self, PyObject *args)
{
	const auto arg_len = PyTuple_Size(args);
	if (arg_len > 0) {
		if (PyObject *function = PyTuple_GetItem(args, 0)) {

			if (PyCallable_Check(function)) {
				Py_INCREF(function);

				uv_loop_t *loop = uv_default_loop();
				auto *async1 = new uv_async_t;

				auto *asyncData = new OnUiAsyncData();
				asyncData->function = function;
				async1->data = asyncData;

				uv_async_init(loop, async1, _on_ui);
				uv_async_send(async1);
			}
		}
	}

	Py_INCREF(Py_None);
	return Py_None;

}
*/



static PyObject *run_in_ui(PyObject *self, PyObject *args)
{
	const auto arg_len = PyTuple_Size(args);
	if (arg_len > 0) {
		if (PyObject *function = PyTuple_GetItem(args, 0)) {

			if (PyCallable_Check(function)) {
				Py_INCREF(function);

				uv_loop_t *loop = uv_default_loop();
				auto *async1 = new uv_async_t;

				auto *asyncData = new OnUiAsyncData();
				asyncData->function = function;
				async1->data = asyncData;

				uv_async_init(loop, async1, _on_ui);
				uv_async_send(async1);
			}
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *add_venv_localpath(PyObject *self, PyObject *args)
{
	if (PyVars::add_venv_localpath) {
		if (PyObject *ret = PyObject_Call(PyVars::add_venv_localpath, args, NULL)) {
			Py_DECREF(ret);
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}
/*
*/

/*
static PyObject *typeof(PyObject *self, PyObject *args)
{
	auto arg_len = PyTuple_Size(args);
	if (arg_len > 0) {
		auto *arg = PyTuple_GetItem(args, 0); // Borrowed reference
			const auto jsObj =
				//JsObjectReg::getTargetV8ObjectFromPyObject(arg);
			PyObject_to_JsValue(arg);

		//const auto jsObj = JsObjectReg::getTargetV8ObjectFromPyObject(arg);
		const auto a = jsObj->TypeOf(_isolate);
		const auto ex_chs = *v8::String::Utf8Value(a);
		PyObject *ex_str = PyUnicode_FromString(ex_chs);
		return ex_str;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
*/





static void _console_info(const std::string &str)
{
	auto *isolate = v8::Isolate::GetCurrent();
	auto context = _isolate->GetCurrentContext();
	auto global = isolate->GetCurrentContext()->Global();

	auto console_str = String::NewFromUtf8(_isolate, "console", v8::NewStringType::kNormal).ToLocalChecked();

	auto console = global->Get(context, console_str);
	if (console.IsEmpty() == false) {
		auto info_str = String::NewFromUtf8(_isolate, "info", v8::NewStringType::kNormal).ToLocalChecked();

		auto maybe_consoleError = console.ToLocalChecked().As<Object>()->Get(context, info_str);
		if (!maybe_consoleError.IsEmpty()) {
			auto consoleError = maybe_consoleError.ToLocalChecked();
			
			if (consoleError->IsFunction()) {
				Local<Value> args[] = {
					String::NewFromUtf8(JsVars::getIsolate(), str.c_str(), v8::NewStringType::kNormal).ToLocalChecked()
				};
				consoleError.As<Function>()->Call(context, console.ToLocalChecked(), 1, args).IsEmpty();
			}
		}
	}

}

class ConsoleInfoAsyncData : public JsCallFromBackground {

public:
	ConsoleInfoAsyncData() = default;

	std::string str;
	
	void ui_getjs() override {
		_console_info(this->str);

	}

	PyObject* getPyObject() override {
		return NULL;
	}

	~ConsoleInfoAsyncData() override = default;
};

 


bool CustomModuleManager::consoleInfo(const std::string &str)
{
	if (CustomModuleManager::inMainThread()) {
		_console_info(str);
		return true;
	}

	// spawn new thread to avoid deadlock 
	std::thread th([](std::string str) {
		auto state_ = PyGILState_Ensure();

		auto *item = new ConsoleInfoAsyncData();
		item->str = std::string(str);

		JsCallFromBackground::call_js_from_background(item);

		PyGILState_Release(state_);

	}, str);

	th.detach();

	return true;
}



static PyObject * js_error(PyObject *self, PyObject *args)
{
	if (PyVars::js_error) {
		if (PyObject *ret = PyObject_Call(PyVars::js_error, args, NULL)) {
			return ret;
		}
	}

	if (PyErr_Occurred()) {
		PyErr_Clear();
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject * exec_path(PyObject *self, PyObject *args)
{
	if (PyVars::exec_path) {
		Py_INCREF(PyVars::exec_path);
		return PyVars::exec_path;
	}

	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject * package_dir(PyObject *self, PyObject *args)
{
	if (PyVars::package_dir) {
		Py_INCREF(PyVars::package_dir);
		return PyVars::package_dir;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}


/*
static PyObject *open_d(PyObject *self, PyObject *args, PyObject *keywds)
{
	if (PyVars::d_block) {
		if (PyObject *ret = PyObject_Call(PyVars::d_block, args, keywds)) {
			return ret;
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}
*/


static PyObject *background_decorator(PyObject *self, PyObject *args, PyObject *keywds)
{
	if (PyVars::background_decorator) {
		if (PyObject *ret = PyObject_Call(PyVars::background_decorator, args, keywds)) {
			return ret;
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *print_warn(PyObject *self, PyObject *args)
{
	if (PyVars::print_warn) {
		if (PyObject *ret = PyObject_Call(PyVars::print_warn, args, NULL)) {
			Py_DECREF(ret);
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *print_console(PyObject *self, PyObject *args)
{
	if (PyVars::print_override) {
		if (PyObject *ret = PyObject_Call(PyVars::print_override, args, NULL)) {
			Py_DECREF(ret);
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}











// plynth.bind(jsobj, "joifwe", pyobj)
// plynth.unbind(jsobj, "joifwe")

// jsobj.jspyobj = plynth.bind(pyobj)
// pyobj = jsobj.get_bind(jsobj)

// self = jsobj.pyobj

static PyObject *_bindobject(PyObject *self, PyObject *args)
{
	auto arg_len = PyTuple_Size(args);
	if (arg_len > 0) {
		auto *arg = PyTuple_GetItem(args, 0); // Borrowed reference
		const auto jsObj =
			//JsObjectReg::getTargetV8ObjectFromPyObject(arg);
			PyObject_to_JsValue_with_bind(arg);
		return JsValue_to_PyObject(jsObj);
	}

	Py_INCREF(Py_None);
	return Py_None;
}



class CreatePyrefAsyncData : public JsCallFromBackground {

public:
	CreatePyrefAsyncData() = default;

	PyObject *self;
	PyObject *args;


	void ui_getjs() override {
		//static std::unique_ptr<UiBox> _JsCall_getattr(JsCallObject *obj, char *name, bool is_main_thread)
		auto a = std::unique_ptr<UiBox>(new UiBox(_bindobject(this->self, this->args), false));
		this->set_box(std::move(a));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}


	~CreatePyrefAsyncData() override = default;
};




static PyObject *bindobject(PyObject *self, PyObject *args)
{
	if (CustomModuleManager::inMainThread()) {
		return _bindobject(self, args);
	}

	auto *item = new CreatePyrefAsyncData();
	item->self= self;
	item->args = args;

	return JsCallFromBackground::call_js_from_background(item);
}


























































// plynth.bind(jsobj, "joifwe", pyobj)
// plynth.unbind(jsobj, "joifwe")

// jsobj.jspyobj = plynth.bind(pyobj)
// pyobj = jsobj.get_bind(jsobj)

// self = jsobj.pyobj


static PyObject *_findbind(PyObject *self, PyObject *args)
{
	auto arg_len = PyTuple_Size(args);
	if (arg_len > 0) {
		auto *arg = PyTuple_GetItem(args, 0); // Borrowed reference
		const auto jsObj = JsObjectReg::getTargetV8ObjectFromPyObject(arg);
		if (jsObj.IsEmpty() == false) {

			auto *a = resolve_pyholder(jsObj);
			if (a) {
				return a;
			}
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}


class CreateResolvePyholderAsyncData : public JsCallFromBackground {

public:
	CreateResolvePyholderAsyncData() = default;

	PyObject *self;
	PyObject *args;


	void ui_getjs() override {
		//static std::unique_ptr<UiBox> _JsCall_getattr(JsCallObject *obj, char *name, bool is_main_thread)
		auto a = std::unique_ptr<UiBox>(new UiBox(_findbind(this->self, this->args), false));
		this->set_box(std::move(a));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}

	~CreateResolvePyholderAsyncData() override = default;
};




static PyObject *findbind(PyObject *self, PyObject *args)
{
	if (CustomModuleManager::inMainThread()) {
		return _findbind(self, args);
	}

	auto *item = new CreateResolvePyholderAsyncData();
	item->self = self;
	item->args = args;

	return JsCallFromBackground::call_js_from_background(item);
}

/*
	if (JsVars::getInstance()->info != nullptr) {
		auto info = *JsVars::getInstance()->info;
		if (info.kArgsLength > 0) {
			auto a = info[0];
			return JsValue_to_PyObject(a);
		}
	}
	*/


/*
static PyObject *get_args(PyObject *self, PyObject *args)
{

	if (CustomModuleManager::inMainThread()) {
	


		if (!JsVars::getInstance()->_current_args.IsEmpty()) {
			auto args = JsVars::getInstance()->_current_args.Get(_isolate);
			return JsValue_to_PyObject(args);
		}

		
	}

	Py_INCREF(Py_None);
	return Py_None;
}
*/

static PyObject *get_this_context(PyObject *self, PyObject *args)
{
	if (CustomModuleManager::inMainThread()) {
		if (!JsVars::getInstance()->_current_this_context.IsEmpty()) {
			auto thisContext = JsVars::getInstance()->_current_this_context.Get(_isolate);
			return JsValue_to_PyObject(thisContext);
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}




//def js_class(superclass = None, jsclassattr = "JsClass", jssuperclassattr = "JsSuperclass") :

static PyObject *js_class(PyObject *self, PyObject *args, PyObject*kwargs)
{
	if (PyVars::js_class) {
		if (PyObject *ret = PyObject_Call(PyVars::js_class, args, kwargs)) {
			return ret;
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}




static PyObject *create_js_class(PyObject *self, PyObject *args, PyObject *kwargs)
{
	if (PyVars::create_js_class) {
		if (PyObject *ret = PyObject_Call(PyVars::create_js_class, args, kwargs)) {
			return ret;
		}
	}
	Py_INCREF(Py_None);
	return Py_None;

}




static PyObject *is_jsvalue(PyObject *self, PyObject *args)
{
	auto arg_len = PyTuple_Size(args);
	bool is_true = false;
	if (arg_len > 0) {
		auto *arg = PyTuple_GetItem(args, 0); // Borrowed reference
		is_true = JsObjectReg::isJsObject(arg);
	}

	if (is_true) {
		Py_INCREF(Py_True);
		return Py_True;
	}

	Py_INCREF(Py_False);
	return Py_False;
}




static PyMethodDef plynth_internal_utilMethods[] = {
	{ "later_request", CallLaterReg::start_call_later, METH_VARARGS, "Return the number of arguments received by the process." },
	//{ "create_future", create_future, METH_VARARGS, "Create future." },
	{ NULL, NULL, 0, NULL }
};


static PyModuleDef plynth_internal_Module = {
	PyModuleDef_HEAD_INIT, "plynth_knic", NULL, -1, plynth_internal_utilMethods,
	NULL, NULL, NULL, NULL
};


static PyObject* PyInit_plynth_util(void)
{
	auto *m = PyModule_Create(&plynth_internal_Module);
	return m;
}

// plynth.runb(func)
// plynth.runf(func)
static PyMethodDef EmbMethods[] = {
	{ "run_in_background", (PyCFunction)BackgroundTask::run_in_newthread, METH_VARARGS | METH_KEYWORDS, "run a function in background" },
	{ "runb", (PyCFunction)BackgroundTask::run_in_newthread, METH_VARARGS | METH_KEYWORDS, "run a function in a background thread" },
	{ "run_in_ui", run_in_ui, METH_VARARGS, "run a function in an ui thread" },
	//{ "new", new_construct, METH_VARARGS, "constuct with new" },
	{ "runf", run_in_ui, METH_VARARGS, "run a function in ui thread" },

	//{ "bindobject", bindobject, METH_VARARGS, "" },
	{ "create_pyholder", bindobject, METH_VARARGS, "" },
	{ "resolve_pyholder", findbind, METH_VARARGS, "" },

	{ "js_this", get_this_context, METH_VARARGS, "get this context in a js function" },
	//{ "js_arguments", get_args, METH_VARARGS, "" },
	//{ "js_arglen", get_arglen, METH_VARARGS, "" },

	{ "add_venv_localpath", add_venv_localpath, METH_VARARGS, "add_venv_localpath" },

	{ "log", print_console, METH_VARARGS, "log to the web console" },
	{ "warn", print_warn, METH_VARARGS, "warn to the web console" },
	{ "background", (PyCFunction)background_decorator, METH_VARARGS | METH_KEYWORDS, "decorator for wrapping plynth.runb" },
	//{ "D", (PyCFunction)open_d, METH_VARARGS | METH_KEYWORDS, "flexible with" },
	{ "js_error", js_error, METH_VARARGS, "get an error in JavaScript catch " },
	{ "package_dir", package_dir, METH_VARARGS, "get a path of package directory" },
	{ "exec_path", exec_path, METH_VARARGS, "get a path of package directory" },

	{ "version", version, METH_VARARGS, "get version str for Plynth" },
	{ "version_info", version_info, METH_VARARGS, "get version info dict" },

	{ "undefined", undefined, METH_VARARGS, "get js undefined" },


	{ "create_js_class", (PyCFunction)create_js_class, METH_VARARGS | METH_KEYWORDS, "" },
	{ "js_class", (PyCFunction)js_class, METH_VARARGS | METH_KEYWORDS, "" },

	{ "is_jsvalue", (PyCFunction)is_jsvalue, METH_VARARGS, "" },





	//{ "wait", WaitPromise::await_promise_or_future, METH_VARARGS, "wait future or promise in background" }, // await promise
	//{ "sleep", plynth_sleep, METH_VARARGS, "sleep" }, // sleep

	//{ "typeof", typeof, METH_VARARGS, "typeof" },


	{ NULL, NULL, 0, NULL }
};

static PyModuleDef PlynthModuleDef = {
	PyModuleDef_HEAD_INIT, "plynth", NULL, -1, EmbMethods,
	NULL, NULL, NULL, NULL
};


static PyObject* _plynthModule = nullptr;
/*

static inline void _add_module_to(PyObject * jsimport_module, const char *name, v8::Local<v8::Object> jsroot)
{
	auto *obj = JsValue_to_PyObject(jsroot, name, jsroot->Get(v8::String::NewFromUtf8(_isolate, name)));
	PyModule_AddObject(jsimport_module, name, obj); // This steals a reference to value.
	std::string fullpath = std::string("plynth.jsimports.") + std::string(name);
	//auto *document_module = 
	PyImport_AddModule(fullpath.c_str());
}


static void _init_jsimport_module(PyObject * jsimport_module, v8::Local<v8::Object> jsroot)
{
	_add_module_to(jsimport_module, "document", jsroot);
	_add_module_to(jsimport_module, "window", jsroot);
	_add_module_to(jsimport_module, "console", jsroot);
	_add_module_to(jsimport_module, "localStorage", jsroot);


	_add_module_to(jsimport_module, "Object", jsroot);
	_add_module_to(jsimport_module, "Array", jsroot);
	_add_module_to(jsimport_module, "String", jsroot);
	_add_module_to(jsimport_module, "Number", jsroot);
	_add_module_to(jsimport_module, "Function", jsroot);
	_add_module_to(jsimport_module, "Boolean", jsroot);
	_add_module_to(jsimport_module, "Date", jsroot);

	_add_module_to(jsimport_module, "Math", jsroot);
	_add_module_to(jsimport_module, "Symbol", jsroot);
	_add_module_to(jsimport_module, "RegExp", jsroot);

	_add_module_to(jsimport_module, "Map", jsroot);
	_add_module_to(jsimport_module, "Set", jsroot);
	_add_module_to(jsimport_module, "WeakMap", jsroot);
	_add_module_to(jsimport_module, "WeakSet", jsroot);


	_add_module_to(jsimport_module, "Promise", jsroot);
	_add_module_to(jsimport_module, "JSON", jsroot);
	_add_module_to(jsimport_module, "Buffer", jsroot);

	_add_module_to(jsimport_module, "setImmediate", jsroot);
	_add_module_to(jsimport_module, "clearImmediate", jsroot);
	_add_module_to(jsimport_module, "setTimeout", jsroot);
	_add_module_to(jsimport_module, "setInterval", jsroot);
	_add_module_to(jsimport_module, "clearInterval", jsroot);
	_add_module_to(jsimport_module, "eval", jsroot);

	_add_module_to(jsimport_module, "Error", jsroot);

	_add_module_to(jsimport_module, "Infinity", jsroot);

	_add_module_to(jsimport_module, "NaN", jsroot);
	_add_module_to(jsimport_module, "Proxy", jsroot);


	_add_module_to(jsimport_module, "ArrayBuffer", jsroot);
	_add_module_to(jsimport_module, "DataView", jsroot);
	_add_module_to(jsimport_module, "SharedArrayBuffer", jsroot);
	//_add_module_to(jsimport_module, "TypedArray", jsroot);
	_add_module_to(jsimport_module, "Float32Array", jsroot);
	_add_module_to(jsimport_module, "Float64Array", jsroot);
	_add_module_to(jsimport_module, "Int16Array", jsroot);
	_add_module_to(jsimport_module, "Int32Array", jsroot);
	_add_module_to(jsimport_module, "Int8Array", jsroot);
	_add_module_to(jsimport_module, "Uint16Array", jsroot);
	_add_module_to(jsimport_module, "Uint32Array", jsroot);
	_add_module_to(jsimport_module, "Uint8Array", jsroot);
	_add_module_to(jsimport_module, "Uint8ClampedArray", jsroot);

	_add_module_to(jsimport_module, "WebAssembly", jsroot);

	_add_module_to(jsimport_module, "decodeURI", jsroot);
	_add_module_to(jsimport_module, "decodeURIComponent", jsroot);
	_add_module_to(jsimport_module, "encodeURI", jsroot);
	_add_module_to(jsimport_module, "encodeURIComponent", jsroot);
	_add_module_to(jsimport_module, "encodeURI", jsroot);
	_add_module_to(jsimport_module, "escape", jsroot);
	_add_module_to(jsimport_module, "isFinite", jsroot);
	_add_module_to(jsimport_module, "isNaN", jsroot);
	_add_module_to(jsimport_module, "parseFloat", jsroot);
	_add_module_to(jsimport_module, "parseInt", jsroot);
}
*/


static PyObject* PyInit_plynth(void)
{
	// register types

	// JsObject
	auto *jsreg = JsObjectReg::getInstance();
	jsreg->on_AppendInittab();
	auto *jsObjectType = jsreg->getJsObjectType();
	if (PyType_Ready(jsObjectType) < 0) {
		return NULL;
	}
	Py_INCREF(jsObjectType);


	// JsCallObject
	auto *call_reg = JsCallObjectReg::getInstance();
	call_reg->on_append_inittab();
	auto *jsCallObjectType = call_reg->getJsCallObjectType();
	if (PyType_Ready(jsCallObjectType) < 0) {
		return NULL;
	}
	Py_INCREF(jsCallObjectType);


	_plynthModule = PyModule_Create(&PlynthModuleDef);
	Py_INCREF(_plynthModule);

	rootJsObject = (JsObject*)JsObjectReg::JsObject_new(NULL, NULL);

	auto jsroot = JsVars::getInstance()->getJsGlobal();
	rootJsObject->_object.Reset(_isolate, jsroot);

	/*
	PyModule_AddObject(_plynthModule, "jsglobal", (PyObject *)rootJsObject);
	auto *jsglobal_module = PyImport_AddModule("plynth.jsglobal");
	if (jsglobal_module) {

	}
	*/

	// dynamic
	PyModule_AddObject(_plynthModule, "js", (PyObject *)rootJsObject);
	if (auto *jslib_module = PyImport_AddModule("plynth.js")) {

	}

	//PyModule_AddObject(_plynthModule, "background", (PyObject *)rootJsObject);



	// static
	//auto *jsimport_module = PyImport_AddModule("plynth.jsimports");

	//_init_jsimport_module(jsimport_module, jsroot);

	//auto *taskModule = PyImport_AddModule("plynth.util");
	{
		// submodules
		//auto *taskModule = PyImport_AddModule("plynth.util");

		//auto *asynio_util_module = PyModule_Create(&plynth_internal_Module);

		//auto *m = PyModule_Create(&plynth_internal_Module);
	}

	PyObject * py_dict = NULL;

	/*
	PyObject * py_dict = PyDict_New();
	Py_INCREF(Py_True);
	Py_INCREF(Py_True);
	PyDict_SetItemString(py_dict, "obj", Py_True);
	PyDict_SetItemString(py_dict, "file", Py_True);
	*/
	PyVars::JsException = PyErr_NewException("plynth.JsError", PyExc_Exception, py_dict);
	Py_INCREF(PyVars::JsException);
	PyModule_AddObject(_plynthModule, "JsError", PyVars::JsException);

	return _plynthModule;
}




void CustomModuleManager::pre_register(v8::Local<v8::Object> rootJs)
{
	// main_thread = uv_thread_self();

	JsCallObjectReg::getInstance()->Init();
	JsObjectReg::getInstance()->Init();

	CallLaterReg::Init(rootJs);

	PyImport_AppendInittab("plynth", &PyInit_plynth);
	PyImport_AppendInittab("plynth_knic", &PyInit_plynth_util);

	PyConverters::pre_register(rootJs);
}


void CustomModuleManager::Init(v8::Local<v8::Object> rootJs)
{
	_isolate = v8::Isolate::GetCurrent();

	main_tid = std::this_thread::get_id();

	PyConverters::Init(rootJs);
}

void CustomModuleManager::Clean()
{
	PyConverters::Clean();
}

