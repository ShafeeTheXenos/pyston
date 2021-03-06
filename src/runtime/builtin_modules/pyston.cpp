// Copyright (c) 2014-2015 Dropbox, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "core/types.h"
#include "runtime/objmodel.h"
#include "runtime/types.h"

namespace pyston {

BoxedModule* pyston_module;

static Box* setOption(Box* option, Box* value) {
    if (option->cls != str_cls)
        raiseExcHelper(TypeError, "option must be a 'string' object but received a '%s'", getTypeName(option));
    BoxedString* option_string = (BoxedString*)option;

    if (value->cls != int_cls)
        raiseExcHelper(TypeError, "value must be a 'int' object but received a '%s'", getTypeName(value));
    int n = ((BoxedInt*)value)->n;

#define CHECK(_s)                                                                                                      \
    if (option_string->s == STRINGIFY(_s))                                                                             \
    _s = n

    // :)
    CHECK(ENABLE_INTERPRETER);
    else CHECK(ENABLE_OSR);
    else CHECK(ENABLE_REOPT);
    else CHECK(FORCE_INTERPRETER);
    else CHECK(REOPT_THRESHOLD_INTERPRETER);
    else CHECK(OSR_THRESHOLD_INTERPRETER);
    else CHECK(REOPT_THRESHOLD_BASELINE);
    else CHECK(OSR_THRESHOLD_BASELINE);
    else CHECK(SPECULATION_THRESHOLD);
    else raiseExcHelper(ValueError, "unknown option name '%s", option_string->s.c_str());

    return None;
}

void setupPyston() {
    pyston_module = createModule("__pyston__", "__builtin__");

    pyston_module->giveAttr("setOption",
                            new BoxedBuiltinFunctionOrMethod(boxRTFunction((void*)setOption, UNKNOWN, 2), "setOption"));
}
}
