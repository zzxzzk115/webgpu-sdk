/*
 * Copyright (c) 2023 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#ifndef EMSCRIPTEN_GLFW_EVENTS_H
#define EMSCRIPTEN_GLFW_EVENTS_H

#include <emscripten/html5.h>
#include <functional>
#include "ErrorHandler.h"

namespace emscripten::glfw3 {

//! EmscriptenEventCallback
template<typename E>
using EmscriptenEventCallback = bool (*)(int, E const *, void *);

//! EmscriptenCallbackFunction
template<typename E>
using EmscriptenCallbackFunction = EMSCRIPTEN_RESULT (*)(const char *, void *, bool, EmscriptenEventCallback<E>, pthread_t);

//! EmscriptenCallbackFunction2 (implicit target)
template<typename E>
using EmscriptenCallbackFunction2 = EMSCRIPTEN_RESULT (*)(void *, bool, EmscriptenEventCallback<E>, pthread_t);

class EventListenerBase
{
public:
  virtual ~EventListenerBase() { remove(); };

  void remove();

protected:
  /**
   * Wraps the call to an Emscripten setting callback function (ex: emscripten_set_mousemove_callback_on_thread)
   * in a generic fashion. */
  using callback_setter_function_t = std::function<int()>;

protected:
  void setTarget(char const *iTarget);
  bool addCallback(int iEventType, callback_setter_function_t const &iCallback);
  virtual void *getGenericCallback() const = 0;

protected:
  char const *fSpecialTarget{};
  std::string fTarget{};
  int fEventTypeId{};
  pthread_t fThread{EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD};

private:
  bool fAdded{};
};

template<typename E>
class EventListener : public EventListenerBase
{
public:
  using event_listener_t = std::function<bool(int iEventType, E const *iEvent)>;

public:

  inline EventListener &target(char const *iTarget) { setTarget(iTarget); return *this; }
  inline EventListener &listener(event_listener_t v) { fEventListener = std::move(v); return *this; }

  bool add(int iEventType, EmscriptenCallbackFunction<E> iFunction);
  bool add(int iEventType, EmscriptenCallbackFunction2<E> iFunction);

  bool invoke(int iEventType, E const *iEvent) { if(fEventListener) return fEventListener(iEventType, iEvent); else return false; }

protected:
  void *getGenericCallback() const override;

private:
  event_listener_t fEventListener{};
};

//------------------------------------------------------------------------
// EventListenerCallback
// - generic callback which extracts EventListener<E> from iUserData and invoke it
//------------------------------------------------------------------------
template<typename E>
bool EventListenerCallback(int iEventType, E const *iEvent, void *iUserData)
{
  auto cb = reinterpret_cast<EventListener<E> *>(iUserData);
  return cb->invoke(iEventType, iEvent);
}


//------------------------------------------------------------------------
// EventListenerBase::getGenericCallback
//------------------------------------------------------------------------
template<typename E>
void *EventListener<E>::getGenericCallback() const
{
  return reinterpret_cast<void *>(EventListenerCallback<E>);
}

//------------------------------------------------------------------------
// EventListener<E>::add
//------------------------------------------------------------------------
template<typename E>
bool EventListener<E>::add(int iEventType, EmscriptenCallbackFunction<E> iFunction)
{
  return addCallback(iEventType, [iFunction, this]() {
    return iFunction(fSpecialTarget ? fSpecialTarget : fTarget.c_str(),
                     this,
                     false,
                     EventListenerCallback<E>,
                     fThread);
  });
}

//------------------------------------------------------------------------
// EventListener<E>::add
//------------------------------------------------------------------------
template<typename E>
bool EventListener<E>::add(int iEventType, EmscriptenCallbackFunction2<E> iFunction)
{
  return addCallback(iEventType, [iFunction, this]() {
    return iFunction(this,
                     false,
                     EventListenerCallback<E>,
                     fThread);
  });
}


}
#endif //EMSCRIPTEN_GLFW_EVENTS_H
