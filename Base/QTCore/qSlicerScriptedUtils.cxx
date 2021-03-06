/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#include "qSlicerScriptedUtils_p.h"

// Qt includes
#include <QFileInfo>

// PythonQt includes
#include <PythonQt.h>

//-----------------------------------------------------------------------------
bool qSlicerScriptedUtils::loadSourceAsModule(const QString& moduleName,
                                       const QString& fileName,
                                       PyObject * global_dict,
                                       PyObject * local_dict)
{
  PyObject* pyRes = 0;
  if (fileName.endsWith(".py"))
    {
    pyRes = PyRun_String(
          QString("import imp;imp.load_source('%2', '%1');del imp;")
          .arg(fileName).arg(moduleName).toLatin1(),
          Py_file_input, global_dict, local_dict);
    }
  else if (fileName.endsWith(".pyc"))
    {
    pyRes = PyRun_String(
          QString("with open('%1', 'rb') as f:import imp;imp.load_module('%2', f, '%1', ('.pyc', 'rb', 2));del imp")
          .arg(fileName).arg(moduleName).toLatin1(),
          Py_file_input, global_dict, local_dict);
    }
  if (!pyRes)
    {
    PythonQt::self()->handleError();
    qCritical() << "loadSourceAsModule - Failed to load file" << fileName
                << " as module" << moduleName << "!";
    return false;
    }
  Py_DECREF(pyRes);
  return true;
}

//-----------------------------------------------------------------------------
qSlicerPythonCppAPI::qSlicerPythonCppAPI()
{
  this->PythonSelf = 0;
}

//-----------------------------------------------------------------------------
qSlicerPythonCppAPI::~qSlicerPythonCppAPI()
{
  if (this->PythonSelf)
    {
    foreach(PyObject* method, this->PythonAPIMethods)
      {
      Py_XDECREF(method);
      }
    Py_DECREF(this->PythonSelf);
    }
}

//-----------------------------------------------------------------------------
void qSlicerPythonCppAPI::declareMethod(int id, const char* name)
{
  if (!name)
    {
    return;
    }
  this->APIMethods[id] = QString(name);
}

//-----------------------------------------------------------------------------
PyObject* qSlicerPythonCppAPI::instantiateClass(QObject* cpp, const QString& className, PyObject* classToInstantiate)
{
  PyObject * wrappedThis = PythonQt::self()->priv()->wrapQObject(cpp);
  if (!wrappedThis)
    {
    PythonQt::self()->handleError();
    qCritical() << "qSlicerPythonCppAPI::instantiateClass"
                << "- Failed to wrap" << cpp->metaObject()->className() << "associated with " << className;
    return 0;
    }

  PyObject * arguments = PyTuple_New(1);
  PyTuple_SET_ITEM(arguments, 0, wrappedThis);

  // Attempt to instantiate the associated python class
  PyObject * self = PyInstance_New(classToInstantiate, arguments, 0);
  Py_DECREF(arguments);
  if (!self)
    {
    PythonQt::self()->handleError();
    qCritical() << "qSlicerPythonCppAPI::instantiateClass"
                << "- Failed to instantiate scripted pythonqt class" << className << classToInstantiate;
    return 0;
    }

  foreach(int methodId, this->APIMethods.keys())
    {
    QString methodName = this->APIMethods.value(methodId);
    if (!PyObject_HasAttrString(self, methodName.toLatin1()))
      {
      continue;
      }
    PyObject * method = PyObject_GetAttrString(self, methodName.toLatin1());
    this->PythonAPIMethods[methodId] = method;
    }

  this->PythonSelf = self;

  return self;
}

//-----------------------------------------------------------------------------
PyObject * qSlicerPythonCppAPI::callMethod(int id, PyObject * arguments)
{
  if (!this->PythonAPIMethods.contains(id))
    {
    return 0;
    }
  PyObject * method = this->PythonAPIMethods.value(id);
  PythonQt::self()->clearError();
  PyObject * result = PyObject_CallObject(method, arguments);
  if (PythonQt::self()->handleError())
    {
    return 0;
    }
  return result;
}

//-----------------------------------------------------------------------------
PyObject* qSlicerPythonCppAPI::pythonSelf()const
{
  return this->PythonSelf;
}
