/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include <string>
#include "ZenLib/Ztring.h"
#include "ZenLib/ZtringListList.h"
#include "ZenLib/File.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfoList.h"
#include <stdio.h>

struct Option
{
    ZenLib::Ztring media_filename;
    ZenLib::Ztring out_filename;
    ZenLib::Ztring details;
    ZenLib::Ztring read_by_human;
    ZenLib::Ztring language;
    ZenLib::Ztring out_format;
    ZenLib::ZtringListList extra_options;
};

static int usage()
{
    fprintf(stderr, "tool [--details|-d] [--human-readable|-h] [--format|-f output_format] [--language|-l output_language] [--extra-options|-e options] media_filename output_filename\n");
    return 1;
}

static int parse_options(int ac, char *av[], Option& opts)
{
    if (ac < 3)
        return usage();

    opts.out_format = __T("XML");
    opts.details = __T("0");
    opts.read_by_human = __T("0");
    opts.language = __T("raw");
    opts.extra_options.Separator_Set(0, __T(","));
    opts.extra_options.Separator_Set(1, __T(":"));

    int nb_arguments = 0;

    for (int i = 1; i < ac; ++i)
    {
        if (std::string("--details") == av[i] || std::string("-d") == av[i])
        {
            opts.details = __T("1");
        }
        else if (std::string("--human-readable") == av[i] || std::string("-h") == av[i])
        {
            opts.read_by_human = __T("1");
        }
        else if (std::string("--format") == av[i] || std::string("-f") == av[i])
        {
            ++i;
            if (i == ac)
                return usage();

            opts.out_format = ZenLib::Ztring().From_Local(av[i]);
        }
        else if (std::string("--language") == av[i] || std::string("-l") == av[i])
        {
            ++i;
            if (i == ac)
                return usage();

            opts.language = ZenLib::Ztring().From_Local(av[i]);
        }
        else if (std::string("--extra-options") == av[i] || std::string("-e") == av[i])
        {
            ++i;
            if (i == ac)
                return usage();

            opts.extra_options.Write(ZenLib::Ztring().From_Local(av[i]));
        }
        else
        {
            if (nb_arguments == 0)
                opts.media_filename = ZenLib::Ztring().From_Local(av[i]);
            else if (nb_arguments == 1)
                opts.out_filename = ZenLib::Ztring().From_Local(av[i]);
            else
                return usage();

            ++nb_arguments;
        }
    }

    if (nb_arguments != 2)
        return usage();

    return 0;
}

int main(int ac, char *av[])
{
    setlocale(LC_ALL, "");
    MediaInfoLib::MediaInfo::Option_Static(__T("CharSet"), __T(""));

    Option opts;
    if (parse_options(ac, av, opts))
        return 1;

    ZenLib::File out;
    out.Create(opts.out_filename);
    out.Open(opts.out_filename, ZenLib::File::Access_Write);
    if (!out.Opened_Get())
    {
        fprintf(stderr, "Cannot open output file:%s\n", opts.out_filename.To_UTF8().c_str());
        return 1;
    }

    MediaInfoLib::MediaInfoList mi;

    mi.Option(__T("Details"), opts.details);
    mi.Option(__T("ReadByHuman"), opts.read_by_human);
    mi.Option(__T("Language"), opts.language);
    mi.Option(__T("Inform"), opts.out_format);

    for (int i = 0; i < opts.extra_options.size(); ++i)
    {
        ZenLib::Ztring option = opts.extra_options.Read(i, 0);
        ZenLib::Ztring value = opts.extra_options.Read(i, 1);

        mi.Option(option, value);
    }

    if (mi.Open(opts.media_filename) == 0)
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
