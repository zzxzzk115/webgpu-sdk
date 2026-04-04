/*
 * Copyright (c) 2025 pongasoft
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

#include "Events.h"

namespace emscripten::glfw3 {

//------------------------------------------------------------------------
// EventListenerBase::setTarget
//------------------------------------------------------------------------
void EventListenerBase::setTarget(char const *iTarget)
{
  if(iTarget == EMSCRIPTEN_EVENT_TARGET_WINDOW)
  {
    fSpecialTarget = EMSCRIPTEN_EVENT_TARGET_WINDOW;
    fTarget = "window";
  } else if(iTarget == EMSCRIPTEN_EVENT_TARGET_DOCUMENT)
  {
    fSpecialTarget = EMSCRIPTEN_EVENT_TARGET_DOCUMENT;
    fTarget = "document";
  } else if(iTarget == EMSCRIPTEN_EVENT_TARGET_SCREEN)
  {
    fSpecialTarget = EMSCRIPTEN_EVENT_TARGET_SCREEN;
    fTarget = "screen";
  } else {
    fTarget = iTarget;
  }
}

//------------------------------------------------------------------------
// EventListenerBase::addCallback
//------------------------------------------------------------------------
bool EventListenerBase::addCallback(int iEventType, callback_setter_function_t const &iCallback)
{
  remove();

  fEventTypeId = iEventType;

  auto error = iCallback();

  if(error != EMSCRIPTEN_RESULT_SUCCESS)
  {
    ErrorHandler::instance().logError(GLFW_PLATFORM_ERROR, "Error [%d] while registering listener for [%s]",
                                      error,
                                      fTarget.c_str());
    return false;
  }

  fAdded = true;

  return true;
}

//------------------------------------------------------------------------
// EventListenerBase::remove
//------------------------------------------------------------------------
void EventListenerBase::remove()
{
  if(fAdded)
  {
    auto error = emscripten_html5_remove_event_listener(fSpecialTarget ? fSpecialTarget : fTarget.c_str(),
                                                        this,
                                                        fEventTypeId,
                                                        getGenericCallback());
    if(error != EMSCRIPTEN_RESULT_SUCCESS)
    {
      ErrorHandler::instance().logError(GLFW_PLATFORM_ERROR, "Error [%d] while removing listener for [%s]",
                                        error,
                                        fTarget.c_str());
    }
    fAdded = false;
  }
}

}
