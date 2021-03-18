// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

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

namespace node {

using v8::Context;
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

namespace python {

static void SafeGetenv(const FunctionCallbackInfo<Value>& args) {
  //args.GetReturnValue().Set(result);
  Py_InitializeEx(0);
  PyEval_InitThreads();
  PyObject *sys_ = PyImport_ImportModule("sys");

  args.GetReturnValue().Set(static_cast<uint32_t>(2195));
  Py_INCREF(sys_);
}

void Initialize(Local<Object> target,
                Local<Value> unused,
                Local<Context> context,
                void* priv) {
	/*
  static uv_once_t init_once = UV_ONCE_INIT;
  uv_once(&init_once, InitCryptoOnce);
  */

  Environment* env = Environment::GetCurrent(context);
  Isolate* isolate = env->isolate();

  env->SetMethod(target, "call", SafeGetenv);

}

}  // namespace crypto
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(python, node::python::Initialize)
