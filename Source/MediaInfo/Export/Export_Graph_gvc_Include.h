/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Export_Graph_gvc_IncludeH
#define Export_Graph_gvc_IncludeH
//---------------------------------------------------------------------------

//***************************************************************************
// Needed gvc.h and cgraph.h definitions
//***************************************************************************

typedef void GVC_t;
typedef void graph_t;


//***************************************************************************
// Dynamic load stuff
//***************************************************************************

extern "C"
{

#if defined (_WIN32) || defined (WIN32)
    #ifndef _UNICODE
        #define GVCDLL_NAME "gvc.dll"
        #define CGRAPHDLL_NAME "cgraph.dll"
    #else //_UNICODE
        #define GVCDLL_NAME L"gvc.dll"
        #define CGRAPHDLL_NAME L"cgraph.dll"
    #endif
#elif defined(__APPLE__) && defined(__MACH__)
    #define GVCDLL_NAME "libgvc.6.dylib"
    #define CGRAPHDLL_NAME "libcgraph.6.dylib"
#else
    #define GVCDLL_NAME "libgvc.so.6"
    #define CGRAPHDLL_NAME "libcgraph.so.6"
#endif //!defined(_WIN32) || defined (WIN32)

#ifdef MEDIAINFO_GLIBC
    #include <gmodule.h>
    static GModule* gvc_Module=NULL;
    static GModule* cgraph_Module=NULL;
#elif defined (_WIN32) || defined (WIN32)
    #undef __TEXT
    #if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
        namespace WindowsNamespace
        {
    #endif
    #include "windows.h"
    #if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
        }
        using namespace WindowsNamespace;
    #endif
    static HMODULE gvc_Module=NULL;
    static HMODULE cgraph_Module=NULL;
#else
    #include <dlfcn.h>
    static void* gvc_Module=NULL;
    static void* cgraph_Module=NULL;
#endif

#ifdef MEDIAINFO_GLIBC
    #define ASSIGN(_Module, _Name) \
        if (!g_module_symbol(_Module, #_Name, (gpointer*)&_Name)) \
            Error=true;
    #define DLOPEN(_Module, _Name) \
        _Module=g_module_open(_Name, G_MODULE_BIND_LAZY);
    #define DLCLOSE(_Module) \
        g_module_close(_Module);
#elif defined (_WIN32) || defined (WIN32)
    #define ASSIGN(_Module, _Name) \
        _Name=(_Module##_##_Name)GetProcAddress(_Module, #_Name); \
        if (_Name==NULL) Error=true;
    #ifdef WINDOWS_UWP
        #define DLOPEN(_Module, _Name) \
            _Module=LoadPackagedLibrary(_Name, 0);
    #else
        #define DLOPEN(_Module, _Name) \
            _Module=LoadLibrary(_Name);
    #endif //WINDOWS_UWP
    #define DLCLOSE(_Module) \
        FreeLibrary(_Module);
#else
    #define ASSIGN(_Module, _Name) \
        _Name=(_Module##_##_Name)dlsym(_Module, #_Name); \
        if (_Name==NULL) Error=true;
    #define DLOPEN(_Module, _Name) \
        _Module=dlopen(_Name, RTLD_LAZY); \
        if (!_Module) \
            _Module=dlopen("./" _Name, RTLD_LAZY); \
        if (!_Module) \
            _Module=dlopen("/usr/local/lib/" _Name, RTLD_LAZY); \
        if (!_Module) \
            _Module=dlopen("/usr/local/lib64/" _Name, RTLD_LAZY); \
        if (!_Module) \
            _Module=dlopen("/usr/lib/" _Name, RTLD_LAZY); \
        if (!_Module) \
            _Module=dlopen("/usr/lib64/" _Name, RTLD_LAZY);
    #define DLCLOSE(_Module) \
        dlclose(_Module);
#endif //MEDIAINFO_GLIBC

//---------------------------------------------------------------------------
// Interface
typedef GVC_t* (*gvc_Module_gvContext)(); static gvc_Module_gvContext gvContext=NULL;
typedef int (*gvc_Module_gvFreeContext)(GVC_t*); static gvc_Module_gvFreeContext gvFreeContext=NULL;
typedef int (*gvc_Module_gvLayout)(GVC_t*, graph_t*, const char*); static gvc_Module_gvLayout gvLayout=NULL;
typedef int (*gvc_Module_gvFreeLayout)(GVC_t*, graph_t*); static gvc_Module_gvFreeLayout gvFreeLayout=NULL;
typedef int (*gvc_Module_gvRenderData)(GVC_t*, graph_t*, const char*, char**, unsigned int*); static gvc_Module_gvRenderData gvRenderData=NULL;
typedef void (*gvc_Module_gvFreeRenderData)(char*); static gvc_Module_gvFreeRenderData gvFreeRenderData=NULL;
typedef void (*gvc_Module_gvFinalize)(GVC_t*); static gvc_Module_gvFinalize gvFinalize=NULL;
typedef graph_t* (*cgraph_Module_agmemread)(const char*); static cgraph_Module_agmemread agmemread=NULL;
typedef int (*cgraph_Module_agclose)(graph_t*); static cgraph_Module_agclose agclose=NULL;

}

#endif
