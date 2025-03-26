/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include <locale>
#include <codecvt>

#include "Helpers.h"

//
// Helpers functions
//
bool To_Uint32(napi_env Environment, napi_value Source, uint32_t& Destination)
{    napi_valuetype Value_Type;
    if (napi_typeof(Environment, Source, &Value_Type)!=napi_ok || Value_Type!=napi_number)
        return false;

    if (napi_get_value_uint32(Environment, Source, &Destination)!=napi_ok)
        return false;

    return true;
}

bool To_Int64(napi_env Environment, napi_value Source, int64_t& Destination)
{
    napi_valuetype Value_Type;
    if (napi_typeof(Environment, Source, &Value_Type)!=napi_ok || Value_Type!=napi_number)
        return false;

    if (napi_get_value_int64(Environment, Source, &Destination)!=napi_ok)
        return false;

    return true;
}

bool To_SizeT(napi_env Environment, napi_value Source, size_t& Destination)
{
    napi_valuetype Value_Type;
    if (napi_typeof(Environment, Source, &Value_Type)!=napi_ok || Value_Type!=napi_number)
        return false;

    switch (sizeof(size_t))
    {
        case 4: // size_t is 32 bits
            if (napi_get_value_uint32(Environment, Source, (uint32_t*)&Destination)!=napi_ok)
                return false;
            break;
        break;
        case 8: // size_t is 64 bits
            if (napi_get_value_int64(Environment, Source, (int64_t*)&Destination)!=napi_ok)
                return false;
            break;
        default: // unsupported
            napi_throw_error(Environment, NULL, "internal error: unsupported size_t size");
            return false;
    }

    return true;
}

bool To_Unicode(napi_env Environment, napi_value Source, std::wstring& Destination)
{
    napi_valuetype Value_Type;
    if (napi_typeof(Environment, Source, &Value_Type)!=napi_ok || Value_Type!=napi_string)
        return false;

    size_t Source_Size=0;
    if (napi_get_value_string_utf8(Environment, Source, NULL, 0, &Source_Size)!=napi_ok)
        return false;

    char Buffer[Source_Size+1];
    if (napi_get_value_string_utf8(Environment, Source, (char*)Buffer, Source_Size+1, &Source_Size)!=napi_ok)
        return false;

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> Codec;
    Destination=Codec.from_bytes((char*)Buffer);

    return true;
}

std::string From_Unicode(std::wstring Source)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> Codec;
    return Codec.to_bytes(Source);
}

napi_value From_SizeT(napi_env Environment, const size_t Source)
{
    napi_value ToReturn;
    switch (sizeof(size_t))
    {
        case 4: // size_t is 32 bits
            if (napi_create_uint32(Environment, Source, &ToReturn)!=napi_ok)
            {
                napi_throw_error(Environment, NULL, "internal error: napi_create_uint32() failed");
                return Undefined(Environment); //undefined
            }
            break;
        break;
        case 8: // size_t is 64 bits
            if (napi_create_int64(Environment, Source, &ToReturn)!=napi_ok)
            {
                napi_throw_error(Environment, NULL, "internal error: napi_create_int64() failed");
                return Undefined(Environment); //undefined
            }
            break;
        default: // unsupported
            napi_throw_error(Environment, NULL, "internal error: unsupported size_t size");
                return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value Undefined(napi_env Environment)
{
    napi_value ToReturn;
    if (napi_get_undefined(Environment, &ToReturn)!=napi_ok)
        napi_throw_error(Environment, NULL, "internal error: napi_get_undefined() failed, returned value may be inconsistent");

    return ToReturn;
}
