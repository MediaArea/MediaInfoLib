/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#ifndef HelpersH
#define HelpersH

#include <string>
#include <node_api.h>

bool To_Uint32(napi_env Environment, napi_value Source, uint32_t& Destination);
bool To_Int64(napi_env Environment, napi_value Source, int64_t& Destination);
bool To_SizeT(napi_env Environment, napi_value Source, size_t& Destination);
bool To_Unicode(napi_env Environment, napi_value Source, std::wstring& Destination);
std::string From_Unicode(std::wstring Source);
napi_value From_SizeT(napi_env Environment, const size_t Source);
napi_value Undefined(napi_env Environment);

#endif //HelpersH
