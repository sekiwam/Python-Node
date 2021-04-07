#include "node_ref.h"

#include <unordered_map>

#include <functional>

#include <chrono>
#include <memory> //std::make_unique
#include <thread> // std::this_thread

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string>
//#include <unistd>
#include <stdint.h>

#include "JsCallObject.h"
#include "JsObject.h"
#include "pyconverters.h"
#include "uibox.h"
#include "JsVars.h"
#include "CustomModules.h"
#include "bg_task.h"


using namespace std;
using namespace v8;
using namespace plynth;


constexpr int MAX_JS_ARGS_LEN = 7;


static v8::Isolate *_isolate = nullptr;

static PyTypeObject *_pytype_JsCallObject = nullptr;




class plynth_try_catch
{
public:
	plynth_try_catch(bool is_main_thread) : trycatch{ JsVars::getIsolate() } {
		this->is_main_thread = is_main_thread;
	}

	~plynth_try_catch() {
		if (trycatch.HasCaught()) {
			v8::Local<v8::Value> exception = trycatch.Exception();
			DCHECK(!exception.IsEmpty());
			this->a = JsCallFromBackground::treat_exception(exception, is_main_thread);

		}
	}

	plynth_try_catch(const plynth_try_catch&) = delete;
	plynth_try_catch& operator=(const plynth_try_catch&) = delete;
private:
	v8::TryCatch trycatch;
	std::unique_ptr<UiBox> a;
	bool is_main_thread;
	//return JsCallFromBackground::treat_exception(exception, is_main_thread);

};




static std::unique_ptr<UiBox> __execute_new_method(v8::Local<v8::Function> func, PyObject *args, bool is_main_thread)
{
	DCHECK(func.IsEmpty() == false);

	// converts real argments passed in PyFunction to v8::Object
	const int arg_len = static_cast<int>(PyTuple_Size(args));
	auto *jsArgs = new v8::Local<v8::Value>[arg_len];
	for (int i = 0; i < arg_len; i++) {
		jsArgs[i] = PyObject_to_JsValue(PyTuple_GetItem(args, i)); // Return value: Borrowed reference.
	}

	v8::TryCatch trycatch(_isolate);

	auto context = _isolate->GetCurrentContext();
	//DCHECK(func->IsConstructor());
	auto new_instance = func->CallAsConstructor(context, arg_len, jsArgs);

	delete[] jsArgs;

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}


	if (!new_instance.IsEmpty()) {
		auto cipher = new_instance.ToLocalChecked();
		if (cipher.IsEmpty()) {
			return std::unique_ptr<UiBox>(new UiBox(PyLong_FromLong(22), false));
		}

		return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(cipher), false));
	}

	Py_INCREF(Py_None);
	return std::unique_ptr<UiBox>(new UiBox(Py_None, false));
}


static std::unique_ptr<UiBox> _execute_new_method(PyObject *self, PyObject *args, bool is_main_thread)
{
	JsCallObject *caller = (JsCallObject*)self;
	//auto ownerValue = caller->_owner.Get(_isolate);
	v8::Local<v8::Function> localValue = caller->_function.Get(_isolate);

	return __execute_new_method(localValue, args, is_main_thread);
}





// return New Reference as same as PyObject_Call
static std::unique_ptr<UiBox> _JsCallObject_tp_call(PyObject *self, PyObject *args, PyObject *kwargs, bool is_main_thread, bool autoNew)
{
	JsCallObject *caller = (JsCallObject*)self;
	auto ownerValue = caller->_owner.Get(_isolate);
	auto localValue = caller->_function.Get(_isolate);

	DCHECK(localValue.IsEmpty() == false);
	if (ownerValue.IsEmpty()) {
		ownerValue = v8::Undefined(_isolate).As<Object>();
	}

	if (autoNew) {
		if (caller->access_name.empty() == false) {
			if (caller->access_name.length() > 0) {
				auto *ch = caller->access_name.c_str();
				//DCHECK(CustomModuleManager::consoleInfo(caller->access_name));
				if (ch[0] <= 'Z' && ch[0] >= 'A') {
					auto *jsVars = JsVars::getInstance();
					auto *isolate= JsVars::getIsolate();
                    // Primitives
					if (localValue == jsVars->_BigInt.Get(isolate)
						|| localValue == jsVars->_Boolean.Get(isolate)
						|| localValue == jsVars->_Number.Get(isolate)
						|| localValue == jsVars->_String.Get(isolate)
						|| localValue == jsVars->_Symbol.Get(isolate))

					{

					}
					else {
						return __execute_new_method(localValue, args, is_main_thread);
					}
				}
			}
		}
		else {
			//DCHECK(CustomModuleManager::consoleInfo("empty"));
		}

	}

	v8::TryCatch trycatch(_isolate);

	const auto arg_len = static_cast<uint32_t>(PyTuple_Size(args));

	v8::Local<v8::Value> call_js_value;

	{
		// converts actual argments in PyFunction to v8::Value
		if (arg_len < MAX_JS_ARGS_LEN) { // stack version
			static v8::Local<v8::Value> stack_js_arg_array[MAX_JS_ARGS_LEN];

			for (uint32_t i = 0; i < arg_len; i++) {
				stack_js_arg_array[i] = PyObject_to_JsValue(PyTuple_GetItem(args, i)); // Return value : Borrowed reference.
			}
			auto context = _isolate->GetCurrentContext();
			auto result = localValue->Call(context, ownerValue, arg_len, stack_js_arg_array);
			if (result.IsEmpty() == false) {
				call_js_value = result.ToLocalChecked();
			}
		}
		else { // heap version
			auto *js_arg_array = new v8::Local<v8::Value>[arg_len];
			for (uint32_t i = 0; i < arg_len; i++) {
				js_arg_array[i] = PyObject_to_JsValue(PyTuple_GetItem(args, i));
			}
			auto context = _isolate->GetCurrentContext();
			auto result = localValue->Call(context, ownerValue, arg_len, js_arg_array);
			if (!result.IsEmpty()) {
				call_js_value = result.ToLocalChecked();
			}
			delete[] js_arg_array;
		}
	}


	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(call_js_value), false));
}


class JsCallObject_call_Item : public JsCallFromBackground {
public:
	JsCallObject_call_Item() {

	}

	~JsCallObject_call_Item() override {

	}

	PyObject *self;
	PyObject *args;
	PyObject *kwargs;
	bool autoNew;

	void ui_getjs() override {
		this->set_box(_JsCallObject_tp_call(this->self, this->args, this->kwargs, false, this->autoNew));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};



static PyObject *JsCallObject_tp_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _JsCallObject_tp_call(self, args, kwargs, true, true);
		return box->get_pyobject();
	}

	auto *item = new JsCallObject_call_Item();
	item->self = self;
	item->args = args;
	item->kwargs = kwargs;
	item->autoNew = true;

	auto *pyobj = JsCallFromBackground::call_js_from_background(item);

	return pyobj;
}


static PyObject *JsCallObject_tp_call_nonew(PyObject *self, PyObject *args, PyObject *kwargs)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _JsCallObject_tp_call(self, args, kwargs, true, false);
		return box->get_pyobject();
	}

	auto *item = new JsCallObject_call_Item();
	item->self = self;
	item->args = args;
	item->kwargs = kwargs;
	item->autoNew = false;

	auto *pyobj = JsCallFromBackground::call_js_from_background(item);

	return pyobj;
}





class JsCallObject_new_Item : public JsCallFromBackground {
public:
	~JsCallObject_new_Item() override = default;

	PyObject *self;
	PyObject *args;

	void ui_getjs() override {
		this->set_box(_execute_new_method(this->self, this->args, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};


static PyObject *execute_new_method(PyObject *self, PyObject *args)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _execute_new_method(self, args, true);
		return box->get_pyobject();
	}

	auto *item = new JsCallObject_new_Item();
	item->self = self;
	item->args = args;

	return JsCallFromBackground::call_js_from_background(item);
}




static std::unique_ptr<UiBox> _JsCall_getattr(JsCallObject *obj, char *name, bool is_main_thread)
{

	if (strcmp(name, "new") == 0) {
		static PyMethodDef *new_method_def = nullptr;

		if (new_method_def == nullptr) {
			new_method_def = new PyMethodDef();

			// std::memset(def, 0, sizeof(PyMethodDef));
			new_method_def->ml_name = name;
			new_method_def->ml_meth = reinterpret_cast<PyCFunction>(execute_new_method);
			new_method_def->ml_flags = METH_VARARGS;
			new_method_def->ml_doc = "new";
		}

		if (auto *m_ptr = PyCFunction_NewEx(new_method_def, (PyObject*)obj, NULL)) {
			return std::unique_ptr<UiBox>(new UiBox(m_ptr, false));
		}
	}
	else if (strcmp(name, "call") == 0) {
		static PyMethodDef *call_method_def = nullptr;

		if (call_method_def == nullptr) {
			call_method_def = new PyMethodDef();

			//			static PyObject *JsCallObject_tp_call(PyObject *self, PyObject *args, PyObject *kwargs)

						// std::memset(def, 0, sizeof(PyMethodDef));
			call_method_def->ml_name = name;
			call_method_def->ml_meth = reinterpret_cast<PyCFunction>(JsCallObject_tp_call_nonew);
			call_method_def->ml_flags = METH_VARARGS;
			call_method_def->ml_doc = "call";
		}

		if (auto *m_ptr = PyCFunction_NewEx(call_method_def, (PyObject*)obj, NULL)) {
			return std::unique_ptr<UiBox>(new UiBox(m_ptr, false));
		}
	}

	v8::TryCatch trycatch(_isolate);

	auto targetLocalObject = obj->_function.Get(_isolate);

	auto v8name = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();
	auto context = _isolate->GetCurrentContext();

	auto result = targetLocalObject->Get(context, v8name);
	if (!result.IsEmpty()) {
		auto localValue = result.ToLocalChecked();// v8::String::NewFromUtf8(_isolate, name));
		return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(targetLocalObject, name, localValue), false));
	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	return std::unique_ptr<UiBox>(new UiBox((PyObject*)NULL, false));
}


class JsCallObject_getattr_Item : public JsCallFromBackground {
public:
	~JsCallObject_getattr_Item() override = default;

	JsCallObject *jsobj;
	std::string name;

	void ui_getjs() override {
		char *name_char = (char*)this->name.c_str();
		this->set_box(_JsCall_getattr(this->jsobj, name_char, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());

	}
};



static std::unique_ptr<UiBox> _JsCall_setattr(JsCallObject *obj, char*name, PyObject *v, bool is_main_thread)
{
	v8::TryCatch trycatch(_isolate);


	if (v == NULL) {
		auto v8name = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();
		auto context = _isolate->GetCurrentContext();

		auto result = obj->_function.Get(_isolate)->Delete(context, v8name);// v8::String::NewFromUtf8(_isolate, name));
		if (result.IsNothing()) {

		}
	}
	else {
		auto jsValue = PyObject_to_JsValue(v);
		auto v8name = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();
		auto context = _isolate->GetCurrentContext();

		auto runInContext = obj->_function.Get(_isolate)->Set(context, v8name, jsValue);
		if (runInContext.IsNothing()) {

		}
	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	return std::unique_ptr<UiBox>(new UiBox((PyObject*)NULL, false));
}



class JsCallObject_setattr_Item : public JsCallFromBackground {
public:
	JsCallObject_setattr_Item(char *name_chars) : name{ name_chars } {


	}

	~JsCallObject_setattr_Item() override = default;

	JsCallObject *jsobj;
	PyObject *val;

	void ui_getjs() override {
		char *name_char = (char*)this->name.c_str();
		this->set_box(_JsCall_setattr(this->jsobj, name_char, this->val, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}

private:
	std::string name;

};



static PyObject *JsCall_getattr(JsCallObject *obj, char *name)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _JsCall_getattr(obj, name, true);
		return box->get_pyobject();
	}

	auto *item = new JsCallObject_getattr_Item();
	item->jsobj = obj;
	item->name = std::string(name);

	return JsCallFromBackground::call_js_from_background(item);
}




static int JsCall_setattr(JsCallObject *obj, char*name, PyObject *v)
{
	if (CustomModuleManager::inMainThread()) {
		_JsCall_setattr(obj, name, v, true);
		return 0;
	}

	auto *item = new JsCallObject_setattr_Item(name);
	item->jsobj = obj;
	item->val = v;

	if (PyObject * pyobj = JsCallFromBackground::call_js_from_background(item)) {
		Py_DECREF(pyobj);
	}

	return 0;
}

// --------------------------- class JsCallObject_richcmpfunc2_Item ------------------------

class JsCallObject_richcmpfunc2_Item : public JsCallFromBackground {
public:
	~JsCallObject_richcmpfunc2_Item() override;

	JsCallObject *jsobj;
	PyObject *val;
	int op;


	static PyObject *_richcmpfunc2(JsCallObject *a, PyObject *b, int op)
	{
		if (Py_TYPE(b) == _pytype_JsCallObject) { // currently supports only for JsObject
			auto left_target = JsCallObjectReg::getTargetV8Object(a);
			auto right_target = JsCallObjectReg::getTargetV8Object((JsCallObject*)b);

			return richcmpfunc3(left_target, right_target, op);
		}

		Py_INCREF(Py_False);
		return Py_False;
	}

	static PyObject *richcmpfunc2(JsCallObject *a, PyObject *b, int op) {
		if (CustomModuleManager::inMainThread()) {
			return _richcmpfunc2(a, b, op);
		}

		auto *item = new JsCallObject_richcmpfunc2_Item();
		item->jsobj = a;
		item->op = op;
		item->val = b;

		auto *pyobj = JsCallFromBackground::call_js_from_background(item);
		if (pyobj) {
			return pyobj;
		}

		Py_INCREF(Py_False);
		return Py_False;
	}

	void ui_getjs() override {
		this->set_box(std::unique_ptr<UiBox>(new UiBox(_richcmpfunc2(this->jsobj, this->val, this->op), false)));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());

	}
};
JsCallObject_richcmpfunc2_Item::~JsCallObject_richcmpfunc2_Item() {


}


static void JsCallObject_dealloc(JsCallObject *self)
{
	self->_function.Reset();
	self->_owner.Reset();
	Py_TYPE(self)->tp_free((PyObject *)self);
}


// pimpl
struct JsCallObjectReg::pimpl
{
public:
	pimpl() :_customCallType(new PyTypeObject())
	{
		_pytype_JsCallObject = this->_customCallType.get();
	}

	std::unique_ptr<PyTypeObject> _customCallType;
	// static PyHeapTypeObject aheap;
	std::unique_ptr<PyMappingMethods> _mappingMethods;
	std::unique_ptr<PyAsyncMethods> _asyncMethods;







	void onAppendInittab()
	{
		this->_customCallType->ob_base.ob_base.ob_type = NULL;
		this->_customCallType->ob_base.ob_base.ob_refcnt = 1;
		this->_customCallType->ob_base.ob_size = 0;

		this->_customCallType->tp_name = "plynth.native";
		this->_customCallType->tp_doc = "plynth.native";
		this->_customCallType->tp_basicsize = sizeof(JsCallObject);
		this->_customCallType->tp_itemsize = 0;
		this->_customCallType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;// | Py_TPFLAGS_HEAPTYPE;

		this->_customCallType->tp_dealloc = (destructor)JsCallObject_dealloc;

		this->_customCallType->tp_methods = JsObjectMethods;

		this->_customCallType->tp_getattr = reinterpret_cast<getattrfunc>(JsCall_getattr);
		this->_customCallType->tp_setattr = reinterpret_cast<setattrfunc>(JsCall_setattr);
		this->_customCallType->tp_call = reinterpret_cast<ternaryfunc>(JsCallObject_tp_call);
		this->_customCallType->tp_richcompare = reinterpret_cast<richcmpfunc>(JsCallObject_richcmpfunc2_Item::richcmpfunc2);

		this->_customCallType->tp_str = JsObjectReg::JsObject_strfunc;
		this->_customCallType->tp_repr = JsObjectReg::JsObject_strfunc;

		this->_asyncMethods.reset(new PyAsyncMethods());
		this->_customCallType->tp_as_async = this->_asyncMethods.get();
		this->_customCallType->tp_as_async->am_await = [](PyObject *arg) {
			return PyLong_FromLong(32299);
		};

		this->_mappingMethods.reset(new PyMappingMethods());
		this->_customCallType->tp_as_mapping = this->_mappingMethods.get();
		setSubscriptFuncs(this->_customCallType->tp_as_mapping);
	}
}; // end of pimpl







JsCallObjectReg::JsCallObjectReg() : _pimpl(new pimpl())
{

}


void JsCallObjectReg::Init()
{
	_isolate = v8::Isolate::GetCurrent();
}


PyTypeObject *JsCallObjectReg::getJsCallObjectType()
{
	return this->_pimpl->_customCallType.get();
}


void JsCallObjectReg::on_append_inittab()
{
	this->_pimpl->onAppendInittab();
}


/**
 * Create new instance of JsCallObject
 */
PyObject *JsCallObjectReg::JsCallObject_New(PyObject *args, PyObject *kwds)
{
	JsCallObject *self;

	auto *type = _pytype_JsCallObject;

	self = (JsCallObject *)type->tp_alloc(type, 0); // PyObject_New(CustomObject, type);
	if (self != NULL) {

	}

	return (PyObject *)self;
}


/*
 * Retrieve v8 object of JsCallObject
 * @static
 */
v8::Local<v8::Object> JsCallObjectReg::getTargetV8Object(JsCallObject *callObject)
{
	return callObject->_function.Get(_isolate);
}


JsCallObject::JsCallObject() {

}

JsCallObject::~JsCallObject() = default;


JsCallObjectReg::~JsCallObjectReg() = default;

JsCallObjectReg* JsCallObjectReg::instance = 0;

JsCallObjectReg* JsCallObjectReg::getInstance()
{
	if (instance == 0) {
		instance = new JsCallObjectReg();
	}

	return instance;
}