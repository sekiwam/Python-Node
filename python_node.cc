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

namespace python_node
{
    static void SafeGetenv(const FunctionCallbackInfo<Value> &args)
    {
        //args.GetReturnValue().Set(result);
        Py_InitializeEx(0);
        PyEval_InitThreads();
        PyObject *sys_ = PyImport_ImportModule("sys");

        args.GetReturnValue().Set(static_cast<uint32_t>(955));
        // Py_INCREF(sys_);
    }
    

    void ak()
     {
        //startPythonBinding(nullptr, module_path.c_str(), func_name.c_str());
        python_node_register();
    }


    void Initialize(Local<Object> target,
                    Local<Value> unused,
                    Local<Context> context,
                    void *priv)
    {
        /*
        static uv_once_t init_once = UV_ONCE_INIT;
        uv_once(&init_once, InitCryptoOnce);
        */

        WeakValueMap *a = nullptr;
        node::Environment *env = node::Environment::GetCurrent(context);
        Isolate *isolate = env->isolate();

        env->SetMethod(target, "call", SafeGetenv);
    }

} // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(python, ::python_node::Initialize)
