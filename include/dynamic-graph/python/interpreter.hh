// -*- mode: c++ -*-
// Copyright 2011, Florent Lamiraux, CNRS.

#ifndef DYNAMIC_GRAPH_PYTHON_INTERPRETER_H
#define DYNAMIC_GRAPH_PYTHON_INTERPRETER_H

#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string>
#include "dynamic-graph/python/api.hh"
#include "dynamic-graph/python/deprecated.hh"

#include "dynamic-graph/python/api.hh"

namespace dynamicgraph {
namespace python {
///
/// This class implements a basis python interpreter.
///
/// String sent to method python are interpreted by an onboard python
/// interpreter.
class DYNAMIC_GRAPH_PYTHON_DLLAPI Interpreter {
 public:
  Interpreter();
  ~Interpreter();
  /// \brief Method to start python interperter.
  /// \param command string to execute
  /// Method deprecated, you *SHOULD* handle error messages.
  DYNAMIC_GRAPH_PYTHON_DEPRECATED std::string python(const std::string& command);

  /// \brief Method to start python interperter.
  /// \param command string to execute, result, stdout, stderr strings
  void python(const std::string& command, std::string& result, std::string& out, std::string& err);

  /// \brief Method to exectue a python script.
  /// \param filename the filename
  void runPythonFile(std::string filename);
  void runPythonFile(std::string filename, std::string& err);
  void runMain(void);

  /// \brief Process input stream to send relevant blocks to python
  /// \param stream input stream
  std::string processStream(std::istream& stream, std::ostream& os);

  /// \brief Return a pointer to the dictionary of global variables
  PyObject* globals();

 private:
  /// The Pythone thread state
  PyThreadState* _pyState;
  /// Pointer to the dictionary of global variables
  PyObject* globals_;
  /// Pointer to the dictionary of local variables
  PyObject* locals_;
  PyObject* mainmod_;
  PyObject* traceback_format_exception_;
};
}  // namespace python
}  // namespace dynamicgraph
#endif  // DYNAMIC_GRAPH_PYTHON_INTERPRETER_H
