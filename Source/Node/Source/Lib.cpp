/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "Helpers.h"
#include "NodeMediaInfo.h"
#include "NodeMediaInfoList.h"

napi_value CreateMediaInfoObject(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    if (napi_get_cb_info(Environment, Info, &ArgumentsCount, Arguments, NULL, NULL)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_cb_info() failed");
        return Undefined(Environment); //undefined
    }

    napi_value Instance;
    if (NodeMediaInfo::NewInstance(Environment, ArgumentsCount, Arguments, &Instance)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: NodeMediaInfo::NewInstance() failed");
        return Undefined(Environment); //undefined
    }

    return Instance;
}

napi_value CreateMediaInfoListObject(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    if (napi_get_cb_info(Environment, Info, &ArgumentsCount, Arguments, NULL, NULL)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_cb_info() failed");
        return Undefined(Environment); //undefined
    }

    napi_value Instance;
    if (NodeMediaInfoList::NewInstance(Environment, ArgumentsCount, Arguments, &Instance)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: NodeMediaInfoList::NewInstance() failed");
        return Undefined(Environment); //undefined
    }

    return Instance;
}

napi_value Init(napi_env Environment, napi_value Exports)
{
    if (NodeMediaInfo::Init(Environment)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: NodeMediaInfo::Init() failed");
        return Undefined(Environment); //undefined
    }

    if (NodeMediaInfoList::Init(Environment)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: NodeMediaInfoList::Init() failed");
        return Undefined(Environment); //undefined
    }

    napi_value MediaInfoFunc;
    if (napi_create_function(Environment, "MediaInfo", NAPI_AUTO_LENGTH, CreateMediaInfoObject, NULL, &MediaInfoFunc)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_function() failed");
        return Undefined(Environment); //undefined
    }

    napi_value MediaInfoListFunc;
    if (napi_create_function(Environment, "MediaInfoList", NAPI_AUTO_LENGTH, CreateMediaInfoListObject, NULL, &MediaInfoListFunc)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_function() failed");
        return Undefined(Environment); //undefined
    }

    if (napi_set_named_property(Environment, Exports, "MediaInfo", MediaInfoFunc)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_set_named_property() failed");
        return Undefined(Environment); //undefined
    }

    if (napi_set_named_property(Environment, Exports, "MediaInfoList", MediaInfoListFunc)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_set_named_property() failed");
        return Undefined(Environment); //undefined
    }

    return Exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
