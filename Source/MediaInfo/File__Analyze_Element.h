/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File__Analyze_ELEMENTH
#define MediaInfo_File__Analyze_ELEMENTH
//---------------------------------------------------------------------------

#include "MediaInfo/MediaInfo_Config.h"
#include <cstring>
#include <new>
#include <sstream>
#include <utility>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
struct element_details
{
#if MEDIAINFO_TRACE
    class Element_Node_Data;

    //String stored once, shared by the nodes using it (e.g. the same element name in each packet)
    class Element_Node_String
    {
        friend class Element_Node_Data;

    public:
        Element_Node_String() : Item(NULL) {}
        Element_Node_String(const Element_Node_String& v) : Item(v.Item) {if (Item) Item->Count++;}
        ~Element_Node_String() {clear();}
        Element_Node_String& operator=(const Element_Node_String& v)
        {
            item* New=v.Item;
            if (New)
                New->Count++;
            clear();
            Item=New;
            return *this;
        }

        void assign(const char* Data, size_t Size); //New string, not shared yet
        void clear() {if (Item && !--Item->Count) ::operator delete(Item); Item=NULL;}
        void swap(Element_Node_String& v) {item* Temp=Item; Item=v.Item; v.Item=Temp;}
        bool empty() const {return !Item;}
        size_t size() const {return Item?Item->Size:0;}
        const char* data() const {return Item?(const char*)(Item+1):"";}
        bool operator==(const char* v) const {return !strcmp(data(), v);}

    private:
        struct item //Followed by the characters and a NUL
        {
            size_t Count;
            size_t Size;
        };
        item* Item;
    };

    //Vector of pointers, only a pointer when empty (most of the nodes have no child)
    //The count of items is stored before the items, the capacity is the count rounded up to a power of 2
    template<typename T>
    class Element_Node_Pointers
    {
    public:
        Element_Node_Pointers() : Data(NULL) {}
        Element_Node_Pointers(const Element_Node_Pointers& v) : Data(NULL) {*this=v;}
        ~Element_Node_Pointers() {clear();}
        Element_Node_Pointers& operator=(const Element_Node_Pointers& v)
        {
            if (this!=&v)
            {
                clear();
                for (size_t i=0; i<v.size(); i++)
                    push_back(v[i]);
            }
            return *this;
        }

        size_t size() const {return Data?*Data:0;}
        bool empty() const {return !size();}
        T*& operator[](size_t i) {return Items()[i];}
        T* operator[](size_t i) const {return Items()[i];}
        void push_back(T* v)
        {
            size_t Size=size();
            if (!(Size&(Size-1))) //0 or a power of 2, the storage is full
                Grow(Size);
            Items()[Size]=v;
            (*Data)++;
        }
        void clear() {::operator delete(Data); Data=NULL;}
        void swap(Element_Node_Pointers& v) {size_t* Temp=Data; Data=v.Data; v.Data=Temp;}

    private:
        size_t* Data; //Count of items, followed by the items
        T** Items() const {return (T**)(Data+1);}
        void Grow(size_t Size)
        {
            size_t* New=(size_t*)::operator new(sizeof(size_t)+(Size?Size*2:1)*sizeof(T*));
            *New=Size;
            if (Size)
                std::memcpy((void*)(New+1), (const void*)(Data+1), Size*sizeof(T*));
            ::operator delete(Data);
            Data=New;
        }
    };

    //Vector of values, only a pointer when empty (most of the nodes have no info)
    //The count of items is stored before the items (on 64 bits for the alignment of the items), the capacity is the count rounded up to a power of 2
    template<typename T>
    class Element_Node_Values
    {
    public:
        Element_Node_Values() : Data(NULL) {}
        Element_Node_Values(const Element_Node_Values& v) : Data(NULL) {*this=v;}
        ~Element_Node_Values() {clear();}
        Element_Node_Values& operator=(const Element_Node_Values& v)
        {
            if (this!=&v)
            {
                clear();
                for (size_t i=0; i<v.size(); i++)
                    emplace_back(v[i]);
            }
            return *this;
        }

        size_t size() const {return Data?(size_t)*Data:0;}
        bool empty() const {return !size();}
        T& operator[](size_t i) {return Items()[i];}
        const T& operator[](size_t i) const {return Items()[i];}
        template<typename... Args>
        T& emplace_back(Args&&... args)
        {
            size_t Size=size();
            if (!(Size&(Size-1))) //0 or a power of 2, the storage is full
                Grow(Size);
            T* Item=new (Items()+Size) T(std::forward<Args>(args)...);
            (*Data)++;
            return *Item;
        }
        void clear()
        {
            size_t Size=size();
            for (size_t i=0; i<Size; i++)
                Items()[i].~T();
            ::operator delete(Data);
            Data=NULL;
        }
        void swap(Element_Node_Values& v) {int64u* Temp=Data; Data=v.Data; v.Data=Temp;}

    private:
        int64u* Data; //Count of items, followed by the items
        T* Items() const {return (T*)(Data+1);}
        void Grow(size_t Size)
        {
            int64u* New=(int64u*)::operator new(sizeof(int64u)+(Size?Size*2:1)*sizeof(T));
            *New=Size;
            T* New_Items=(T*)(New+1);
            for (size_t i=0; i<Size; i++)
            {
                new (New_Items+i) T(std::move(Items()[i]));
                Items()[i].~T();
            }
            ::operator delete(Data);
            Data=New;
        }
    };

    class Element_Node_Data
    {
    public:

        enum Value_Output_Format
        {
            Format_Tree,
            Format_Xml,
        };

        enum Value_Type
        {
            ELEMENT_NODE_NONE,
            ELEMENT_NODE_CHAR8,
            ELEMENT_NODE_STR,
            ELEMENT_NODE_BOOL,
            ELEMENT_NODE_INT8U,
            ELEMENT_NODE_INT8S,
            ELEMENT_NODE_INT16U,
            ELEMENT_NODE_INT16S,
            ELEMENT_NODE_INT32U,
            ELEMENT_NODE_INT32S,
            ELEMENT_NODE_INT64U,
            ELEMENT_NODE_INT64S,
            ELEMENT_NODE_INT128U,
            ELEMENT_NODE_FLOAT32,
            ELEMENT_NODE_FLOAT64,
            ELEMENT_NODE_FLOAT80,
            ELEMENT_NODE_STR_SHARED,
        };

        union Value
        {
            char*        Str;
            char         Chars[8];
            bool         b;
            int8u        i8u;
            int8s        i8s;
            int16u       i16u;
            int16s       i16s;
            int32u       i32u;
            int32s       i32s;
            int64u       i64u;
            int64s       i64s;
            float32      f32;
            float64      f64;
            float80     *f80;
            int128u     *i128u;
            void        *Shared; //Item of an Element_Node_String
        };

        Element_Node_Data() : type(ELEMENT_NODE_NONE), format_out(Format_Xml), Option((int8u)-1) {}
        ~Element_Node_Data() { clear(); }

        Element_Node_Data& operator=(const Element_Node_Data&);
        void operator=(const std::string& v);
        void operator=(const char* v);
        void operator=(const Ztring& v);
        void operator=(const Element_Node_String& v); //Shared if not stored in the node
        void operator=(bool v);
        void operator=(int8u v);
        void operator=(int8s v);
        void operator=(int16u v);
        void operator=(int16s v);
        void operator=(int32u v);
        void operator=(int32s v);
        void operator=(int64u v);
        void operator=(int64s v);
        void operator=(int128u v);
        void operator=(float32 v);
        void operator=(float64 v);
        void operator=(float80 v);

        bool operator==(const std::string& str);

        void clear();
        void swap(Element_Node_Data& v);
        bool empty() {return type == ELEMENT_NODE_NONE;}
        void set_Option(int8u c) {Option = c;}
        void Set_Output_Format(Value_Output_Format v) {format_out = v;}
        void Append(std::string& Out) const; //Append the value to Out, in the current output format

        static void get_hexa_from_deci_limited_by_bits(std::string& val, int8u bits, int8u default_bits);

    private:
        Value               val;
        int8u               type; //Value_Type
        int8u               format_out; //Value_Output_Format
        int8u               Option; // float: count of valid digits after comma; int: count of valid bits; chars: count of valid chars

        Element_Node_Data(const Element_Node_Data&);
    };

    struct Element_Node_Info
    {
        Element_Node_Info() {}
        template<typename T>
        Element_Node_Info(T parameter, int8u Option=(int8u)-1)
        {
            data.set_Option(Option);
            data = parameter;
        }
        Element_Node_Info(const Element_Node_Info& v)
        {
            data = v.data;
            Measure = v.Measure;
        }
        Element_Node_Info(Element_Node_Info&& v)
        {
            data.swap(v.data);
            Measure.swap(v.Measure);
        }

        Element_Node_Data   data;
        Element_Node_String Measure;

    private:
        Element_Node_Info& operator=(const Element_Node_Info&);
    };

    class Element_Node
    {
    public:
        Element_Node();
        Element_Node(const Element_Node& node);
        ~Element_Node();

        // Move
        void TakeChilrenFrom(Element_Node& node);

        int64u                           Pos;             // Position of the element in the file
        int64u                           Size;            // Size of the element (including header and sub-elements)
    private:
        Element_Node_String              Name;            // Name planned for this element
    public:
        Element_Node_Data                Value;           // The value (currently used only with Trace XML)
        Element_Node_Values<Element_Node_Info> Infos;     // More info about the element
        Element_Node_Pointers<Element_Node> Children;     // Elements depending on this element
        int32s                           Current_Child;   // Current child selected, used for param
        //Flags in 1 byte, so the node fits in 64 bytes
        bool                             NoShow:1;        // Don't show this element
        bool                             OwnChildren:1;   // Child is owned by this node
        bool                             IsCat:1;         // Node is a category
        bool                             HasError:1;      // Node or sub-nodes has Nok
        bool                             RemoveIfNoErrors:1;// Remove Children Node if no NOK appears

        void                             Init();          //Initialize with common values
        void Add_Child(Element_Node* node);               //Add a subchild to the current node, the content of node is moved
        void Set_Name(const string &Name_)
        {
            Name.assign(Name_.data(), Name_.size());
        }
        void Set_Name(const char* Name_)
        {
            Name.assign(Name_, Name_?strlen(Name_):0);
        }
        void Set_Name(const char* Name_, size_t Size)
        {
            Name.assign(Name_, Size);
        }
        void Set_Name(const Element_Node_String& Name_)
        {
            Name = Name_;
        }
        bool Name_Is_Empty() const {return Name.empty();}

        // Print
        int  Print(MediaInfo_Config::trace_Format Format, std::string& str, const string& eol, int64u File_Size);  //Print the node into str

    private:
        struct print_struc
        {
            std::string& Out;
            const string eol;
            const size_t offset_size;
            size_t level;

            print_struc(std::string& Out_, const string& eol_, size_t offset_size_)
                :
                Out(Out_),
                eol(eol_),
                offset_size(offset_size_),
                level(0)
            {
            }
        };
        int  Print_Xml(print_struc& s);                                            //Print the node in XML into s.Out
        int  Print_Micro_Xml(print_struc& s);                                      //Print the node in micro XML into s.Out
        int  Print_Tree(print_struc& s);                                           //Print the node into s.Out
        int  Print_Tree_Cat(print_struc& s);
    };
#endif //MEDIAINFO_TRACE

    int64u       Code;               //Code filled in the file
    int64u       Next;               //
    bool         WaitForMoreData;    //This element is not complete, we need more data
    bool         UnTrusted;          //This element has a problem
    bool         IsComplete;         //This element is fully buffered, no need of more

#if MEDIAINFO_TRACE
    Element_Node TraceNode;
#endif //MEDIAINFO_TRACE
};

} //NameSpace

#endif // !MediaInfo_File__Analyze_ELEMENTH
