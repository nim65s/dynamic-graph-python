// -*- mode: c++ -*-
// Copyright 2011, Florent Lamiraux, CNRS.

#include <iostream>
#include "dynamic-graph/debug.h"
#include "dynamic-graph/python/interpreter.hh"
#include "dynamic-graph/python/link-to-python.hh"

std::ofstream dg_debugfile("/tmp/dynamic-graph-traces.txt", std::ios::trunc& std::ios::out);

// Python initialization commands
namespace dynamicgraph {
namespace python {
static const std::string pythonPrefix[8] = {"from __future__ import print_function\n",
                                            "import traceback\n",
                                            "class StdoutCatcher:\n"
                                            "    def __init__(self):\n"
                                            "        self.data = ''\n"
                                            "    def write(self, stuff):\n"
                                            "        self.data = self.data + stuff\n"
                                            "    def fetch(self):\n"
                                            "        s = self.data[:]\n"
                                            "        self.data = ''\n"
                                            "        return s\n",
                                            "stdout_catcher = StdoutCatcher()\n",
                                            "stderr_catcher = StdoutCatcher()\n",
                                            "import sys\n",
                                            "sys.stdout = stdout_catcher",
                                            "sys.stderr = stderr_catcher"};
}
}  // namespace dynamicgraph

namespace dynamicgraph {
namespace python {

// Get any PyObject and get its str() representation as an std::string
std::string obj_to_str(PyObject *o) {
    std::string ret;
    PyObject *os;
#if PY_MAJOR_VERSION >= 3
    os = PyObject_Str(o);
    assert(os != NULL);
    assert(PyUnicode_Check(os));
    ret = PyUnicode_AsUTF8(os);
#else
    os = PyObject_Unicode(o);
    assert(os != NULL);
    assert(PyUnicode_Check(os));
    PyObject *oss = PyUnicode_AsUTF8String(os);
    assert(oss != NULL);
    ret = PyString_AsString(oss);
    Py_DECREF(oss);
#endif
    Py_DECREF(os);
    return ret;
}

std::string HandleErr(PyObject *ctx) {
  dgDEBUGIN(15);
  std::string err = "";

  if (PyErr_Occurred() != NULL) {
      PyErr_Print();
      PyObject *stderr_obj = PyRun_String("stderr_catcher.fetch()", Py_eval_input, ctx, ctx);
      err = obj_to_str(stderr_obj);
      Py_DECREF(stderr_obj);
  } else {
    std::cout << "no object generated but no error occured." << std::endl;
  }
  return err;
}

Interpreter::Interpreter() {
  // load python dynamic library
  // this is silly, but required to be able to import dl module.
#ifndef WIN32
  dlopen(libpython.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
  Py_Initialize();
  PyEval_InitThreads();
  mainmod_ = PyImport_AddModule("__main__");
  Py_INCREF(mainmod_);
  globals_ = PyModule_GetDict(mainmod_);
  assert(globals_);
  Py_INCREF(globals_);
  PyRun_SimpleString(pythonPrefix[0].c_str());
  PyRun_SimpleString(pythonPrefix[1].c_str());
  PyRun_SimpleString(pythonPrefix[2].c_str());
  PyRun_SimpleString(pythonPrefix[3].c_str());
  PyRun_SimpleString(pythonPrefix[4].c_str());
  PyRun_SimpleString(pythonPrefix[5].c_str());
  PyRun_SimpleString(pythonPrefix[6].c_str());
  PyRun_SimpleString(pythonPrefix[7].c_str());
  PyRun_SimpleString("import linecache");

  traceback_format_exception_ =
      PyDict_GetItemString(PyModule_GetDict(PyImport_AddModule("traceback")), "format_exception");
  assert(PyCallable_Check(traceback_format_exception_));
  Py_INCREF(traceback_format_exception_);

  // Allow threads
  _pyState = PyEval_SaveThread();
}

Interpreter::~Interpreter() {
  PyEval_RestoreThread(_pyState);

  // Ideally, we should call Py_Finalize but this is not really supported by
  // Python.
  // Instead, we merelly remove variables.
  // Code was taken here: https://github.com/numpy/numpy/issues/8097#issuecomment-356683953
  {
    PyObject* poAttrList = PyObject_Dir(mainmod_);
    PyObject* poAttrIter = PyObject_GetIter(poAttrList);
    PyObject* poAttrName;

    while ((poAttrName = PyIter_Next(poAttrIter)) != NULL) {
      std::string oAttrName(obj_to_str(poAttrName));

      // Make sure we don't delete any private objects.
      if (oAttrName.compare(0, 2, "__") != 0 || oAttrName.compare(oAttrName.size() - 2, 2, "__") != 0) {
        PyObject* poAttr = PyObject_GetAttr(mainmod_, poAttrName);

        // Make sure we don't delete any module objects.
        if (poAttr && poAttr->ob_type != mainmod_->ob_type) PyObject_SetAttr(mainmod_, poAttrName, NULL);

        Py_DECREF(poAttr);
      }

      Py_DECREF(poAttrName);
    }

    Py_DECREF(poAttrIter);
    Py_DECREF(poAttrList);
  }

  Py_DECREF(mainmod_);
  Py_DECREF(globals_);
  Py_DECREF(traceback_format_exception_);
  // Py_Finalize();
}

std::string Interpreter::python(const std::string& command) {
  std::string lerr(""), lout(""), lres("");
  python(command, lres, lout, lerr);
  return lres;
}

void Interpreter::python(const std::string& command, std::string& res, std::string& out, std::string& err) {
  res = "";
  out = "";
  err = "";

  // Check if the command is not a python comment or empty.
  std::string::size_type iFirstNonWhite = command.find_first_not_of(" \t");
  // Empty command
  if (iFirstNonWhite == std::string::npos) return;
  // Command is a comment. Ignore it.
  if (command[iFirstNonWhite] == '#') return;

  PyEval_RestoreThread(_pyState);

  std::cout << command.c_str() << std::endl;
  PyObject* result_obj = PyRun_String(command.c_str(), Py_eval_input, globals_, globals_);
  if (result_obj == NULL) err = HandleErr(globals_);

  PyObject *stdout_obj = 0;
  stdout_obj = PyRun_String("stdout_catcher.fetch()", Py_eval_input, globals_, globals_);
  out = obj_to_str(stdout_obj);
  Py_DECREF(stdout_obj);
  // Local display for the robot (in debug mode or for the logs)
  if (out.size() != 0) std::cout << "Output:" << out << std::endl;
  if (err.size() != 0) std::cout << "Error:" << err << std::endl;
  // If python cannot build a string representation of result
  // then results is equal to NULL. This will trigger a SEGV
  dgDEBUG(15) << "For command: " << command << std::endl;
  if (result_obj != NULL) {
    res = obj_to_str(result_obj);
    dgDEBUG(15) << "Result is: " << res << std::endl;
    Py_DECREF(result_obj);
  } else {
      dgDEBUG(15) << "Result is: empty" << std::endl;
  }
  dgDEBUG(15) << "Out is: " << out << std::endl;
  dgDEBUG(15) << "Err is :" << err << std::endl;

  _pyState = PyEval_SaveThread();

  return;
}

void Interpreter::runPythonFile(std::string filename) {
  std::string err = "";
  runPythonFile(filename, err);
}

void Interpreter::runPythonFile(std::string filename, std::string& err) {
  FILE* pFile = fopen(filename.c_str(), "r");
  if (pFile == 0x0) {
    err = filename + " cannot be open";
    return;
  }

  PyEval_RestoreThread(_pyState);

  err = "";
  PyObject* run = PyRun_File(pFile, filename.c_str(), Py_file_input, globals_, globals_);
  if (run == NULL) {
    err = HandleErr(globals_);
    std::cerr << err << std::endl;
  }
  Py_DecRef(run);

  _pyState = PyEval_SaveThread();
  fclose(pFile);
}

void Interpreter::runMain(void) {
  PyEval_RestoreThread(_pyState);
#if PY_MAJOR_VERSION >= 3
  const Py_UNICODE* argv[] = {L"dg-embedded-pysh"};
  Py_Main(1, const_cast<Py_UNICODE**>(argv));
#else
  const char* argv[] = {"dg-embedded-pysh"};
  Py_Main(1, const_cast<char**>(argv));
#endif
  _pyState = PyEval_SaveThread();
}

std::string Interpreter::processStream(std::istream& stream, std::ostream& os) {
  char line[10000];
  sprintf(line, "%s", "\n");
  std::string command;
  std::streamsize maxSize = 10000;
#if 0
  while (line != std::string("")) {
    stream.getline(line, maxSize, '\n');
    command += std::string(line) + std::string("\n");
  };
#else
  os << "dg> ";
  stream.getline(line, maxSize, ';');
  command += std::string(line);
#endif
  return command;
}

}  // namespace python
}  // namespace dynamicgraph
