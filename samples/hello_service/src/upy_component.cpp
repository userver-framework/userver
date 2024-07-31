#include "upy_component.hpp"

#include <userver/logging/log.hpp>

#include <stdexcept>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace upython {

Component::Component(
    [[maybe_unused]] const userver::components::ComponentConfig& config,
    [[maybe_unused]] const userver::components::ComponentContext& context)
    : userver::components::LoggableComponentBase{config, context} {
#if 0
  /* Add a built-in module, before Py_Initialize */
  if (PyImport_AppendInittab("foo", PyInit_foo) == -1) {
    fprintf(stderr, "Error: could not extend in-built modules table\n");
    exit(1);
  }
#endif

  Py_Initialize();
  PyObject* pName = nullptr;

  pName = PyUnicode_FromString("upy");
  pModule_ = PyImport_Import(pName);

  Py_DECREF(pName);

  if (!pModule_) {
    PyErr_Print();
    throw std::runtime_error("oops");
  }

  PyObject* pFunc = PyObject_GetAttrString(pModule_, "init");

  if (!pFunc) {
    PyErr_Print();
    Py_DECREF(pModule_);
    throw std::runtime_error("oops");
  }

  PyObject* pResult = PyObject_CallFunction(pFunc, "s", "Hello, world!");

  Py_XDECREF(pFunc);

  if (!pResult) {
    PyErr_Print();
    throw std::runtime_error("oops");
  }

  const char* retval = PyUnicode_AsUTF8(pResult);

  if (!retval) {
    PyErr_Print();
    throw std::runtime_error("oops");
  }

  LOG_INFO() << "Python said: " << retval;

  Py_XDECREF(pResult);

  //RunScript();
}

std::string Component::RunScript() {
  std::string result;

  {
    std::lock_guard<userver::engine::Mutex> lock(mutex_);

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pFunc = PyObject_GetAttrString(pModule_, "run_script");

    if (!pFunc) {
      PyErr_Print();
      PyGILState_Release(gstate);
      throw std::runtime_error("oops");
    }

    PyObject* pResult = PyObject_CallNoArgs(pFunc);
    Py_DECREF(pFunc);

    if (!pResult) {
      PyErr_Print();
      PyGILState_Release(gstate);
      throw std::runtime_error("oops");
    }

    const char* retval = PyUnicode_AsUTF8(pResult);

    result = retval;
    Py_DECREF(pResult);
PyGILState_Release(gstate);  }

  return result;
}

}  // namespace upython
