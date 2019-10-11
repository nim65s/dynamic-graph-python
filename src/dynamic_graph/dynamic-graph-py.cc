// Copyright 2010, Florent Lamiraux, Thomas Moulard, LAAS-CNRS.

#include <Python.h>
#include <iostream>
#include <sstream>
#include <string>

#include <dynamic-graph/debug.h>
#include <dynamic-graph/exception-factory.h>
#include <dynamic-graph/signal-base.h>

#include "dynamic-graph/python/exception.hh"
#include "dynamic-graph/python/dynamic-graph-py.hh"
#include "dynamic-graph/python/signal-wrapper.hh"

namespace dynamicgraph {
namespace python {

/**
   \brief plug a signal into another one.
*/
PyObject* plug(PyObject* /*self*/, PyObject* args) {
  PyObject* objOut = NULL;
  PyObject* objIn = NULL;
  void* pObjOut;
  void* pObjIn;

  if (!PyArg_ParseTuple(args, "OO", &objOut, &objIn)) return NULL;

  if (!PyCapsule_CheckExact(objOut)) {
    PyErr_SetString(PyExc_TypeError,
                    "first argument should be a pointer to"
                    " signalBase<int>.");
    return NULL;
  }
  if (!PyCapsule_CheckExact(objIn)) {
    PyErr_SetString(PyExc_TypeError,
                    "second argument should be a pointer to"
                    " signalBase<int>.");
    return NULL;
  }

  pObjIn = PyCapsule_GetPointer(objIn, "dynamic_graph.Signal");
  SignalBase<int>* signalIn = (SignalBase<int>*)pObjIn;
  pObjOut = PyCapsule_GetPointer(objOut, "dynamic_graph.Signal");
  SignalBase<int>* signalOut = (SignalBase<int>*)pObjOut;
  std::ostringstream os;

  try {
    signalIn->plug(signalOut);
  }
  CATCH_ALL_EXCEPTIONS();
  return Py_BuildValue("");
}

PyObject* enableTrace(PyObject* /*self*/, PyObject* args) {
  PyObject* boolean;
  char* filename = NULL;

  if (PyArg_ParseTuple(args, "Os", &boolean, &filename)) {
    if (!PyBool_Check(boolean)) {
      PyErr_SetString(PyExc_TypeError,
                      "enableTrace takes as first "
                      "argument True or False,\n"
                      "           and as "
                      "second argument a filename.");
      return NULL;
    }
    if (PyObject_IsTrue(boolean)) {
      try {
        DebugTrace::openFile(filename);
      }
      CATCH_ALL_EXCEPTIONS();
    } else {
      try {
        DebugTrace::closeFile(filename);
      }
      CATCH_ALL_EXCEPTIONS();
    }
  } else {
    return NULL;
  }
  return Py_BuildValue("");
}

PyObject* error_out(
#if PY_MAJOR_VERSION >= 3
    PyObject* m, PyObject*
#else
    PyObject*, PyObject*
#endif
) {
  struct module_state* st = GETSTATE(m);
  PyErr_SetString(st->dgpyError, "something bad happened");
  return NULL;
}

}  // namespace python
}  // namespace dynamicgraph

#ifdef __cplusplus
extern "C" {
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_wrap(void)
#else
void initwrap(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
  PyObject* module = PyModule_Create(&dynamicgraph::python::dynamicGraphModuleDef);
#else
  PyObject* module = Py_InitModule("wrap", dynamicgraph::python::dynamicGraphMethods);
#endif

  if (module == NULL) INITERROR;
  struct dynamicgraph::python::module_state* st = GETSTATE(module);

  st->dgpyError = PyErr_NewException(const_cast<char*>("dynamic_graph.dgpyError"), NULL, NULL);
  if (st->dgpyError == NULL) {
    Py_DECREF(module);
    INITERROR;
  }

#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}

#ifdef __cplusplus
}  // extern "C"
#endif
