
class D:
  def __init__(self, *args, **kwargs):
      self.kwargs = kwargs
      self.args = args
  
  def __enter__(self):
    return self

  def __exit__(self, ex_type, ex_value, trace):
    pass

def error_out(message):
    from plynth import js as jg
     
    jg.console.error(str(message))


def error_trace(val, val_traceback):
    import traceback
    from plynth import js

    if val_traceback:
        tb = traceback.format_tb(val_traceback)
        if tb and len(tb) > 0:
            js.console.error(str(val) +": " + tb[len(tb)-1])

def print_line_number(fut):
    def __print_line_number(fut):
        import traceback
        from plynth import js

        if fut.exception:
            exc = fut.exception()
            if exc:
                if exc.__traceback__:
                    tb = traceback.format_tb(exc.__traceback__)
                    if tb and len(tb) > 1:
                        js.console.error(str(exc) +": " + tb[len(tb)-1])
    
    if fut.add_done_callback:
        fut.add_done_callback(__print_line_number)

async def wrap_coro(coro):
    from plynth import js as jg
    import sys
    import traceback
    try:
        return await coro
    except Exception as e:
        #caller = getframeinfo(stack()[1][0])
        #filename = caller.filename
        #jg.console.error(filename + str(caller.lineno))

        #jg.console.error(str(err))
        #t, v, tb = sys.exc_info()
        #jg.console.info(traceback.format_exception(t,v,tb))
        if e.__traceback__:
            tb = traceback.format_tb(e.__traceback__)
            if tb and len(tb) > 1:
                jg.console.error(str(e) +": " + tb[len(tb)-1])



def background(func):
    from functools import wraps

    @wraps(func)
    def wrapper(*args, **kwargs):
        import plynth
        from plynth import js
        try:
            return plynth.runb(func, {}, *args, **kwargs)
        except Exception as err:
            js.console.error(str(err))

    return wrapper


async def wait_for(future):
    #async def wait_for2(future):
    return await future
    #return wait_for2().__await__()
    
    #return await future

def js_error():
    import sys
    info = sys.exc_info()[1]
    if info:
        return info.args[0]["obj"]

    return None

def print_warn(*mes):
    import os
    from plynth import js as jg
    from inspect import currentframe, getframeinfo, stack


    try:
        caller = getframeinfo(stack()[1][0])
        filename = caller.filename

        if len(filename) > 30:
            try:
                filename = os.path.join(os.path.basename(os.path.dirname(filename)), os.path.basename(filename))
            finally:
                pass

        jg.console.error(str(" ".join(map(str, mes)))
            + "\t\t["
            + str(filename)+ "(" + str(caller.lineno) + ")]"
        )
    finally:
        pass

def print_override(*args, **kwargs):
    import os
    import plynth
    from plynth import js as jg
    from inspect import currentframe, getframeinfo, stack
    
    if len(args) < 1:
        return

    mes = args[0]

    try:
        caller = getframeinfo(stack()[1][0])
        filename = caller.filename

        if len(filename) > 30:
            try:
                filename = os.path.join(os.path.basename(os.path.dirname(filename)), os.path.basename(filename))
                if len(filename) > 20:
                    filename = os.path.basename(filename)

            except Exception as ex:

                pass

        if plynth.is_jsvalue(mes):
            jg.console.info(mes)
            jg.console.info("\t[" + str(filename) + "(" + str(caller.lineno) + ")]")
            pass

        else:
            jg.console.info(str(mes)#str(" ".join(map(str, mes)))
                + "\t\t[" + str(filename) + "(" + str(caller.lineno) + ")]")
    finally:
        pass

def add_venv_localpath(topdir=None):
    import sys
    import os
    import platform
    import plynth
    from plynth import js as jg
    
    platform_system = platform.system()

    if topdir is None:
        topdir = jg.__dirname
    else:
        topdir = os.path.join(jg.__dirname, topdir)

    if platform_system == "Windows":
        sys.path.append(os.path.join(topdir, ".venv/Lib/site-packages"))
    else:
        sys.path.append(os.path.join(topdir, ".venv/lib/python" + str(sys.version_info.major) + "." + str(sys.version_info.minor) + "/site-packages"))


class JsExceptionHolder:
    def __init__(self):
        self.err_str = ""
        pass

#this makes it possible to python's await waits js's promise
class JsPromiseWrapper:
    def __init__(self, obj):
        self._obj = obj


    async def coro(self):
        import asyncio
        #self._future.add_done_callback(done_cb)

        fut = asyncio.Future()

        # asyncio.ensure_future(self._future)
        def resolver(arg):
            fut.set_result(arg)

        def catcher(arg):
            from plynth import JsError
            #fut.set_exception(JsError("argfojaiwefaowjfoawjioow"))
            fut.set_exception(JsError(dict(obj=arg)))
            # JsError(dict(obj=arg)))

            #raise Exception(str(arg))
            #fut.set_result(arg)

        self._obj.then(resolver, catcher)
        #self._obj.catch(catcher)

        return await fut

    def __await__(self):
        import asyncio
        
        return self.coro().__await__()

class ThreadRunner:
    def __init__(self):
        pass

# class Executor(concurrent.futures.Executor):
#     def __init__(self):
#         pass
class BackgroundTaskRunner:
    def __init__(self, function, *args, **kwargs):
        import asyncio
        self._function = function
        self._args = args
        self._kwargs = kwargs

        self._result = None
        self._exception = None
        self._finished = False
        self._future = asyncio.Future()
        self._future._is_plynth = True


    def invoke(self):
        from plynth import js as jg

        try:
            if self._kwargs:
                self._result = self._function(*self._args, **self._kwargs)
            else:
                self._result = self._function(*self._args)

        except Exception as err:
            self._exception = err

        self._finished = True
    
    def invoke_on_finished(self):
        from plynth import js as jg

        if self._exception:
            self._future.set_exception(self._exception)
        else:
            self._future.set_result(self._result)

    def get_future(self):
        return self._future

#class JsUvEventLoop(asyncio.ProactorEventLoop):
#class JsUvEventLoop(asyncio.SelectorEventLoop):
class JsUvEventLoop(asyncio.AbstractEventLoop):
    def __init__(self):
        from plynth import js as jg
        import asyncio


        #self.set_debug(True) 
        self._time = self.current_time()
        self._running = False
        self._invoking = False
        self._immediate = []
        self._prev_exc_str = ""


        self._later_dict = dict()
        self._later_index = 0

        self._exc = None

        # try:
        #     asyncio.SelectorEventLoop.__init__(self)
        #     pass
        # except Exception as err:
        #     jg.console.info(str(err))
        #     pass

    def set_debug(self, enable):
        pass
        
    def get_debug(self):
        # A silly hangover from the way asyncio is written
        return False

    def time(self):
        return self._time

    def ensure_future(self, coro):
        import asyncio
        import types


        if isinstance(coro, types.CoroutineType):
            asyncio.ensure_future(coro)
        self.invokeNext()

    def current_time(self):
        import time
        return time.monotonic() * 1000
        #return time.time() * 1000

    def invokeNext2(self):
        pass

    def invokeNext(self, later_index=None):
        '''
        later_index: index id generated when call_later called
        '''
        from plynth import js as jg
        #import plynth_knic

        self._running = True

        if self._invoking:
            if later_index is not None:
                self._pendings.append(later_index)
            else:
                self._soon_pending = True

            return

        self._pendings = []
        self._soon_pending = False
        self._invoking = True

        self._time = self.current_time()

        try:
            while (self._immediate
                   or bool(self._later_dict)) and self._running:

                self._time = self.current_time()
                h = None

                if self._immediate:
                    # jg.console.info("self._immediate")

                    h = self._immediate[0]

                    self._immediate = self._immediate[1:]
                else:

                    #if len(self._later_dict) > 30:
                    # jg.console.info(len(self._later_dict))

                    if later_index is not None:
                        if later_index in self._later_dict:
                            h = self._later_dict[later_index]
                            del self._later_dict[later_index]

                            #console.info("self._time = " + str(self._time))
                            #console.info("h._when = " + str(h._when))
                            #console.info("diff = " + str(self._time - h._when))

                            h._scheduled = False  # just for asyncio.TimerHandle debugging?

                if h is not None:
                    if not h._cancelled:
                        try:
                            h._run()
                        except Exception as err:
                            jg.console.info("except")
                            pass
               
                        pass

                else:
                    break
        except:
            #console.info(traceback.format_exc())
            pass
        finally:
            self._invoking = False
            pass

        pendings = self._pendings

        if self._soon_pending:
            self.invokeNext()

        if pendings:
            for index in pendings:
                # jg.console.info("do later pending" + str(index))

                self.invokeNext(index)

        pass

    def invokeWithCallBack(self):
        print("")

    def run_forever(self):
        self.invokeNext()

    def run_until_complete(self, future):
        pass

    def _timer_handle_cancelled(self, handle):
        pass

    def is_running(self):
        return self._running

    def is_closed(self):
        return not self._running

    def stop(self):
        self._running = False

    def close(self):
        self._running = False

    def shutdown_asyncgens(self):
        pass

    def get_exception_handler(self):
        return self.default_exception_handler

    def set_exception_handler(self, handler):
        pass


    def call_exception_handler(self, context):
        #self.get_exception_handler()(context)
        pass

    # Methods for scheduling jobs.
    #
    # If the job is a coroutine, the end-user should call asyncio.ensure_future(coro()).
    # The asyncio machinery will invoke loop.create_task(). Asyncio will then
    # run the coroutine in pieces, breaking it apart at async/await points, and every time it
    # will construct an appropriate callback and call loop.call_soon(cb).
    #
    # If the job is a plain function, the end-user should call one of the loop.call_*()
    # methods directly.

    #def call_soon_threadsafe(self, callback, *args, context=None):
        #self.call_soon(callback, *args, context)

    def call_soon(self, callback, *args, context=None):
        import asyncio
        #from js_top_objects import Math, console

        #console.info(repr(callback))
        h = asyncio.Handle(callback, args, self)
        self._immediate.append(h)
        self.invokeNext()
        return h

    def call_later(self, delay, callback, *args, context=None):
        import time
        if delay < 0:
            delay = 0.1

        self._time = self.current_time()
        return self.__call_at(
            self._time + delay * 1000, callback, *args, context=context)

    def call_at(self, when, callback, *args, context=None):
        import time
        self._time = self.current_time()
        if when < self._time:
            raise Exception("Can't schedule in the past in at")

        return self.__call_at(when, callback, *args, context=context)

    def __call_at(self, when, callback, *args, context=None):
        import heapq
        import asyncio
        import plynth_knic
        import time

        h = asyncio.TimerHandle(when, callback, args, self)

        next_requrest_time = h._when
        self._later_index += 1
        later_index = self._later_index

        self._later_dict[later_index] = h

        plynth_knic.later_request(later_index,
                                   next_requrest_time - self._time)

        h._scheduled = True  # perhaps just for debugging in asyncio.TimerHandle?

        return h

    def create_task(self, coro):
        import asyncio
        from plynth import js as jg
        from inspect import currentframe, getframeinfo, stack

        async def wrapper():
            try:
                await coro
            except Exception as e:
                #print("Wrapped exception")
                jg.console.info("Wrapped exception")
                self._exc = e
                jg.console.info(str(e))#"Wrapped exception")
                #raise e

        #task = asyncio.Task(wrapper(), loop=self)
        task = asyncio.Task(coro, loop=self)
        #self.invokeNext()

        return task

    def create_future(self):
        import asyncio
        # Not sure why this is here rather than in AbstractEventLoop.
        fut = asyncio.Future(loop=self)
        #self.invokeNext()

        return fut

class JsUndefined:
    def __init__(self):
        pass

undefined_obj = JsUndefined()
loop = JsUvEventLoop()
asyncio.set_event_loop(loop)
