/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "NodeMediaInfoList.h" 
#include "Helpers.h"

napi_ref NodeMediaInfoList::Constructor;

NodeMediaInfoList::NodeMediaInfoList(const char* LibraryPath) :Environment_(NULL), Wrapper_(NULL), MI(new MediaInfoList(LibraryPath))
{
}

NodeMediaInfoList::~NodeMediaInfoList()
{
    delete MI;
    napi_delete_reference(Environment_, Wrapper_);
}

napi_status NodeMediaInfoList::Init(napi_env Environment)
{
    napi_status Status;

    napi_property_descriptor Properties[]={
        {"Open", NULL, Open, NULL, NULL, NULL, napi_default, NULL},
        {"Close", NULL, Close, NULL, NULL, NULL, napi_default, NULL},
        {"Inform", NULL, Inform, NULL, NULL, NULL, napi_default, NULL},
        {"Get", NULL, Get, NULL, NULL, NULL, napi_default, NULL},
        {"Option", NULL, Option, NULL, NULL, NULL, napi_default, NULL},
        {"State_Get", NULL, State_Get, NULL, NULL, NULL, napi_default, NULL},
        {"Count_Get", NULL, Count_Get, NULL, NULL, NULL, napi_default, NULL}
    };

    napi_value Exports;
    Status=napi_define_class(Environment, "MediaInfoList", NAPI_AUTO_LENGTH, New, NULL, sizeof(Properties)/sizeof(Properties[0]), Properties, &Exports);
    if (Status!=napi_ok)
        return Status;

    Status=napi_create_reference(Environment, Exports, 1, &Constructor);
    if (Status!=napi_ok)
        return Status;

    return napi_ok;
}

void NodeMediaInfoList::Finalize(napi_env Environment, void* NativeObject, void*)
{
    reinterpret_cast<NodeMediaInfoList*>(NativeObject)->~NodeMediaInfoList();
}

napi_value NodeMediaInfoList::New(napi_env Environment, napi_callback_info Info)
{
    napi_value JsThis;
    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    if (napi_get_cb_info(Environment, Info, &ArgumentsCount, Arguments, &JsThis, NULL)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_cb_info() failed");
        return Undefined(Environment); //undefined
    }

    NodeMediaInfoList* Instance=NULL;
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

        Instance=new NodeMediaInfoList((char*)Buffer);
    }
    else
    {
        Instance=new NodeMediaInfoList();
    }

    Instance->Environment_=Environment;
    if (napi_wrap(Environment, JsThis, reinterpret_cast<void*>(Instance), NodeMediaInfoList::Finalize, NULL, &Instance->Wrapper_)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_wrap() failed");
        return Undefined(Environment); //undefined
    }

    return JsThis;
}

napi_status NodeMediaInfoList::NewInstance(napi_env Environment, size_t ArgumentsCount, napi_value* Arguments, napi_value* Instance)
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

NodeMediaInfoList* NodeMediaInfoList::GetInstance(napi_env& Environment, napi_callback_info& Info, size_t& ArgumentsCount, napi_value* Arguments)
{
    napi_value JsThis;
    if (napi_get_cb_info(Environment, Info, &ArgumentsCount, Arguments, &JsThis, NULL)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_get_cb_info() failed");
        return NULL;
    }

    NodeMediaInfoList* Instance;
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
napi_value NodeMediaInfoList::Open(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=2;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    std::wstring File;
    if (!ArgumentsCount || !To_Unicode(Environment, Arguments[0], File))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfoList::Open()");
        return Undefined(Environment); //undefined
    }

    uint32_t FileOptions=(uint32_t)FileOption_Nothing;
    if (ArgumentsCount>1)
        To_Uint32(Environment, Arguments[1], FileOptions);

    size_t Result=Instance->MI->Open(File.c_str(), (fileoptions_t)FileOptions);

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfoList::Close(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Pos=(size_t)-1;
    To_SizeT(Environment, Arguments[0], Pos);

    Instance->MI->Close(Pos);

    napi_value ToReturn;
    if(napi_get_null(Environment, &ToReturn)!=napi_ok)
        napi_throw_error(Environment, NULL, "internal error: napi_get_null() failed, returned value may be inconsistent");

    return ToReturn;
}

napi_value NodeMediaInfoList::Inform(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=1;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Pos=(size_t)-1;
    if (ArgumentsCount)
        To_SizeT(Environment, Arguments[0], Pos);

    std::wstring Result=Instance->MI->Inform(Pos);

    napi_value ToReturn;
    if (napi_create_string_utf8(Environment, From_Unicode(Result).c_str(), NAPI_AUTO_LENGTH, &ToReturn)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_string_utf8() failed");
        return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value NodeMediaInfoList::Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=6;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Pos;
    if (!ArgumentsCount || !To_SizeT(Environment, Arguments[0], Pos))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfoList::Get()");
        return Undefined(Environment); //undefined
    }

    uint32_t StreamKind;
    if (ArgumentsCount<2 || !To_Uint32(Environment, Arguments[1], StreamKind))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 2 for NodeMediaInfoList::Get()");
        return Undefined(Environment); //undefined
    }


    size_t StreamNumber;
    if (ArgumentsCount<3 || !To_SizeT(Environment, Arguments[2], StreamNumber))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 3 for NodeMediaInfoList::Get()");
        return Undefined(Environment); //undefined
    }

    napi_valuetype Valuetype3;
    if (ArgumentsCount<4 || napi_typeof(Environment, Arguments[3], &Valuetype3)!=napi_ok || (Valuetype3!=napi_number && Valuetype3!=napi_string))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 4 for NodeMediaInfoList::Get()");
        return Undefined(Environment); //undefined
    }

    uint32_t InfoKind=1;
    if (ArgumentsCount>4)
        To_Uint32(Environment, Arguments[4], InfoKind);

    uint32_t SearchKind=0;
    if (ArgumentsCount>5)
        To_Uint32(Environment, Arguments[5], SearchKind);

    std::wstring Result;
    if (Valuetype3==napi_number)
    {
        size_t Parameter;
        if (!To_SizeT(Environment, Arguments[3], Parameter))
        {
            napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 4 for NodeMediaInfoList::Get()");
            return Undefined(Environment); //undefined
        }

        Result=Instance->MI->Get(Pos, (stream_t)StreamKind, StreamNumber, Parameter, (info_t)InfoKind);
    }
    else // Valuetype3 == napi_string
    {
        std::wstring Parameter;
        if (!To_Unicode(Environment, Arguments[3], Parameter))
        {
            napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 4 for NodeMediaInfoList::Get()");
            return Undefined(Environment); //undefined
        }

        Result=Instance->MI->Get(Pos, (stream_t)StreamKind, StreamNumber, Parameter.c_str(), (info_t)InfoKind, (info_t)SearchKind);
    }

    napi_value ToReturn;
    if (napi_create_string_utf8(Environment, From_Unicode(Result).c_str(), NAPI_AUTO_LENGTH, &ToReturn)!=napi_ok)
    {
        napi_throw_error(Environment, NULL, "internal error: napi_create_string_utf8() failed");
        return Undefined(Environment); //undefined
    }

    return ToReturn;
}

napi_value NodeMediaInfoList::Option(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=2;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    std::wstring Option;
    if (!ArgumentsCount || !To_Unicode(Environment, Arguments[0], Option))
    {
        napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfoList::Option()");
        return Undefined(Environment); //undefined
    }

    std::wstring Value=L"";
    if(ArgumentsCount>1)
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

napi_value NodeMediaInfoList::State_Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=0;
    napi_value* Arguments=NULL;

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Result=Instance->MI->State_Get();

    return From_SizeT(Environment, Result);
}

napi_value NodeMediaInfoList::Count_Get(napi_env Environment, napi_callback_info Info)
{
    size_t ArgumentsCount=3;
    napi_value Arguments[ArgumentsCount];

    NodeMediaInfoList* Instance=GetInstance(Environment, Info, ArgumentsCount, Arguments);
    if (!Instance)
        return Undefined(Environment); //undefined

    size_t Result;
    if (ArgumentsCount)
    {
        size_t Pos;
        if (!To_SizeT(Environment, Arguments[0], Pos))
        {
            napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfoList::Count_Get()");
            return Undefined(Environment); //undefined
        }

        uint32_t StreamKind;
        if(ArgumentsCount<2 || !To_Uint32(Environment, Arguments[1], StreamKind))
        {
            napi_throw_type_error(Environment, NULL, "error: invalid or missing argument 1 for NodeMediaInfoList::Count_Get()");
            return Undefined(Environment); //undefined
        }

        size_t StreamNumber=(size_t)-1;
        if (ArgumentsCount>2)
            To_SizeT(Environment, Arguments[2], StreamNumber);

        Result=Instance->MI->Count_Get(Pos, (stream_t)StreamKind, StreamNumber);
    }
    else
    {
        Result=Instance->MI->Count_Get();
    }

    return From_SizeT(Environment, Result);
}
