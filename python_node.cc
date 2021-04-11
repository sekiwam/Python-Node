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
    static void StartPythonScript(const FunctionCallbackInfo<Value> &args)
    {
        //args.GetReturnValue().Set(result);
        //Py_InitializeEx(0);
        //PyEval_InitThreads();
        //PyObject *sys_ = PyImport_ImportModule("sys");

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

            auto jsfunc = [](const FunctionCallbackInfo<Value> &info)
            {
                info.GetReturnValue().Set(300);
            };

            auto passData = String::NewFromUtf8(isolate, "abc", v8::NewStringType::kNormal).ToLocalChecked();
            auto func = v8::Function::New(context, jsfunc, passData).ToLocalChecked();
            const auto result = jsObj->Set(context, get_symbol, func);

            const auto newProxy = v8::Proxy::New(context, jsObj, jsObj).ToLocalChecked();

            args.GetReturnValue().Set(newProxy);

            //static_cast<uint32_t>(955));

        }

        v8::String::Utf8Value utf_str_2(isolate, modulePath);

        auto pythonPath = std::string(*utf_str_2);
        printf("@[%s]", pythonPath.c_str());

        // Py_INCREF(sys_);
    }

    fn awef()
    {
        int k = 1423;
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
    }

} // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(python, ::python_node::Initialize)
