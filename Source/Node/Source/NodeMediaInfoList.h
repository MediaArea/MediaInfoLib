/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#ifndef NodeMediaInfoListH
#define NodeMediaInfoListH

#include <string>
#include <node_api.h>

#include "MediaInfoDLL.h"

using namespace MediaInfoDLL;

class NodeMediaInfoList
{
public:
    static napi_status Init(napi_env Environment);
    static void Finalize(napi_env Environment, void* NativeObject, void*);
    static napi_status NewInstance(napi_env Environment, size_t ArgumentsCount, napi_value* Arguments, napi_value* Instance);

private:
    NodeMediaInfoList(const char* LibraryPath=NULL);
    ~NodeMediaInfoList();

    static napi_ref Constructor;
    static napi_value New(napi_env Environment, napi_callback_info Info);
    static NodeMediaInfoList* GetInstance(napi_env& Environment, napi_callback_info& Info, size_t& ArgumentsCount, napi_value* Arguments);

    //  Wrapper funtions
    static napi_value Open(napi_env Environment, napi_callback_info Info);
    static napi_value Close(napi_env Environment, napi_callback_info Info);
    static napi_value Inform(napi_env Environment, napi_callback_info Info);
    static napi_value Get(napi_env Environment, napi_callback_info Info);
    static napi_value Option(napi_env Environment, napi_callback_info Info);
    static napi_value State_Get(napi_env Environment, napi_callback_info Info);
    static napi_value Count_Get(napi_env Environment, napi_callback_info Info);

    napi_env Environment_;
    napi_ref Wrapper_;
    MediaInfoList* MI;
};

#endif //NodeMediaInfoListH
