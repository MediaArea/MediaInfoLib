/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "ZenLib/Ztring.h"
#include "ZenLib/File.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfoList.h"
#include <stdio.h>

int main(int ac, char *av[])
{
    setlocale(LC_ALL, "");
    MediaInfoLib::MediaInfo::Option_Static(__T("CharSet"), __T(""));

    if (ac != 3)
    {
        fprintf(stderr, "Only a media filename and an output filename are required\n");
        return 1;
    }

    ZenLib::Ztring media_filename = ZenLib::Ztring().From_Local(av[1]);
    ZenLib::Ztring out_filename = ZenLib::Ztring().From_Local(av[2]);
    ZenLib::File   out;

    out.Create(out_filename);
    out.Open(out_filename, ZenLib::File::Access_Write);
    if (!out.Opened_Get())
    {
        fprintf(stderr, "Cannot open output file:%s\n", out_filename.To_UTF8().c_str());
        return 1;
    }

    MediaInfoLib::MediaInfoList mi;

    mi.Option(__T("Details"), __T("1"));
    mi.Option(__T("ReadByHuman"), __T("1"));
    mi.Option(__T("Language"), __T("raw"));
    mi.Option(__T("Inform"), __T("XML"));

    if (mi.Open(media_filename) == 0)
    {
        fprintf(stderr, "Cannot open media file\n");
        return 1;
    }
    if (out.Write(mi.Inform()) == 0)
    {
        fprintf(stderr, "Cannot write to output file\n");
        return 1;
    }
    out.Close();
    mi.Close();

    return 0;
}
