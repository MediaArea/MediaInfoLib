/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// TODO: very ugly automation, it deserves a refactoring
// Usage: ..\..\.. ..\..\..\..\MediaArea-Website

#include <iostream>
#include <string>
#include "ZenLib/ZtringListList.h"
#include "ZenLib/File.h"
using namespace ZenLib;
using namespace std;

const char* StreamKinds[] =
{
    "General",
    "Video",
    "Audio",
    "Text",
    "Other",
    "Image",
    "Menu",
};

ZtringListList Load(const string& Name, const char* ToAdd = nullptr, bool IgnoreErrors = false) {
    File InF(Ztring().From_Local(Name));
    auto Size = InF.Size_Get();
    if (Size == 0) {
        if (!IgnoreErrors)
            std::cerr << "Can not open " << Name << '\n';
        return {};
    }
    uint8_t* Buffer = new uint8_t[Size];
    if (InF.Read(Buffer, Size) != Size) {
        if (!IgnoreErrors)
            std::cerr << "Can not read " << Name << '\n';
        return {};
    }
    Ztring In;
    In.From_UTF8((const char*)Buffer, Size);
    delete[] Buffer;

    if (ToAdd) {
        In += Ztring().From_UTF8(ToAdd);
    }

    ZtringListList List;
    List.Write(In);
    if (List.empty()) {
        if (!IgnoreErrors)
            std::cerr << "Can not extract " << Name << '\n';
    }

    return std::move(List);
}


int main(int argc, char* argv[]) {
    string ToInsert;
    size_t i = 1;
    for (const auto& StreamKind : StreamKinds) {
        ToInsert += R"( <thead>
  <tr onclick="Line_Show(')";
        ToInsert += StreamKind;
        ToInsert += R"x(')" id=")x";
        ToInsert += StreamKind;
        ToInsert += R"(H" style="cursor: pointer">
   <td colspan="5" class="xheader">)";
        ToInsert += StreamKind;
        ToInsert += R"(</td>
  </tr>
  <tr onclick="Line_Hide(')";
        ToInsert += StreamKind;
        ToInsert += R"x(')" id=")x";
        ToInsert += StreamKind;
        ToInsert += R"(S" style="cursor: pointer">
   <td colspan="5" class="xheader">)";
        ToInsert += StreamKind;
        ToInsert += R"(</td>
  </tr>
 </thead>
 <tbody id=")";
        ToInsert += StreamKind;
        ToInsert += R"(">
)";

        string Name = string(argv[1]) + "/Source/Resource/Text/Stream/" + StreamKind + ".csv";
        string Name_Mapping = string(argv[1]) + "/Source/Resource/Text/Mapping/" + StreamKind + ".csv";

        const char* ToAdd = nullptr;
        if (strcmp(StreamKind, "General") == 0) {
            ToAdd = "IsTruncated;;; Y YIY;;; Indicate if the file is detected as truncated\n";
        }

        auto List = Load(Name, ToAdd);
        auto IgnoreMissingMapping = strcmp(StreamKind, "General") != 0;
        auto Mapping = Load(Name_Mapping, nullptr, IgnoreMissingMapping);
        if (List.empty() || (!IgnoreMissingMapping && Mapping.empty())) {
            return 1;
        }


        for (const auto& Line : List)
        {
            if (Line.size() <= 6 || Line[3].size() <= 2 || Line[3][2] != __T('Y'))
                continue;
            ToInsert += R"(  <tr>
   <td id="mi)";
            ToInsert += to_string(i);
            ToInsert += R"(">)";
            ToInsert += Line[0].To_UTF8();
            ToInsert += R"(</td>
   <td id="de)";
            ToInsert += to_string(i);
            ToInsert += R"(">)";
            if (!Line[6].empty())
                ToInsert += Line[6].To_UTF8();
            else
                ToInsert += "&nbsp;";
            ToInsert += R"(</td>
   <td id="mp)";
            ToInsert += to_string(i);
            ToInsert += R"(">)";
            auto j = Mapping.Find(Line[0]);

            if (j != (size_t)-1 && Mapping[j].size() > 1 && !Mapping[j][1].empty())
                ToInsert += Mapping[j][1].To_UTF8();
            else
                ToInsert += "&nbsp;";
            ToInsert += R"(</td>
   <td id="eb)";
            ToInsert += to_string(i);
            ToInsert += R"(">)";
            if (j != (size_t)-1 && Mapping[j].size() > 2 && !Mapping[j][2].empty())
                ToInsert += Mapping[j][2].To_UTF8();
            else
                ToInsert += "&nbsp;";
            ToInsert += R"(</td>
   <td id="pb)";
            ToInsert += to_string(i);
            ToInsert += R"(">)";
            if (j != (size_t)-1 && Mapping[j].size() > 3 && !Mapping[j][3].empty())
                ToInsert += Mapping[j][3].To_UTF8();
            else
                ToInsert += "&nbsp;";
            ToInsert += R"(</td>
  </tr>
)";
            i++;
        }
        ToInsert += R"( </tbody>
)";
    }

    string Name = string(argv[2]) + "/src/MediaInfoBundle/Resources/views/Support/fields.html.twig";
    File OutputF(Ztring().From_Local(Name));
    auto Size = OutputF.Size_Get();
    if (Size == 0) {
        std::cerr << "Can not open " << Name << '\n';
        return 1;
    }
    uint8_t* Buffer = new uint8_t[Size + 1];
    if (OutputF.Read(Buffer, Size) != Size) {
        std::cerr << "Can not read " << Name << '\n';
        return 1;
    }
    Buffer[Size] = '\0';
    auto In = strstr((const char*)Buffer, " <thead>");
    if (!In) {
        std::cerr << "Can find start in " << Name << '\n';
        return 1;
    }
    In = strstr(In + 1, " <thead>");
    if (!In) {
        std::cerr << "Can find start in " << Name << '\n';
        return 1;
    }
    auto Out = strstr(In + 1, "</table>");
    if (!Out) {
        std::cerr << "Can find end in " << Name << '\n';
        return 1;
    }

    OutputF.Close();
    OutputF.Create(Ztring().From_Local(Name), true);
    OutputF.Write(Buffer, In - (const char*)Buffer);
    OutputF.Write((const int8u*)ToInsert.c_str(), ToInsert.size());
    OutputF.Write((const int8u*)Out, (const char*)Buffer + Size - Out);
    OutputF.Close();

    delete[] Buffer;

    return 0;
}
