#ifndef lIBREGEX_H
#define LIBREGEX_H
#include <Python.h>
#include "struct.h"
#include "regex.h"
#include "manipulate.h"

PyObject* py_match(PyObject* self, PyObject* args);
void retField(PyObject** string,Fields* field,Subfield* sub);

#endif
