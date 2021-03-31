import sys
import asyncio
import concurrent
import io


class Logger(io.IOBase):
    def __init__(self):
        pass

    def write(self, message):
        import sys
        import os
        import plynth
        from plynth import js as jg
        from inspect import currentframe, getframeinfo, stack

        if message and len(message.strip()) > 0:
            try:
                caller = getframeinfo(stack()[1][0])
                filename = caller.filename

                if len(filename) > 30:
                    try:
                        filename = os.path.join(os.path.basename(os.path.dirname(filename)), os.path.basename(filename))
                        if len(filename) > 20:
                            filename = os.path.basename(filename)

                    except Exception as e:
                        pass


                if plynth.is_jsvalue(message):
                    jg.console.info(message)
                    jg.console.info("\t[" + str(filename) + "(" + str(caller.lineno) + ")]")
                else:
                    jg.console.log(message
                        + "\t\t[" + str(filename) + "(" + str(caller.lineno) + ")]")
            
            except Exception as e:
                jg.console.error(str(e))
    
            
logger = Logger()

sys.stdout = logger
sys.stderr = logger


def js_class(_func=None, *, superclass=None, jsclassattr="JsClass", jssuperclassattr="JsSuperclass"):
    
    def decorator_name(arg_python_class):
        import plynth
        if superclass and isinstance(jssuperclassattr, str):
            setattr(arg_python_class, jssuperclassattr, superclass)

        new_js_class = plynth.create_js_class(arg_python_class, superclass)
        if new_js_class and isinstance(jsclassattr, str):
            setattr(arg_python_class, jsclassattr, new_js_class)

        #python_class.JsClass = CustomJsClass
        return arg_python_class

    if _func is None:
        return decorator_name
    else:
        return decorator_name(_func)


def js_class_inherit(superclass, jsclassattr="JsClass", jssuperclassattr="JsSuperclass"):
    if not jsclassattr:
        jsclassattr = "JsClass"

    def decorator_name(arg_python_class):
        import plynth
        if superclass and isinstance(jssuperclassattr, str):
            setattr(arg_python_class, jssuperclassattr, superclass)

        new_js_class = plynth.create_js_class(arg_python_class, superclass)
        if new_js_class:
            setattr(arg_python_class, jsclassattr, new_js_class)

        #python_class.JsClass = CustomJsClass
        return arg_python_class

    return decorator_name




def create_js_class(python_class, superclass=None):
    import plynth
    import plynth.js as js
    import types

    sym = js.Symbol("__pskey__")

    def callfunc(funcitem, args):
        import inspect

        if callable(funcitem):
            arg_length = len(args)#.length
            if funcitem.__code__:
                if funcitem.__code__.co_argcount > 0:
                    has_varargs = funcitem.__code__.co_flags & inspect.CO_VARARGS
                    if has_varargs == False:
                        co_argcount = funcitem.__code__.co_argcount - 1
                        if co_argcount < arg_length:
                            arg_length = co_argcount


            arg_list = []
            for i in range(arg_length):
                arg_list.append(args[i])

            return funcitem(*arg_list)


    def set_static_method(name, classobj, python_class):
        import plynth
        import plynth.js as js

        def func83(*args):
            funcitem = getattr(python_class, name)
            return callfunc(funcitem, args)
            
        js.Reflect.set(classobj, name, func83)


    def set_method(name, classobj):
        import plynth
        import plynth.js as js

        def func83(*args):
            import plynth
            import plynth.js as js

            this = plynth.js_this()
            if this:
                #keyobj = this[sym]
                keyobj = js.Reflect.get(this, sym)
                pyobj = plynth.resolve_pyholder(keyobj)

                funcitem = getattr(pyobj, name)
                return callfunc(funcitem, args)
        
        js.Reflect.set(classobj.prototype, name, func83)


    def first_constructor(*args):
        import plynth
        import plynth.js as js
        import inspect

        #context = plynth.js_context()
        this = plynth.js_this()# js_this
        #args = plynth.js_arguments()# js_arguments

        arg_length = len(args)#.length
        if python_class.__init__ and python_class.__init__.__code__:
            if python_class.__init__.__code__.co_argcount > 0:
                has_varargs = python_class.__init__.__code__.co_flags & inspect.CO_VARARGS
                if has_varargs == False:
                    co_argcount = python_class.__init__.__code__.co_argcount - 1
                    if co_argcount < arg_length:
                        arg_length = co_argcount


        arg_list = []
        for i in range(arg_length):
            arg_list.append(args[i])
        
        #js.console.info(arg_list)
        # inspect.isclass(python_class)
        pyobj = python_class(*arg_list)

        js.Object.defineProperty(this, sym, {
                'value': plynth.create_pyholder(pyobj),
                'writable': False,
                'enumerable': False,
                'configurable': False,
            }
        )

        #this[sym] = plynth.bindobject(pyobj)
    temp = js.Object()

    temp.CustomJsClass = js.Object(first_constructor)
    CustomJsClass = temp.CustomJsClass
    temp.CustomJsClass = None


    if superclass and superclass.prototype:
        CustomJsClass.prototype = js.Object.create(superclass.prototype)
        CustomJsClass.prototype.constructor = first_constructor

    for temp_name in dir(python_class):
        name = str(temp_name)
        if str(name).startswith("__") and str(name).endswith("__"):
            continue

        if isinstance(getattr(python_class, name), staticmethod):
            set_static_method(name, CustomJsClass, python_class)
        elif isinstance(getattr(python_class, name), classmethod):
            set_static_method(name, CustomJsClass, python_class)
        else:
            #isinstance(open, types.FunctionType)
            method = getattr(python_class, name)
            #if callable(method):
            if isinstance(method, (types.FunctionType, types.BuiltinFunctionType, types.MethodType, types.BuiltinMethodType)):
                #js.console.info(name)#dir(method.__code__))
                set_method(name, CustomJsClass)

    return CustomJsClass

