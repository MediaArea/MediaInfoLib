/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "NodeMediaInfo.h" 
#include "Helpers.h"

napi_ref NodeMediaInfo::Constructor;

NodeMediaInfo::NodeMediaInfo(const char* LibraryPath) :Environment_(NULL), Wrapper_(NULL), MI(new MediaInfo(LibraryPath))
{
}

NodeMediaInfo::~NodeMediaInfo()
{
    delete MI;
    napi_delete_reference(Environment_, Wrapper_);
}

napi_status NodeMediaInfo::Init(napi_env Environment)
{
    napi_status Status;

    napi_property_descriptor Properties[]={
        {"Open", NULL, Open, NULL, NULL, NULL, napi_default, NULL},
        {"Open_Buffer_Init", NULL, Open_Buffer_Init, NULL, NULL, NULL, napi_default, NULL},
        {"Open_Buffer_Continue", NULL, Open_Buffer_Continue, NULL, NULL, NULL, napi_default, NULL},
        {"Open_Buffer_Continue_GoTo_Get", NULL, Open_Buffer_Continue_GoTo_Get, NULL, NULL, NULL, napi_default, NULL},
        {"Open_Buffer_Finalize", NULL, Open_Buffer_Finalize, NULL, NULL, NULL, napi_default, NULL},
        {"Open_NextPacket", NULL, Open_NextPacket, NULL, NULL, NULL, napi_default, NULL},
        {"Close", NULL, Close, NULL, NULL, NULL, napi_default, NULL},
        {"Inform", NULL, Inform, NULL, NULL, NULL, napi_default, NULL},
        {"Get", NULL, Get, NULL, NULL, NULL, napi_default, NULL},
        {"Option", NULL, Option, NULL, NULL, NULL, napi_default, NULL},
        {"State_Get", NULL, State_Get, NULL, NULL, NULL, napi_default, NULL},
        {"Count_Get", NULL, Count_Get, NULL, NULL, NULL, napi_default, NULL}
    };

    napi_value Exports;
    Status=napi_define_class(Environment, "MediaInfo", NAPI_AUTO_LENGTH, New, NULL, sizeof(Properties)/sizeof(Properties[0]), Properties, &Exports);
    if (Status!=napi_ok)
        return Status;

    Status=napi_create_reference(Environment, Exports, 1, &Constructor);
    if (Status!=napi_ok)
        return Status;

    return napi_ok;
}

void NodeMediaInfo::Finalize(napi_env Environment, void* nativeObject, void*)
{
    reinterpret_cast<NodeMediaInfo*>(nativeObject)->~NodeMediaInfo();
}

napi_value NodeMediaInfo::New(napi_env Environment, napi_callback_info Info)
{
    napi_value JsThis;

    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    if (napi_get_cb_info(Environment, Info, &ArgumentsCount, Arguments, &JsThis, NULL)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_cb_info() failed");
        return Undefined(Environment); //undefined
    }

    NodeMediaInfo* Instance=NULL;
    if(ArgumentsCount)
    {
        size_t Size=0;
        if (napi_get_value_string_utf8(Environment, Arguments[0], NULL, 0, &Size)!=napi_ok)
        {
            napi_throw_error(Environment, NULL, "internal error: napi_get_value_string_utf8() failed");
            return Undefined(Environment); //undefined
        }

        char Buffer[Size+1];
        if (napi_get_value_string_utf8(Environment, Arguments[0], (char*)Buffer, Size+1, &Size)!=napi_ok)
        {
            napi_throw_error(Environment, NULL, "internal error: napi_get_value_string_utf8() failed");
            return Undefined(Environment); //undefined
        }

        Instance=new NodeMediaInfo((char*)Buffer);
    }
    else
    {
        Instance=new NodeMediaInfo();
    }

    Instance->Environment_=Environment;
    if (napi_wrap(Environment, JsThis, reinterpret_cast<void*>(Instance), NodeMediaInfo::Finalize, NULL, &Instance->Wrapper_)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_wrap() failed");
        return Undefined(Environment); //undefined
    }

    return JsThis;
}

napi_status NodeMediaInfo::NewInstance(napi_env Environment, size_t ArgumentsCount, napi_value* Arguments, napi_value* Instance)
{
    napi_status Status;

    napi_value Cons;
    Status=napi_get_reference_value(Environment, Constructor, &Cons);
    if (Status!=napi_ok)
        return Status;

    Status=napi_new_instance(Environment, Cons, ArgumentsCount, Arguments, Instance);
    if (Status!=napi_ok)
        return Status;

    return napi_ok;
}

NodeMediaInfo* NodeMediaInfo::GetInstance(napi_env& Environment, napi_callback_info& Info, size_t& ArgumentsCount, napi_value* Arguments)
{
    napi_value JsThis;
    if (napi_get_cb_info(Environment, Info, &ArgumentsCount, Arguments, &JsThis, NULL)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_cb_info() failed");
        return NULL;
    }

    NodeMediaInfo* Instance;
    if (napi_unwrap(Environment, JsThis, reinterpret_cast<void**>(&Instance))!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_unwrap() failed");
        return NULL;
    }

    return Instance;
}

//
// Wrapper functions
//
napi_value NodeMediaInfo::Open(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    std::wstring File;
    if (!ArgumentsCount || !To_Unicode(Environment, Arguments[0], File))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfo::Open()");
        return Undefined(Environment); //undefined
    }

    size_t Result=Instance->MI->Open(File.c_str());

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfo::Open_Buffer_Init(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=2;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined


    int64_t File_Size=-1;
    if (ArgumentsCount)
        To_Int64(Environment, Arguments[0], File_Size);

    int64_t File_Offset=0;
    if (ArgumentsCount>1)
        To_Int64(Environment, Arguments[1], File_Offset);

    size_t Result=Instance->MI->Open_Buffer_Init(File_Size, File_Offset);

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfo::Open_Buffer_Continue(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=2;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    bool IsBuffer;
    if (!ArgumentsCount || napi_is_buffer(Environment, Arguments[0], &IsBuffer)!=napi_ok || IsBuffer==false)
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfo::Open_Buffer_Continue()");
        return Undefined(Environment); //undefined
    }

    MediaInfo_int8u* Buffer=NULL;
    size_t Length;
    if (napi_get_buffer_info(Environment, Arguments[0], (void**)&Buffer, &Length)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_buffer_info() failed");
        return Undefined(Environment); //undefined
    }

    size_t Buffer_Size;
    if (ArgumentsCount<2 || !To_SizeT(Environment, Arguments[1], Buffer_Size))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 2 for NodeMediaInfo::Open_Buffer_Continue()");
        return Undefined(Environment); //undefined
    }

    size_t Result=Instance->MI->Open_Buffer_Continue(Buffer, Buffer_Size);

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfo::Open_Buffer_Continue_GoTo_Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    MediaInfo_int64u value=Instance->MI->Open_Buffer_Continue_GoTo_Get();

    napi_value ToReturn;
    if (napi_create_int64(Environment, value==(MediaInfo_int64u)-1?-1:(int64_t)value, &ToReturn)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_int64() failed");
        return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value NodeMediaInfo::Open_Buffer_Finalize(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Result=Instance->MI->Open_Buffer_Finalize();

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfo::Open_NextPacket(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Result=Instance->MI->Open_NextPacket();

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfo::Close(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    Instance->MI->Close();

    napi_value ToReturn;
    if (napi_get_null(Environment, &ToReturn)!=napi_ok)
        napi_throw_error(Environment, NULL, "internal error: napi_get_null() failed, returned value may be inconsistent");

    return ToReturn;
}

napi_value NodeMediaInfo::Inform(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    std::wstring Result=Instance->MI->Inform();

    napi_value ToReturn;
    if (napi_create_string_utf8(Environment, From_Unicode(Result).c_str(), NAPI_AUTO_LENGTH, &ToReturn)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_string_utf8() failed");
        return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value NodeMediaInfo::Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=5;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    uint32_t StreamKind;
    if (!ArgumentsCount || !To_Uint32(Environment, Arguments[0], StreamKind))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfo::Get()");
        return Undefined(Environment); //undefined
    }


    size_t StreamNumber;
    if (ArgumentsCount<2 || !To_SizeT(Environment, Arguments[1], StreamNumber))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 2 for NodeMediaInfo::Get()");
        return Undefined(Environment); //undefined
    }

    napi_valuetype Valuetype2;
    if (ArgumentsCount<3 || napi_typeof(Environment, Arguments[2], &Valuetype2)!=napi_ok || (Valuetype2!=napi_number && Valuetype2!=napi_string))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 3 for NodeMediaInfo::Get()");
        return Undefined(Environment); //undefined
    }

    uint32_t InfoKind=1;
    if (ArgumentsCount>3)
        To_Uint32(Environment, Arguments[3], InfoKind);

    uint32_t SearchKind=0;
    if (ArgumentsCount>4)
        To_Uint32(Environment, Arguments[4], SearchKind);

    std::wstring Result;
    if (Valuetype2==napi_number)
    {
        size_t Parameter;
        if (!To_SizeT(Environment, Arguments[2], Parameter))
        {
            napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 3 for NodeMediaInfo::Get()");
            return Undefined(Environment); //undefined
        }

        Result=Instance->MI->Get((stream_t)StreamKind, StreamNumber, Parameter, (info_t)InfoKind);
    }
    else // Valuetype2 == napi_string
    {
        std::wstring Parameter;
        if (!To_Unicode(Environment, Arguments[2], Parameter))
        {
            napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 3 for NodeMediaInfo::Get()");
            return Undefined(Environment); //undefined
        }

        Result=Instance->MI->Get((stream_t)StreamKind, StreamNumber, Parameter.c_str(), (info_t)InfoKind, (info_t)SearchKind);
    }

    napi_value ToReturn;
    if (napi_create_string_utf8(Environment, From_Unicode(Result).c_str(), NAPI_AUTO_LENGTH, &ToReturn)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_string_utf8() failed");
        return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value NodeMediaInfo::Option(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=2;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    std::wstring Option;
    if (!ArgumentsCount || !To_Unicode(Environment, Arguments[0], Option))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfo::Option()");
        return Undefined(Environment); //undefined
    }

    std::wstring Value=L"";
    if (ArgumentsCount>1)
        To_Unicode(Environment, Arguments[1], Value);

    std::wstring Result=Instance->MI->Option(Option.c_str(), Value.c_str());

    napi_value ToReturn;
    if (napi_create_string_utf8(Environment, From_Unicode(Result).c_str(), NAPI_AUTO_LENGTH, &ToReturn)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_string_utf8() failed");
        return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value NodeMediaInfo::State_Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Result=Instance->MI->State_Get();

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfo::Count_Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=2;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfo* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    uint32_t StreamKind;
    if(!ArgumentsCount || !To_Uint32(Environment, Arguments[0], StreamKind))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfo::Count_Get()");
        return Undefined(Environment); //undefined
    }

    size_t StreamNumber=(size_t)-1;
    if (ArgumentsCount>1)
        To_SizeT(Environment, Arguments[1], StreamNumber);

    size_t Result=Instance->MI->Count_Get((stream_t)StreamKind, StreamNumber);

    return From_SizeT(Environment, Result);
}
