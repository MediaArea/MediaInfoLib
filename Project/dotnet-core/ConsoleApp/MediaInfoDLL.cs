/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Microsoft Visual C# wrapper for MediaInfo Library
// See MediaInfo.h for help
//
// To make it working on Windows, you must put MediaInfo.dll
// in the executable folder
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using System;
using System.Text;
using System.Runtime.InteropServices;

#pragma warning disable 1591 // Disable XML documentation warnings

namespace MediaInfoLib
{
    public enum StreamKind
    {
        General,
        Video,
        Audio,
        Text,
        Other,
        Image,
        Menu,
    }

    public enum InfoKind
    {
        Name,
        Text,
        Measure,
        Options,
        NameText,
        MeasureText,
        Info,
        HowTo
    }

    public enum InfoOptions
    {
        ShowInInform,
        Support,
        ShowInSupported,
        TypeOfValue
    }

    public enum InfoFileOptions
    {
        FileOption_Nothing      = 0x00,
        FileOption_NoRecursive  = 0x01,
        FileOption_CloseAll     = 0x02,
        FileOption_Max          = 0x04
    };

    public enum Status
    {
        None        =       0x00,
        Accepted    =       0x01,
        Filled      =       0x02,
        Updated     =       0x04,
        Finalized   =       0x08,
    }

    public class MediaInfo
    {
        //Import of DLL functions. DO NOT USE until you know what you do (MediaInfo DLL do NOT use CoTaskMemAlloc to allocate memory)
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_New();
        [DllImport("mediainfo")]
        private static extern void   MediaInfo_Delete(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Open(IntPtr Handle, IntPtr FileName);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Open_Buffer_Init(IntPtr Handle, Int64 File_Size, Int64 File_Offset);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Open_Buffer_Continue(IntPtr Handle, IntPtr Buffer, IntPtr Buffer_Size);
        [DllImport("mediainfo")]
        private static extern Int64  MediaInfo_Open_Buffer_Continue_GoTo_Get(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Open_Buffer_Finalize(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern void   MediaInfo_Close(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Inform(IntPtr Handle, IntPtr Reserved);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_GetI(IntPtr Handle, IntPtr StreamKind, IntPtr StreamNumber, IntPtr Parameter, IntPtr KindOfInfo);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Get(IntPtr Handle, IntPtr StreamKind, IntPtr StreamNumber, IntPtr Parameter, IntPtr KindOfInfo, IntPtr KindOfSearch);
        [DllImport("mediainfo", CharSet=CharSet.Unicode)]
        private static extern IntPtr MediaInfo_Option(IntPtr Handle, IntPtr Option, IntPtr Value);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_State_Get(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfo_Count_Get(IntPtr Handle, IntPtr StreamKind, IntPtr StreamNumber);

        //Helpers
        private static string String_From_UTF32_Ptr(IntPtr Ptr)
        {
            int Length = 0;
            while (Marshal.ReadInt32(Ptr, Length)!=0)
                Length+=4;

            byte[] Buffer = new byte[Length];
            Marshal.Copy(Ptr, Buffer, 0, Buffer.Length);
            return new UTF32Encoding(!BitConverter.IsLittleEndian, false, false).GetString(Buffer);
        }

        private static IntPtr UTF32_Ptr_From_String(string Str)
        {
            Encoding Codec = new UTF32Encoding(!BitConverter.IsLittleEndian, false, false);
            int Length = Codec.GetByteCount(Str);
            byte[] Buffer = new byte[Length+4];
            Codec.GetBytes(Str, 0, Str.Length, Buffer, 0);
            IntPtr Ptr = Marshal.AllocHGlobal(Buffer.Length);
            Marshal.Copy(Buffer, 0, Ptr, Buffer.Length);
            return Ptr;
        }

        //MediaInfo class
        public MediaInfo()
        {
            try
            {
                Handle = MediaInfo_New();
            }
            catch
            {
                Handle = (IntPtr)0;
            }
            if (Environment.OSVersion.ToString().IndexOf("Windows") == -1)
            {
                UnicodeIs32Bits=true;
                Option("setlocale_LC_CTYPE", "");
            }
            else
            {
                UnicodeIs32Bits=false;
            }
        }
        ~MediaInfo() { if (Handle == (IntPtr)0) return; MediaInfo_Delete(Handle); }
        public int Open(String FileName)
        {
            if (Handle == (IntPtr)0)
                return 0;
            if (UnicodeIs32Bits)
            {
                IntPtr FileName_Ptr = UTF32_Ptr_From_String(FileName);
                int ToReturn = (int)MediaInfo_Open(Handle, FileName_Ptr);
                Marshal.FreeHGlobal(FileName_Ptr);
                return ToReturn;
            }
            else
            {
                IntPtr FileName_Ptr = Marshal.StringToHGlobalUni(FileName);
                int ToReturn = (int)MediaInfo_Open(Handle, FileName_Ptr);
                Marshal.FreeHGlobal(FileName_Ptr);
                return ToReturn;
            }
        }
        public int Open_Buffer_Init(Int64 File_Size, Int64 File_Offset)
        {
            if (Handle == (IntPtr)0) return 0; return (int)MediaInfo_Open_Buffer_Init(Handle, File_Size, File_Offset);
        }
        public int Open_Buffer_Continue(IntPtr Buffer, IntPtr Buffer_Size)
        {
            if (Handle == (IntPtr)0) return 0; return (int)MediaInfo_Open_Buffer_Continue(Handle, Buffer, Buffer_Size);
        }
        public Int64 Open_Buffer_Continue_GoTo_Get()
        {
            if (Handle == (IntPtr)0) return 0; return (Int64)MediaInfo_Open_Buffer_Continue_GoTo_Get(Handle);
        }
        public int Open_Buffer_Finalize()
        {
            if (Handle == (IntPtr)0) return 0; return (int)MediaInfo_Open_Buffer_Finalize(Handle);
        }
        public void Close() { if (Handle == (IntPtr)0) return; MediaInfo_Close(Handle); }
        public String Inform()
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
                return String_From_UTF32_Ptr(MediaInfo_Inform(Handle, (IntPtr)0));
            else
                return Marshal.PtrToStringUni(MediaInfo_Inform(Handle, (IntPtr)0));
        }
        public String Get(StreamKind StreamKind, int StreamNumber, String Parameter, InfoKind KindOfInfo, InfoKind KindOfSearch)
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
            {
                IntPtr Parameter_Ptr=UTF32_Ptr_From_String(Parameter);
                String ToReturn=String_From_UTF32_Ptr(MediaInfo_Get(Handle, (IntPtr)StreamKind, (IntPtr)StreamNumber, Parameter_Ptr, (IntPtr)KindOfInfo, (IntPtr)KindOfSearch));
                Marshal.FreeHGlobal(Parameter_Ptr);
                return ToReturn;
            }
            else
            {
                IntPtr Parameter_Ptr=Marshal.StringToHGlobalUni(Parameter);
                String ToReturn=Marshal.PtrToStringUni(MediaInfo_Get(Handle, (IntPtr)StreamKind, (IntPtr)StreamNumber, Parameter_Ptr, (IntPtr)KindOfInfo, (IntPtr)KindOfSearch));
                Marshal.FreeHGlobal(Parameter_Ptr);
                return ToReturn;
            }
        }
        public String Get(StreamKind StreamKind, int StreamNumber, int Parameter, InfoKind KindOfInfo)
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
                return String_From_UTF32_Ptr(MediaInfo_GetI(Handle, (IntPtr)StreamKind, (IntPtr)StreamNumber, (IntPtr)Parameter, (IntPtr)KindOfInfo));
            else
                return Marshal.PtrToStringUni(MediaInfo_GetI(Handle, (IntPtr)StreamKind, (IntPtr)StreamNumber, (IntPtr)Parameter, (IntPtr)KindOfInfo));
        }
        public String Option(String Option, String Value)
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
            {
                IntPtr Option_Ptr=UTF32_Ptr_From_String(Option);
                IntPtr Value_Ptr=UTF32_Ptr_From_String(Value);
                String ToReturn=String_From_UTF32_Ptr(MediaInfo_Option(Handle, Option_Ptr, Value_Ptr));
                Marshal.FreeHGlobal(Option_Ptr);
                Marshal.FreeHGlobal(Value_Ptr);
                return ToReturn;
            }
            else
            {
                IntPtr Option_Ptr=Marshal.StringToHGlobalUni(Option);
                IntPtr Value_Ptr=Marshal.StringToHGlobalUni(Value);
                String ToReturn=Marshal.PtrToStringUni(MediaInfo_Option(Handle, Option_Ptr, Value_Ptr));
                Marshal.FreeHGlobal(Option_Ptr);
                Marshal.FreeHGlobal(Value_Ptr);
                return ToReturn;
            }
        }
        public int State_Get() { if (Handle == (IntPtr)0) return 0; return (int)MediaInfo_State_Get(Handle); }
        public int Count_Get(StreamKind StreamKind, int StreamNumber) { if (Handle == (IntPtr)0) return 0; return (int)MediaInfo_Count_Get(Handle, (IntPtr)StreamKind, (IntPtr)StreamNumber); }
        private IntPtr Handle;
        private bool UnicodeIs32Bits;

        //Default values, if you know how to set default values in C#, say me
        public String Get(StreamKind StreamKind, int StreamNumber, String Parameter, InfoKind KindOfInfo) { return Get(StreamKind, StreamNumber, Parameter, KindOfInfo, InfoKind.Name); }
        public String Get(StreamKind StreamKind, int StreamNumber, String Parameter) { return Get(StreamKind, StreamNumber, Parameter, InfoKind.Text, InfoKind.Name); }
        public String Get(StreamKind StreamKind, int StreamNumber, int Parameter) { return Get(StreamKind, StreamNumber, Parameter, InfoKind.Text); }
        public String Option(String Option_) { return Option(Option_, ""); }
        public int Count_Get(StreamKind StreamKind) { return Count_Get(StreamKind, -1); }
    }

    public class MediaInfoList
    {
        //Import of DLL functions. DO NOT USE until you know what you do (MediaInfo DLL do NOT use CoTaskMemAlloc to allocate memory)
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_New();
        [DllImport("mediainfo")]
        private static extern void MediaInfoList_Delete(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_Open(IntPtr Handle, IntPtr FileName, IntPtr Options);
        [DllImport("mediainfo")]
        private static extern void MediaInfoList_Close(IntPtr Handle, IntPtr FilePos);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_Inform(IntPtr Handle, IntPtr FilePos, IntPtr Reserved);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_GetI(IntPtr Handle, IntPtr FilePos, IntPtr StreamKind, IntPtr StreamNumber, IntPtr Parameter, IntPtr KindOfInfo);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_Get(IntPtr Handle, IntPtr FilePos, IntPtr StreamKind, IntPtr StreamNumber, IntPtr Parameter, IntPtr KindOfInfo, IntPtr KindOfSearch);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_Option(IntPtr Handle, IntPtr Option, IntPtr Value);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_State_Get(IntPtr Handle);
        [DllImport("mediainfo")]
        private static extern IntPtr MediaInfoList_Count_Get(IntPtr Handle, IntPtr FilePos, IntPtr StreamKind, IntPtr StreamNumber);

        //Helpers
        private static string String_From_UTF32_Ptr(IntPtr Ptr)
        {
            int Length = 0;
            while (Marshal.ReadInt32(Ptr, Length)!=0)
                Length+=4;

            byte[] Buffer = new byte[Length];
            Marshal.Copy(Ptr, Buffer, 0, Buffer.Length);
            return new UTF32Encoding(!BitConverter.IsLittleEndian, false, false).GetString(Buffer);
        }

        private static IntPtr UTF32_Ptr_From_String(string Str)
        {
            Encoding Codec = new UTF32Encoding(!BitConverter.IsLittleEndian, false, false);
            int Length = Codec.GetByteCount(Str);
            byte[] Buffer = new byte[Length+4];
            Codec.GetBytes(Str, 0, Str.Length, Buffer, 0);
            IntPtr Ptr = Marshal.AllocHGlobal(Buffer.Length);
            Marshal.Copy(Buffer, 0, Ptr, Buffer.Length);
            return Ptr;
        }

        //MediaInfo class
        public MediaInfoList()
        {
            try
            {
                Handle = MediaInfoList_New();
            }
            catch
            {
                Handle = (IntPtr)0;
            }
            if (Environment.OSVersion.ToString().IndexOf("Windows") == -1)
            {
                UnicodeIs32Bits=true;
                Option("setlocale_LC_CTYPE", ""); // Load system default locale
            }
            else
            {
                UnicodeIs32Bits=false;
            }
        }
        ~MediaInfoList() { if (Handle == (IntPtr)0) return; MediaInfoList_Delete(Handle); }
        public int Open(String FileName, InfoFileOptions Options)
        {
            if (Handle == (IntPtr)0)
                return 0;
            if (UnicodeIs32Bits)
            {
Console.WriteLine("Called");
                IntPtr FileName_Ptr = UTF32_Ptr_From_String(FileName);
                int ToReturn = (int)MediaInfoList_Open(Handle, FileName_Ptr, (IntPtr)Options);
                Marshal.FreeHGlobal(FileName_Ptr);
                return ToReturn;
            }
            else
            {
                IntPtr FileName_Ptr = Marshal.StringToHGlobalUni(FileName);
                int ToReturn = (int)MediaInfoList_Open(Handle, FileName_Ptr, (IntPtr)Options);
                Marshal.FreeHGlobal(FileName_Ptr);
                return ToReturn;
            }
        }
        public void Close(int FilePos) { if (Handle == (IntPtr)0) return; MediaInfoList_Close(Handle, (IntPtr)FilePos); }
        public String Inform(int FilePos)
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
                return String_From_UTF32_Ptr(MediaInfoList_Inform(Handle, (IntPtr)FilePos, (IntPtr)0));
            else
                return Marshal.PtrToStringUni(MediaInfoList_Inform(Handle, (IntPtr)FilePos, (IntPtr)0));
        }
        public String Get(int FilePos, StreamKind StreamKind, int StreamNumber, String Parameter, InfoKind KindOfInfo, InfoKind KindOfSearch)
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
            {
                IntPtr Parameter_Ptr=UTF32_Ptr_From_String(Parameter);
                String ToReturn=String_From_UTF32_Ptr(MediaInfoList_Get(Handle, (IntPtr)FilePos, (IntPtr)StreamKind, (IntPtr)StreamNumber, Parameter_Ptr, (IntPtr)KindOfInfo, (IntPtr)KindOfSearch));
                Marshal.FreeHGlobal(Parameter_Ptr);
                return ToReturn;
            }
            else
            {
                IntPtr Parameter_Ptr=Marshal.StringToHGlobalUni(Parameter);
                String ToReturn=Marshal.PtrToStringUni(MediaInfoList_Get(Handle, (IntPtr)FilePos, (IntPtr)StreamKind, (IntPtr)StreamNumber, Parameter_Ptr, (IntPtr)KindOfInfo, (IntPtr)KindOfSearch));
                Marshal.FreeHGlobal(Parameter_Ptr);
                return ToReturn;
            }
        }
        public String Get(int FilePos, StreamKind StreamKind, int StreamNumber, int Parameter, InfoKind KindOfInfo) {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
                return String_From_UTF32_Ptr(MediaInfoList_GetI(Handle, (IntPtr)FilePos, (IntPtr)StreamKind, (IntPtr)StreamNumber, (IntPtr)Parameter, (IntPtr)KindOfInfo));
            else
                return Marshal.PtrToStringUni(MediaInfoList_GetI(Handle, (IntPtr)FilePos, (IntPtr)StreamKind, (IntPtr)StreamNumber, (IntPtr)Parameter, (IntPtr)KindOfInfo));
        }
        public String Option(String Option, String Value)
        {
            if (Handle == (IntPtr)0)
                return "Unable to load MediaInfo library";
            if (UnicodeIs32Bits)
            {
                IntPtr Option_Ptr=UTF32_Ptr_From_String(Option);
                IntPtr Value_Ptr=UTF32_Ptr_From_String(Value);
                String ToReturn=String_From_UTF32_Ptr(MediaInfoList_Option(Handle, Option_Ptr, Value_Ptr));
                Marshal.FreeHGlobal(Option_Ptr);
                Marshal.FreeHGlobal(Value_Ptr);
                return ToReturn;
            }
            else
            {
                IntPtr Option_Ptr=Marshal.StringToHGlobalUni(Option);
                IntPtr Value_Ptr=Marshal.StringToHGlobalUni(Value);
                String ToReturn=Marshal.PtrToStringUni(MediaInfoList_Option(Handle, Option_Ptr, Value_Ptr));
                Marshal.FreeHGlobal(Option_Ptr);
                Marshal.FreeHGlobal(Value_Ptr);
                return ToReturn;
            }
        }
        public int State_Get() { if (Handle == (IntPtr)0) return 0; return (int)MediaInfoList_State_Get(Handle); }
        public int Count_Get(int FilePos, StreamKind StreamKind, int StreamNumber) { if (Handle == (IntPtr)0) return 0; return (int)MediaInfoList_Count_Get(Handle, (IntPtr)FilePos, (IntPtr)StreamKind, (IntPtr)StreamNumber); }
        private IntPtr Handle;
        private bool UnicodeIs32Bits;

        //Default values, if you know how to set default values in C#, say me
        public int Open(String FileName) { return Open(FileName, 0); }
        public void Close() { Close(-1); }
        public String Get(int FilePos, StreamKind StreamKind, int StreamNumber, String Parameter, InfoKind KindOfInfo) { return Get(FilePos, StreamKind, StreamNumber, Parameter, KindOfInfo, InfoKind.Name); }
        public String Get(int FilePos, StreamKind StreamKind, int StreamNumber, String Parameter) { return Get(FilePos, StreamKind, StreamNumber, Parameter, InfoKind.Text, InfoKind.Name); }
        public String Get(int FilePos, StreamKind StreamKind, int StreamNumber, int Parameter) { return Get(FilePos, StreamKind, StreamNumber, Parameter, InfoKind.Text); }
        public String Option(String Option_) { return Option(Option_, ""); }
        public int Count_Get(int FilePos, StreamKind StreamKind) { return Count_Get(FilePos, StreamKind, -1); }
    }

} //NameSpace
