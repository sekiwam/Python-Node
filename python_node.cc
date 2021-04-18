#include "python_node.h"
#include "async_wrap-inl.h"
#include "debug_utils-inl.h"
#include "threadpoolwork-inl.h"
#include "memory_tracker-inl.h"

#include "env-inl.h"
#include "node_external_reference.h"
#include "node_internals.h"
#include "util-inl.h"

#include "v8.h"
#include "Python.h"

#include "WeakValueMap.h"
#include "python_bind.h"

using v8::Array;
using v8::Context;
using v8::FunctionCallbackInfo;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Object;
using v8::String;
using v8::TryCatch;
using v8::Uint32;
using v8::Value;

using fn = int;
namespace python_node
{

    class PyObj : v8::Object {

    };

  // the weak handler function is as follows: 
    static void weakCallbackForObjectHolder(
        const v8::WeakCallbackInfo<int>& data) {
        //delete data.GetParameter();
        printf("released ");
        fflush(stdout);
    }


    static void importPythonModule(const FunctionCallbackInfo<Value> &args)
    {
        if (args[0].IsEmpty()) {
            return;
        }

        auto *isolate = args.GetIsolate();
        auto moduleName = args[0].As<String>();
        if (moduleName.IsEmpty() || moduleName->Length() == 0) {
            return;
        }

        v8::String::Utf8Value utf_str_2(isolate, moduleName);
        auto pythonPath = std::string(*utf_str_2);
        if (pythonPath == "sys") {
            PyObject *sys_ = PyImport_ImportModule("sys");
            PyObject *getter = PyObject_GetAttrString(sys_, "getswitchinterval");
            
            if (PyObject* future = PyObject_CallObject(getter, NULL)) {
                auto double_value = PyFloat_AsDouble(future);


                 const auto jsObj = v8::Object::New(isolate);
                    v8::Persistent<Object> g;
                    g.Reset(isolate, jsObj);
                    g.SetWeak((int*)nullptr, weakCallbackForObjectHolder,     
                                    v8::WeakCallbackType::kParameter);
                    //g.Reset();
                    args.GetReturnValue().Set(jsObj);
            }
        }

       

    }


    static void StartPythonScript(const FunctionCallbackInfo<Value> &args)
    {
        printf("@[%d]", args.Length());
        auto modulePath = args[0].As<String>(); // module path to invoke
        auto funcPath = args[1].As<String>();   // python func name

        
        auto *isolate = args.GetIsolate();

        // test of proxy
        {
            auto prop_name = v8::String::NewFromUtf8(isolate, "jfow", v8::NewStringType::kNormal).ToLocalChecked();

            const auto context = isolate->GetCurrentContext();
            const auto jsObj = v8::Object::New(isolate);

            auto get_symbol = String::NewFromUtf8(isolate, "get", v8::NewStringType::kNormal).ToLocalChecked();

            auto jsfunc = [](const FunctionCallbackInfo<Value> &info) {
                info.GetReturnValue().Set(300);
            };

            auto passData = String::NewFromUtf8(isolate, "abc", v8::NewStringType::kNormal).ToLocalChecked();
            auto func = v8::Function::New(context, jsfunc, passData).ToLocalChecked();
            const auto result = jsObj->Set(context, get_symbol, func);
            const auto newProxy = v8::Proxy::New(context, jsObj, jsObj).ToLocalChecked();
            
            PyObject *sys_ = PyImport_ImportModule("sys");

            args.GetReturnValue().Set(newProxy);

            //static_cast<uint32_t>(955));
        }

        v8::String::Utf8Value utf_str_2(isolate, modulePath);

        auto pythonPath = std::string(*utf_str_2);
        printf("@[%s]", pythonPath.c_str());

        // Py_INCREF(sys_);
    }



    void init_pythonNode()
    {
        start();
    }

    void Initialize(Local<Object> target,
                    Local<Value> unused,
                    Local<Context> context,
                    void *priv)
    {
        init_pythonNode();

        plynth::WeakValueMap *a = nullptr;
        node::Environment *env = node::Environment::GetCurrent(context);
        Isolate *isolate = env->isolate();

        env->SetMethod(target, "call", StartPythonScript);
        env->SetMethod(target, "importModule", importPythonModule);
    }

} // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(python, ::python_node::Initialize)
