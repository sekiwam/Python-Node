#include "node_ref.h"

#include <cstdint>
#include <Python.h>
#include <structmember.h>
#include "WeakValueMap.h"

#include <stdio.h>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>
#include <system_error>


#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#include "CustomModules.h"

#include "PyVars.h"
#include "JsVars.h"

#if defined(OS_WIN)
#endif

#if defined(OS_LINUX)
#include <sstream>
#include <unistd.h>
#endif

#if defined(OS_MACOSX)
#include <vector>
#include <mach-o/dyld.h>
#endif


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#elif __linux__
#include <sstream>
#include <unistd.h>
// linux
#elif __unix__ // all unices not caught above
constexpr OS_TYPE os_type = OS_TYPE::UNIX;
// Unix
#elif defined(_POSIX_VERSION)
// POSIX
#else
#   error "Unknown compiler"
#endif


using namespace v8;






static Isolate *_isolate = nullptr;

static PyThreadState * _pythread_state = nullptr;

static Global<Object> _global_obj;

static std::string _python_module_name;
static std::string _func_name;

static std::string _dirname;


static void _start_python();

void _on_keeper_async_handler(uv_async_t* handle);
void keeper_timer_handler(uv_timer_t *timer);

static std::string _exec_path;




void _on_init_uv_async(uv_async_t* handle)
{
    uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle2) {
        delete handle2;
    });
}

std::string set_var_resolver(Local<Object> func)
{
    _isolate = v8::Isolate::GetCurrent();

    JsVars::getInstance()->_js_toplevel_var_getter.Reset(_isolate, func);
    return "foiw";
}

std::string get_plynth_version()
{
    return CustomModuleManager::get_version();
}



// -- Expose api --
void set_execPath(const char* exec_path, const char* dirname) {
    _exec_path = exec_path;

    if (dirname) {
        _dirname = dirname;
        if (_dirname.empty() || _dirname.length() < 2) {
            _dirname = std::string();
        }
    }

}

std::string startPythonBinding(Local<Object> global_obj, const char* python_module, const char* func_name)
{
    _isolate = v8::Isolate::GetCurrent();

    _global_obj.Reset(_isolate, global_obj);
    JsVars::getInstance()->Init(_isolate, global_obj);

    if (python_module) {
        _python_module_name = std::string(python_module);
    }

    if (func_name) {
        _func_name = std::string(func_name);
    }

    uv_loop_t *loop = uv_default_loop();

    // set timer to keep loop running
    auto *timer1 = new uv_timer_t;
    uv_timer_init(loop, timer1);
    uv_timer_start(timer1, keeper_timer_handler, 0, 4 * 1000);


    //const auto async1 = new uv_async_t;
    //uv_async_init(loop, async1, _on_comp);
    //uv_async_send(async1);
    _start_python();


    //uv_loop_t *loop = uv_default_loop();
    auto *async1 = new uv_async_t;
    uv_async_init(loop, async1, _on_init_uv_async);
    uv_async_send(async1);

    return "1";
}


/*

window.document.addEventListener('DOMContentLoaded', function domContentLoaded(event) {
  try {
    require("plynth").plynth_register();
  } catch(e) {
    console.error(e);
  }

  try {
    window.document.removeEventListener('DOMContentLoaded', domContentLoaded);

  } catch(e) {
    console.error(e);
  }
});


*/
static v8::Global<v8::Function> _contendLoad;

void _plynth_register() {
    auto *isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();
    auto global = context->Global();


    //let scripts = document.getElementsByTagName("script");
    //		for (let i = 0; i < scripts.length; i++) {
    //			let scr = scripts.item(i);
    //			let scriptType = scr.getAttribute("type");
    //			if (scriptType == "text/python") {
    //				let module = scr.getAttribute("module") || scr.getAttribute("import");
    //				if (module) {
    //					let def_name = scr.getAttribute("def") || scr.getAttribute("call");
    //					require("plynth").start(module, def_name);
    //				}
    //			}
    //		}
    //
    auto document_name = v8::String::NewFromUtf8(isolate, "document", v8::NewStringType::kNormal).ToLocalChecked();
    auto maybe_document = global->Get(context, document_name);
    if (!maybe_document.IsEmpty() && maybe_document.ToLocalChecked()->IsObject()) {
        auto document = maybe_document.ToLocalChecked().As<Object>();


        auto getElementsByTagName_name = v8::String::NewFromUtf8(isolate, "getElementsByTagName", v8::NewStringType::kNormal);
        auto maybe_getElementsByTagName = document.As<Object>()->Get(context, getElementsByTagName_name.ToLocalChecked());
        if (!maybe_getElementsByTagName.IsEmpty() && maybe_getElementsByTagName.ToLocalChecked()->IsFunction()) {

            Local<Value> args[] = { String::NewFromUtf8(isolate, "script", v8::NewStringType::kNormal).ToLocalChecked() };

            auto getElementsByTagName = maybe_getElementsByTagName.ToLocalChecked().As<Function>();
            auto res = getElementsByTagName->Call(context, document, 1, args);
            if (res.IsEmpty() == false && res.ToLocalChecked()->IsObject()) {

                auto scripts = res.ToLocalChecked().As<Object>();

                uint32_t len = 0;

                auto item_name = v8::String::NewFromUtf8(isolate, "item", v8::NewStringType::kNormal);
                auto item = scripts.As<Object>()->Get(context, item_name.ToLocalChecked());

                if (item.IsEmpty() == false && item.ToLocalChecked()->IsFunction()) {
                    auto length_name = v8::String::NewFromUtf8(isolate, "length", v8::NewStringType::kNormal);
                    auto item = scripts.As<Object>()->Get(context, length_name.ToLocalChecked());
                    if (item.IsEmpty() == false && item.ToLocalChecked()->IsNumber()) {
                        auto num = item.ToLocalChecked()->ToNumber(context);
                        if (num.IsEmpty() == false) {
                            len = num.ToLocalChecked()->Int32Value(context).ToChecked();
                        }
                    }
                }

                for (uint32_t i = 0; i < len; i++) {
                    Local<Value> args[] = { Number::New(isolate, i) };
                    auto script = item.ToLocalChecked().As<Function>()->Call(context, scripts, 1, args);

                    if (script.IsEmpty() == false && script.ToLocalChecked()->IsObject()) {
                        auto scriptObj = script.ToLocalChecked().As<Object>();
                        auto maybe_getAttribute = scriptObj->Get(context, String::NewFromUtf8(isolate, "getAttribute", v8::NewStringType::kNormal).ToLocalChecked());

                        if (maybe_getAttribute.IsEmpty() == false && maybe_getAttribute.ToLocalChecked()->IsFunction()) {
                            auto getAttribute = maybe_getAttribute.ToLocalChecked().As<Function>();

                            auto type_name = v8::String::NewFromUtf8(isolate, "type", v8::NewStringType::kNormal).ToLocalChecked();
                            auto text_python = v8::String::NewFromUtf8(isolate, "text/python", v8::NewStringType::kNormal).ToLocalChecked();
                            auto module_name = v8::String::NewFromUtf8(isolate, "module", v8::NewStringType::kNormal).ToLocalChecked();
                            auto import_name = v8::String::NewFromUtf8(isolate, "import", v8::NewStringType::kNormal).ToLocalChecked();
                            auto def_name = v8::String::NewFromUtf8(isolate, "def", v8::NewStringType::kNormal).ToLocalChecked();
                            auto call_name = v8::String::NewFromUtf8(isolate, "call", v8::NewStringType::kNormal).ToLocalChecked();

                            // type
                            Local<Value> args[] = { type_name };
                            auto type_arg = getAttribute->Call(context, scriptObj, 1, args);
                            if (type_arg.IsEmpty() == false && type_arg.ToLocalChecked()->IsString()) {
                                auto is_text_python = type_arg.ToLocalChecked()->Equals(context, text_python);
                                if (is_text_python.IsJust() && is_text_python.ToChecked()) {

                                    std::string  python_module;
                                    std::string func_name;

                                    {
                                        Local<Value> attrs[] = { module_name };
                                        auto module_arg = getAttribute->Call(context, scriptObj, 1, attrs);
                                        if (module_arg.IsEmpty() == false && module_arg.ToLocalChecked()->IsString()) {
                                            v8::String::Utf8Value utf_str(isolate, module_arg.ToLocalChecked());

                                            python_module = *utf_str;
                                        }
                                    }

                                    if (python_module.empty()) {
                                        Local<Value> attrs[] = { import_name };
                                        auto arg = getAttribute->Call(context, scriptObj, 1, attrs);
                                        if (arg.IsEmpty() == false && arg.ToLocalChecked()->IsString()) {
                                            v8::String::Utf8Value utf_str(isolate, arg.ToLocalChecked());

                                            python_module = *utf_str;
                                        }
                                    }


                                    {
                                        Local<Value> attrs[] = { call_name };
                                        auto module_arg = getAttribute->Call(context, scriptObj, 1, attrs);
                                        if (module_arg.IsEmpty() == false && module_arg.ToLocalChecked()->IsString()) {
                                            v8::String::Utf8Value utf_str(isolate, module_arg.ToLocalChecked());
                                            func_name = *utf_str;
                                        }
                                    }

                                    if (func_name.empty()) {
                                        Local<Value> attrs[] = { def_name };
                                        auto arg = getAttribute->Call(context, scriptObj, 1, attrs);
                                        if (arg.IsEmpty() == false && arg.ToLocalChecked()->IsString()) {
                                            v8::String::Utf8Value utf_str(isolate, arg.ToLocalChecked());

                                            func_name = *utf_str;
                                        }
                                    }

                                    if (python_module.empty() == false) {
                                        startPythonBinding(global, python_module.c_str(), func_name.c_str());
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //v8::String::Utf8Value utf_str(isolate, execPath);// jsobj->ToString());

            //dirName.assign(*utf_str);// v8::String::Utf8Value(dName));

//				PyVars::exec_path = PyUnicode_FromString(*utf_str);
        }
    }


}


void python_node_register() {
    auto *isolate = v8::Isolate::GetCurrent();
    auto context = isolate->GetCurrentContext();
    auto global = context->Global();


    // We should check more than one call for memory leak?
    auto jsfunc = [](const FunctionCallbackInfo<Value>& info) {

        _plynth_register();
        
        if (_contendLoad.IsEmpty() == false) {

            auto *isolate = v8::Isolate::GetCurrent();
            auto context = isolate->GetCurrentContext();
            auto global = context->Global();

            auto func = _contendLoad.Get(isolate);

            {
                auto document_name = v8::String::NewFromUtf8(isolate, "document", v8::NewStringType::kNormal).ToLocalChecked();
                auto maybe_document = global->Get(context, document_name);
                if (!maybe_document.IsEmpty() && maybe_document.ToLocalChecked()->IsObject()) {
                    auto document = maybe_document.ToLocalChecked().As<Object>();

                    auto removeEventListener_name = v8::String::NewFromUtf8(isolate, "removeEventListener", v8::NewStringType::kNormal);
                    auto maybe_removeEventListener = document.As<Object>()->Get(context, removeEventListener_name.ToLocalChecked());
                    if (!maybe_removeEventListener.IsEmpty() && maybe_removeEventListener.ToLocalChecked()->IsFunction()) {

                        Local<Value> args[] = {
                            String::NewFromUtf8(isolate, "DOMContentLoaded", v8::NewStringType::kNormal).ToLocalChecked(),
                            func
                        };

                        auto removeEventListener = maybe_removeEventListener.ToLocalChecked().As<Function>();
                        removeEventListener->Call(context, document, 2, args).IsEmpty();
                    }
                }
            }

            _contendLoad.Reset();
        }
    };

    auto passData = String::NewFromUtf8(isolate, "abc", v8::NewStringType::kNormal).ToLocalChecked();
    auto func = v8::Function::New(context, jsfunc, passData).ToLocalChecked();

    _contendLoad.Reset(isolate, func);
    {
        auto document_name = v8::String::NewFromUtf8(isolate, "document", v8::NewStringType::kNormal).ToLocalChecked();
        auto maybe_document = global->Get(context, document_name);
        if (!maybe_document.IsEmpty() && maybe_document.ToLocalChecked()->IsObject()) {
            auto document = maybe_document.ToLocalChecked().As<Object>();


            bool already = false;
            auto readyState_str = v8::String::NewFromUtf8(isolate, "readyState", v8::NewStringType::kNormal).ToLocalChecked();

            auto readyState = document->Get(context, readyState_str);
            if (!readyState.IsEmpty() && readyState.ToLocalChecked()->IsString()) {
                auto ready = readyState.ToLocalChecked().As<String>();
                auto complete = v8::String::NewFromUtf8(isolate, "complete", v8::NewStringType::kNormal).ToLocalChecked();
                auto loaded = v8::String::NewFromUtf8(isolate, "loaded", v8::NewStringType::kNormal).ToLocalChecked();
                if (ready == complete || ready == loaded) {
                    already = true;
                }
            }

            //if (document.readyState == = "complete" || document.readyState == = "loaded") {
                // document is already ready to go
            //}


            if (already) {
                _plynth_register();
            }
            else {
                auto addEventListener_name = v8::String::NewFromUtf8(isolate, "addEventListener", v8::NewStringType::kNormal);
                auto maybe_addEventListener = document.As<Object>()->Get(context, addEventListener_name.ToLocalChecked());
                if (!maybe_addEventListener.IsEmpty() && maybe_addEventListener.ToLocalChecked()->IsFunction()) {

                    Local<Value> args[] = {
                        String::NewFromUtf8(isolate, "DOMContentLoaded", v8::NewStringType::kNormal).ToLocalChecked(),
                        func
                    };

                    auto addEventListener = maybe_addEventListener.ToLocalChecked().As<Function>();
                    addEventListener->Call(context, document, 2, args)
                        .IsEmpty();
                }
            }
           
        }
    }
}


//
//window.document.addEventListener('DOMContentLoaded', function domContentLoaded(event) {
//	try {
//		let scripts = document.getElementsByTagName("script");
//		for (let i = 0; i < scripts.length; i++) {
//			let scr = scripts.item(i);
//			let scriptType = scr.getAttribute("type");
//			if (scriptType == "text/python") {
//				let module = scr.getAttribute("module") || scr.getAttribute("import");
//				if (module) {
//					let def_name = scr.getAttribute("def") || scr.getAttribute("call");
//					require("plynth").start(module, def_name);
//				}
//			}
//		}
//
//		window.document.removeEventListener('DOMContentLoaded', domContentLoaded);
//
//	}
//	catch (e) {
//		console.error(e);
//	}
//});














// this should be called in the main thread
void pyErrorLogConsole() {
    if (PyErr_Occurred()) {
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);

        if (value && PyVars::error_trace) {
            auto *args = PyTuple_New(2);

            PyObject *arg0 = value == NULL ? Py_None : value;
            PyObject *arg1 = traceback == NULL ? Py_None : traceback;
            Py_INCREF(arg0);
            Py_INCREF(arg1);
            PyTuple_SetItem(args, 0, arg0);
            PyTuple_SetItem(args, 1, arg1);

            if (auto *ret = PyObject_CallObject(PyVars::error_trace, args)) {
                Py_DECREF(ret);
            }
            Py_DECREF(args);

        }

        /*
        if (value) {
            if (auto *str = PyObject_Str(value)) {
                auto *chs = PyUnicode_AsUTF8(str);

                auto *isolate = v8::Isolate::GetCurrent();
                auto global = isolate->GetCurrentContext()->Global();

                auto console = global->Get(String::NewFromUtf8(_isolate, "console"));
                if (console.IsEmpty() == false) {
                    auto consoleError = console.As<Object>()->Get(String::NewFromUtf8(_isolate, "error")).As<Function>();
                    if (consoleError.IsEmpty() == false) {
                        Local<Value> args[] = {
                            String::NewFromUtf8(_isolate, chs)
                        };
                        consoleError->Call(console, 1, args);
                    }

                }

                Py_DECREF(str);
            }
        }
        */

        PyErr_Clear();
    }
}


/*
static void timer_function(uv_timer_t *timer)
{
    printf("faowie");
    fflush(stdout);
    uv_close(reinterpret_cast<uv_handle_t*>(timer), [](uv_handle_t* handle2) {
        delete handle2;
    });
}
*/




//
//uv_loop_t* create_loop()
//{
//	uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
//	if (loop) {
//		uv_loop_init(loop);
//	}
//	return loop;
//}
//
//void signal_handler(uv_signal_t *handle, int signum)
//{
//	printf("Signal received: %d\n", signum);
//	uv_signal_stop(handle);
//}
//
//
//void aaaaaa(uv_timer_t *timer)
//{
//	printf("aaaaaaaa");
//	fflush(stdout);
//}



/*

// two signal handlers, each in its own loop
void thread2_worker(void *userp)
{
    uv_loop_t *loop2 = create_loop();
    //uv_loop_t *loop3 = create_loop();


    auto *timer1 = new uv_timer_t;
    uv_timer_init(loop2, timer1);
    uv_timer_start(timer1, aaaaaa, 440, 4 * 1000);


    while (uv_run(loop2, UV_RUN_DEFAULT)) { // UV_RUN_NOWAIT
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        printf("i");
        //fflush(stdout);
    }
}
*/


static char * Script1() { return NULL; }
static char * Script2() { return NULL; }


static void _start_python()
{



    bool first_call = true;
    if (_pythread_state == nullptr) {

        //uv_thread_t thread1, thread2;

        //uv_thread_create(&thread2, thread2_worker, 0);

        //uv_thread_join(&thread1);
        //uv_thread_join(&thread2);




//#if defined(OS_LINUX)
        unsigned int bufferSize = 512;

        std::vector<char> buffer(bufferSize + 1);
        // Get the process ID.
        int pid = getpid();

        // Construct a path to the symbolic link pointing to the process executable.
        // This is at /proc/<pid>/exe on Linux systems (we hope).
        std::ostringstream oss;
        oss << "/proc/" << pid << "/exe";
        std::string link = oss.str();

        int count = readlink(link.c_str(), &buffer[0], bufferSize);

        if (count == -1) {
            //			throw std::runtime_error("Could not read symbolic link"); 
                        //printf("not found AA");
                        //fflush(stdout);
        }
        else {
            buffer[count] = '\0';

            std::string ws = &buffer[0];
            //printf("str1 = %s", ws.c_str());
            //fflush(stdout);


            const auto lastSlashPos = ws.find_last_of('/'); // get a dir of execution file
            if (lastSlashPos != std::string::npos) {
                ws = ws.substr(0, lastSlashPos + 1);

                std::wstring base_path = std::wstring(ws.begin(), ws.end());
                std::wstring s = base_path + L"lib/python3.8:" + base_path + L"lib/python3.8/lib-dynload:" + base_path + L"lib/python3.8/site-packages";

                std::wstring shome = base_path + L"lib/python3.8";
                Py_SetPythonHome((wchar_t *)shome.c_str());

                Py_SetPath(s.c_str());
            }

            /*
            std::string::size_type loc = ws.find("electron", 0);
            if (loc != std::string::npos) {
                ws = ws.substr(0, loc);

                std::wstring s = std::wstring(ws.begin(), ws.end());
                s = s + L"lib/python3.7:" + s + L"lib/python3.7/lib-dynload:lib/python3.7:lib/python3.7/lib-dynload";

                Py_SetPath(s.c_str());
            }
            */
        }

//#endif

#if defined(OS_MACOSX)

        /*
        str1 = /Users/plynth/Downloads/plynth_devkit_workspace/__plynth/Plynth.app/Contents/Frameworks/Plynth Helper (Renderer).app/Contents/MacOS/Plynth Helper (Renderer)

        str1 = /Users/plynth/Downloads/plynth_devkit_workspace/__plynth/Plynth.app/Contents/Frameworks/Plynth Helper (Renderer).app/Contents/Frameworks/lib/python3.7

        ,str2 = /Users/plynth/Downloads/plynth_devkit_workspace/__plynth/Plynth.app/Contents/Frameworks/Plynth Helper (Renderer).app/Contents/Frameworks/lib/python3.7:/Users/plynth/Downloads/plynth_devkit_workspace/__plynth/Plynth.app/Contents/Frameworks/Plynth Helper (Renderer).app/Contents/Frameworks/lib/python3.7/site-packages:/Users/plynth/Downloads/plynth_devkit_workspace/__plynth/Plynth.app/Contents/Frameworks/Plynth Helper (Renderer).app/Contents/Frameworks/lib/python3.7/lib-dynload:lib/python3.7:lib/python3.7/lib-dynload


        */
        unsigned int bufferSize = 512;
        std::vector<char> buffer(bufferSize + 1);
        if (_NSGetExecutablePath(&buffer[0], &bufferSize))
        {
            buffer.resize(bufferSize);
            _NSGetExecutablePath(&buffer[0], &bufferSize);
        }

        std::string ws = &buffer[0];
        //printf("str1 = %s", (char*)ws.c_str());
        //fflush(stdout);
        std::string::size_type loc = ws.find("Plynth Helper.app/Contents/MacOS", 0);
        if (loc != std::string::npos) {
            ws = ws.substr(0, loc);
        }


        if (loc == std::string::npos) {
            loc = ws.rfind("/Contents/Frameworks"); // get a dir of execution file
            //loc = ws.find("/Contents/Frameworks", 0);
            if (loc != std::string::npos) {
                ws = ws.substr(0, loc);
                ws = ws + (std::string)("/Contents/Frameworks/"); // here is the @rpath, I guessPl	
                //ws = ws + (std::string)("/Contents/MacOS/"); // here is the @rpath, I guess
            }
        }

        if (loc == std::string::npos) {
            // Users / penguin / projects / 2_python / out / R / Electron.app / Contents / MacOS / Electron
            loc = ws.find("/Contents/MacOS", 0);
            if (loc != std::string::npos) {
                ws = ws.substr(0, loc);
                ws = ws + (std::string)("/Contents/Frameworks/"); // here is the @rpath, I guessPl	
                //ws = ws + (std::string)("/Contents/MacOS/"); // here is the @rpath, I guess
            }
        }


        std::wstring base_path = std::wstring(ws.begin(), ws.end());

        std::wstring s = base_path + L"lib/python3.7:" + base_path + L"lib/python3.7/site-packages:" + base_path + L"lib/python3.7/lib-dynload:lib/python3.7:lib/python3.7/lib-dynload";
        std::wstring shome = base_path + L"lib/python3.7";// +L"lib/python3.7/lib-dynload:lib/python3.7:lib/python3.7/lib-dynload";

        //printf("str1 = %ls", (wchar_t*)shome.c_str());
        //printf(",str2 = %ls", (wchar_t*)s.c_str());
        //fflush(stdout);


        //const wchar_t *inputw = shome.c_str();
        //char input[1024];
        //wcstombs(input, inputw, sizeof(wchar_t)*int(shome.size()));
        //setenv("PYTHONHOME", input, 0);
        //setenv("SSL_CERT_FILE", "/Users/plynth/Desktop/plynth_devkit_workspace/__plynth/Plynth.app/Contents/Frameworks/lib/python3.7/site-packages/certifi/cacert.pem", 1);
        //setenv("$PYTHONHOME", input, 0);


        Py_SetPythonHome((wchar_t *)shome.c_str());
        //Py_SetPythonHome(shome.c_str());

        Py_SetPath(s.c_str());


#endif





        auto localGlobal = _global_obj.Get(_isolate);


        CustomModuleManager::pre_register(localGlobal);

        if (Py_IsInitialized()) {

        }
        Py_InitializeEx(0);
        PyEval_InitThreads();

        auto context = _isolate->GetCurrentContext();


        if (_exec_path.empty()) {
            PyVars::exec_path = PyUnicode_FromString("");

            auto v8_dirname = v8::String::NewFromUtf8(_isolate, "process", v8::NewStringType::kNormal).ToLocalChecked();
            auto process = localGlobal.As<Object>()->Get(context, v8_dirname);
            

            if (!process.IsEmpty() && process.ToLocalChecked()->IsObject()) {

                auto execPathStr = v8::String::NewFromUtf8(_isolate, "execPath", v8::NewStringType::kNormal);

                auto execPath = process.ToLocalChecked().As<Object>()->Get(context, execPathStr.ToLocalChecked());

                if (!execPath.IsEmpty()) {
                    v8::String::Utf8Value utf_str(_isolate, execPath.ToLocalChecked());// jsobj->ToString());

                    //dirName.assign(*utf_str);// v8::String::Utf8Value(dName));
                    PyVars::exec_path = PyUnicode_FromString(*utf_str);
                }
            }
        }
        else {
            PyVars::exec_path = PyUnicode_FromString(_exec_path.c_str());
        }


        {
            // add current working directory
            PyObject *sys_ = PyImport_ImportModule("sys");
            if (auto *_path = PyObject_GetAttrString(sys_, "path")) {

                PyList_Append(_path, PyUnicode_FromString("."));

                // add package dir which includes index.html and package.json

                {
                    std::string dirName = _dirname;

                    if (dirName.empty() == true) {
                        auto v8_dirname = v8::String::NewFromUtf8(_isolate, "__dirname", v8::NewStringType::kNormal);
                        auto dName = localGlobal.As<Object>()->Get(context, v8_dirname.ToLocalChecked());
                        if (!dName.IsEmpty() && !dName.ToLocalChecked()->IsNullOrUndefined()) {
                            v8::String::Utf8Value utf_str(_isolate, dName.ToLocalChecked());// jsobj->ToString());

                            dirName.assign(*utf_str);// v8::String::Utf8Value(dName));
                        }
                    }

                    /*
                    if (dirName.empty()) {
                        std::string dirName = _dirname;
                    }
                    */

                    //printf("dir path = %s\n", dirName.c_str());
                    //fflush(stdout);

                    if (dirName.empty() == false) {
                        std::string currentPackageDir = dirName;



                        {
                            std::string target_str = currentPackageDir;

#if defined(OS_MACOSX)
                            const char * sep_str = "/";
#else
                            const char * sep_str = "\\";
#endif

                            auto lastOf = target_str.find_last_of(sep_str);
                            if (lastOf != std::string::npos) {
                                target_str = target_str.substr(0, lastOf);
                                PyList_Append(_path, PyUnicode_FromString(target_str.c_str()));
                                PyList_Append(_path, PyUnicode_FromString((target_str + sep_str + "pyapp.zip").c_str()));
                            }
                        }


                        PyList_Append(_path, PyUnicode_FromString(currentPackageDir.c_str()));

                        PyVars::package_dir = PyUnicode_FromString(currentPackageDir.c_str());
                    }
                }


                Py_DECREF(_path);
            }
            Py_DECREF(sys_);

        }

        PyVars::CustomScriptDict = PyDict_New();
        PyVars::pyfunction_dict = PyDict_New();
        PyVars::pyobj_dict = PyDict_New();
        

        CustomModuleManager::Init(localGlobal);

        //{
        //	auto wait_start_time = uv_hrtime();

        //	int g = 0;
        //	for (int i = 0; i < 1000000; i++) {
        //		auto j = i + 3.22;
        //		//g = j+5;
        //	}

        //	auto wait_start_time2 = uv_hrtime();
        //	CustomModuleManager::consoleInfo(std::to_string(g) + "here it is: " + std::to_string(wait_start_time2 - wait_start_time));
        //}
        //{

        //	static std::mutex main_sleeper_mutex;

        //	auto wait_start_time = uv_hrtime();

        //	for (int i = 0; i < 1000000; i++) {
        //		std::lock_guard<std::mutex> l(main_sleeper_mutex);
        //		int j = i + 2;
        //		//main_sleeper_mutex.unlock();

        //	}


        //	auto wait_start_time2 = uv_hrtime();
        //	CustomModuleManager::consoleInfo("here it is2:" + std::to_string(wait_start_time2 - wait_start_time));

        //}



        // PyObject *globals = PyEval_GetGlobals(); // we won't use it 

        //const auto python_lib_load_script_b = (std::string)(Script2());
        const auto python_lib_load_script_b = (std::string)(R"(
#[EMBED_PYTHON_SCRIPT_B]
)");

        const auto python_lib_load_script_a = (std::string)(R"(
#[EMBED_PYTHON_SCRIPT_A]
)");

        const auto python_lib_load_script = python_lib_load_script_b + 
            python_lib_load_script_a;



        PyObject *__main__ = PyImport_ImportModule("__main__");
        auto *globals = PyObject_GetAttrString(__main__, "__dict__");
        Py_DECREF(__main__);


        if (const PyObject *result = PyRun_String(python_lib_load_script.c_str(), Py_file_input, globals, PyVars::CustomScriptDict)) {
            Py_DECREF(result);
        }


        pyErrorLogConsole();
        



        auto *JsPromiseWrapper = PyDict_GetItemString(PyVars::CustomScriptDict, "JsPromiseWrapper");
        auto *BackgroundTaskRunner = PyDict_GetItemString(PyVars::CustomScriptDict, "BackgroundTaskRunner");

        auto *JsExceptionHolder = PyDict_GetItemString(PyVars::CustomScriptDict, "JsExceptionHolder");

        auto *add_venv_localpath = PyDict_GetItemString(PyVars::CustomScriptDict, "add_venv_localpath");
        auto *print_override = PyDict_GetItemString(PyVars::CustomScriptDict, "print_override");
        auto *js_error = PyDict_GetItemString(PyVars::CustomScriptDict, "js_error");
        auto *wrap_coro = PyDict_GetItemString(PyVars::CustomScriptDict, "wrap_coro");
        auto *d_block = PyDict_GetItemString(PyVars::CustomScriptDict, "d");

        auto *wait_for = PyDict_GetItemString(PyVars::CustomScriptDict, "wait_for");

        auto *print_warn = PyDict_GetItemString(PyVars::CustomScriptDict, "print_warn");
        auto *background_decorator = PyDict_GetItemString(PyVars::CustomScriptDict, "background");

        auto *print_line_number = PyDict_GetItemString(PyVars::CustomScriptDict, "print_line_number");
        auto *error_trace = PyDict_GetItemString(PyVars::CustomScriptDict, "error_trace");

        auto *create_js_class = PyDict_GetItemString(PyVars::CustomScriptDict, "create_js_class");
        auto *js_class = PyDict_GetItemString(PyVars::CustomScriptDict, "js_class");

        auto *js_undefined = PyDict_GetItemString(PyVars::CustomScriptDict, "undefined_obj");







        //const auto JsUvEventLoop = PyDict_GetItemString(PyVars::CustomScriptDict, "JsUvEventLoop");
        auto *loop = PyDict_GetItemString(PyVars::CustomScriptDict, "loop");


        pyErrorLogConsole();


        Py_XINCREF(loop);
        Py_XINCREF(JsPromiseWrapper);
        Py_XINCREF(BackgroundTaskRunner);
        Py_XINCREF(JsExceptionHolder);
        Py_XINCREF(add_venv_localpath);
        Py_XINCREF(print_override);
        Py_XINCREF(wait_for);
        Py_XINCREF(wrap_coro);
        Py_XINCREF(d_block);

        Py_XINCREF(js_error);

        Py_XINCREF(print_warn);
        Py_XINCREF(background_decorator);
        Py_XINCREF(print_line_number);
        Py_XINCREF(error_trace);
        Py_XINCREF(create_js_class);
        Py_XINCREF(js_class);

        Py_XINCREF(js_undefined);


        PyVars::JsPromiseWrapper = JsPromiseWrapper;
        PyVars::JsUvEventLoop = loop;
        PyVars::BackgroundTaskRunner = BackgroundTaskRunner;
        PyVars::JsExceptionHolder = JsExceptionHolder;
        PyVars::JsExceptionHolderTypeObject = Py_TYPE(PyObject_CallObject(JsExceptionHolder, nullptr));
        PyVars::add_venv_localpath = add_venv_localpath;
        PyVars::print_override = print_override;
        PyVars::js_error = js_error;
        PyVars::wait_for = wait_for;
        PyVars::wrap_coro = wrap_coro;
        PyVars::print_line_number = print_line_number;
        PyVars::error_trace = error_trace;

        PyVars::undefined = js_undefined;

        PyVars::d_block = d_block;

        PyVars::print_warn = print_warn;
        PyVars::create_js_class = create_js_class;
        PyVars::js_class = js_class;

        PyVars::background_decorator = background_decorator;
    }
    else {
        first_call = false;
    }

    _pythread_state = PyThreadState_Get();

    printf("ACB\n");


_python_module_name = "sys";
_func_name = "getcheckinterval";
    //if (_python_module_name.empty() == false) {

        std::string call_func_user_script = "";

        //if (_func_name.empty() == false) {
            call_func_user_script = (std::string)(R"(
message = )") + std::string(_python_module_name) + "." + std::string(_func_name) + std::string("()") +
(std::string)(R"(
async def demo():
    pass

jg.setTimeout(demo, 1)
#jg.setImmediate(demo)

if isinstance(message, types.CoroutineType):
    try:
        f = asyncio.ensure_future(message)
        #_custom_loop.invokeNext()
    except:
        message = traceback.format_exc()
        #console.info(traceback.format_exc())

)")
;
        //}

        const auto python_start_user_script = (std::string)(R"(
import os
import platform
import sys

import plynth
from plynth import js as jg, JsError

import multiprocessing as mp

platform_system = platform.system()

print("jifoew")
jg.console.info("230")
try:
	sys.modules['__plynth__'] = plynth
	if platform_system == "Windows":
		exec_path = plynth.exec_path()
		if exec_path:
			mp.set_executable(os.path.join(os.path.dirname(exec_path), 'pythonw.bin'))
	elif platform_system == "Darwin":
		pass
	else:
		pass
except:
	pass


import )") + std::string(_python_module_name) +
(std::string)(R"(
import types
import traceback
import asyncio
import time
import plynth_knic


)") + std::string(call_func_user_script) +
(std::string)(R"(
_custom_loop.run_forever()

)")
;

        PyObject *__main__ = PyImport_ImportModule("__main__");
        auto *globals = PyObject_GetAttrString(__main__, "__dict__");
        Py_DECREF(__main__);

        auto *localDict = PyDict_New();
        PyDict_SetItemString(localDict, "_custom_loop", PyVars::JsUvEventLoop);

        PyObject *result2 = PyRun_String(python_start_user_script.c_str(), 
        Py_file_input, globals, localDict);

        pyErrorLogConsole();

        //auto message = std::string(PyUnicode_AsUTF8(PyObject_Str(result2)));

        if (result2 != NULL) {
            Py_DECREF(result2);
        }
    //}
}

void my_cleanup(void* arg)
{
    //delete my_object_ptr; //call object dtor, or other stuff that needs to be cleaned up here
    _global_obj.Reset();//_isolate, global_obj);
    _contendLoad.Reset( );

    CustomModuleManager::Clean();
}

void start()
{
    _isolate = v8::Isolate::GetCurrent();

     auto context = _isolate->GetCurrentContext(); // no longer crashes
     auto global_obj = context->Global();
    _global_obj.Reset(_isolate, global_obj);

    JsVars::getInstance()->Init(_isolate, global_obj);

    python_node_register();

    //node::Environment *env = node::Environment::GetCurrent(context);
    node::AtExit(my_cleanup, nullptr);
    
    _start_python();
}



void _on_keeper_async_handler(uv_async_t* handle)
{
    uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle2) {
        delete handle2;
    });
}


void keeper_timer_handler(uv_timer_t *timer)
{
    uv_loop_t *loop = uv_default_loop();

    auto *async1 = new uv_async_t;
    uv_async_init(loop, async1, _on_keeper_async_handler);
    uv_async_send(async1);
}
