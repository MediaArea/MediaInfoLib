/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

class MediaInfo
{
    static
    {
        // libmediainfo for linux depends on libzen
        String os=System.getProperty("os.name");
        if (os!=null && !os.toLowerCase().startsWith("windows") && !os.toLowerCase().startsWith("mac"))
        {
           try
           {
               System.loadLibrary("zen");
           }
           catch (LinkageError e)
           {
           }
        }

        try
        {
            System.loadLibrary("mediainfo");
        }
        catch (LinkageError e)
        {
            throw new UnsatisfiedLinkError("Unable to load library 'mediainfo'");
        }
    }

    public long mi = 0; // Pointer to MediaInfo instance, DO NOT RENAME without editing the corresponding variable in C++ source

    // Constructor
    public MediaInfo()
    {
        try
        {
          mi = Init();
        }
        catch (LinkageError e)
        {
          throw new UnsatisfiedLinkError("Library 'mediainfo' found but its JNI interface is missing");
        }
    }

    public enum StreamKind {
        General,
        Video,
        Audio,
        Text,
        Other,
        Image,
        Menu;
    }

    //Enums
    public enum InfoKind {
        /**
         * Unique name of parameter.
         */
        Name,

        /**
         * Value of parameter.
         */
        Text,

        /**
         * Unique name of measure unit of parameter.
         */
        Measure,

        Options,

        /**
         * Translated name of parameter.
         */
        Name_Text,

        /**
         * Translated name of measure unit.
         */
        Measure_Text,

        /**
         * More information about the parameter.
         */
        Info,

        /**
         * How this parameter is supported, could be N (No), B (Beta), R (Read only), W
         * (Read/Write).
         */
        HowTo,

        /**
         * Domain of this piece of information.
         */
        Domain;
    }

    public enum Status {
        None        (0x00),
        Accepted    (0x01),
        Filled      (0x02),
        Updated     (0x04),
        Finalized   (0x08);

        private int value;
        private Status(int value) {this.value = value;}
        public int getValue(int value) {return value;}
    }

     public void dispose()
    {
        Destroy();
        mi = 0;
    }

    @Override
    protected void finalize() throws Throwable
    {
        Destroy();
    }

    public native long Init();

    public native int Destroy();

    //File
    /**
     * Open a file and collect information about it (technical information and tags).
     *
     * @param file full name of the file to open
     * @return 1 if file was opened, 0 if file was not not opened
     */
    public native int Open(String name);

    public native int Open_Buffer_Init(long fileSize, long fileOffset);

    /**
     *  Open a stream and collect information about it (technical information and tags) (By buffer, Continue)

     * @param buffer pointer to the stream
     * @param size Count of bytes to read
     * @return a bitfield
                bit 0: Is Accepted (format is known)
                bit 1: Is Filled (main data is collected)
                bit 2: Is Updated (some data have beed updated, example: duration for a real time MPEG-TS stream)
                bit 3: Is Finalized (No more data is needed, will not use further data)
                bit 4-15: Reserved
                bit 16-31: User defined
     */
    public native int Open_Buffer_Continue(byte[] buffer, long bufferSize);


    public native long Open_Buffer_Continue_GoTo_Get();

    public native long Open_Buffer_Finalize();

    /**
     * Close a file opened before with Open().
     *
     */
    public native int Close();

    //Information
    /**
     * Get all details about a file.
     *
     * @return All details about a file in one string
     */
    public native String Inform();

    private native String GetI(int streamKind, int streamNumber, int parameter, int infoKind); 

    private native String GetS(int streamKind, int streamNumber, String parameter, int infoKind, int searchKind);

    /**
     * Get a piece of information about a file (parameter is an integer).
     *

     * @param StreamKind Kind of Stream (general, video, audio...)
     * @param StreamNumber Stream number in Kind of Stream (first, second...)
     * @param parameter Parameter you are looking for in the Stream (Codec, width, bitrate...),
     *            in integer format (first parameter, second parameter...)
     * @return a string about information you search, an empty string if there is a problem
     */
    public String Get(StreamKind streamKind, int streamNumber, int parameter)
    {
        return GetI(streamKind.ordinal(), streamNumber, parameter, InfoKind.Text.ordinal());
    }

    /**
     * Get a piece of information about a file (parameter is an integer).
     *

     * @param StreamKind Kind of Stream (general, video, audio...)
     * @param StreamNumber Stream number in Kind of Stream (first, second...)
     * @param parameter Parameter you are looking for in the Stream (Codec, width, bitrate...),
     *            in integer format (first parameter, second parameter...)
     * @param infoKind Kind of information you want about the parameter (the text, the measure,
     *            the help...)
     * @return a string about information you search, an empty string if there is a problem
     */
    public String Get(StreamKind streamKind, int streamNumber, int parameter, InfoKind infoKind)
    {
        return GetI(streamKind.ordinal(), streamNumber, parameter, infoKind.ordinal());
    }

    /**
     * Get a piece of information about a file (parameter is a string).
     *
     * @param StreamKind Kind of Stream (general, video, audio...)
     * @param StreamNumber Stream number in Kind of Stream (first, second...)
     * @param parameter Parameter you are looking for in the Stream (Codec, width, bitrate...),
     *            in string format ("Codec", "Width"...)
     * @param infoKind Kind of information you want about the parameter (the text, the measure,
     *            the help...)
     * @return a string about information you search, an empty string if there is a problem
     */
    public String Get(StreamKind streamKind, int streamNumber, String parameter, InfoKind infoKind)
    {
        return GetS(streamKind.ordinal(), streamNumber, parameter, infoKind.ordinal(), InfoKind.Name.ordinal());
    }

    /**
     * Get a piece of information about a file (parameter is a string).
     *
     * @param StreamKind Kind of Stream (general, video, audio...)
     * @param StreamNumber Stream number in Kind of Stream (first, second...)
     * @param parameter Parameter you are looking for in the Stream (Codec, width, bitrate...),
     *            in string format ("Codec", "Width"...)
     * @param infoKind Kind of information you want about the parameter (the text, the measure,
     *            the help...)
     * @param searchKind Where to look for the parameter
     * @return a string about information you search, an empty string if there is a problem
     */
    public String Get(StreamKind streamKind, int streamNumber, String parameter, InfoKind infoKind, InfoKind searchKind)
    {
        return GetS(streamKind.ordinal(), streamNumber, parameter, infoKind.ordinal(), searchKind.ordinal());
    }

    //Options
    /**
     * Configure or get information about MediaInfo.
     *
     * @param Option The name of option
     * @param Value The value of option
     * @return Depends on the option: by default "" (nothing) means No, other means Yes
     */
    public native String Option(String option, String value);

    /**
     * Configure or get information about MediaInfo.
     *
     * @param Option The name of option
     * @return Depends on the option: by default "" (nothing) means No, other means Yes
     */
    public String Option(String option)
    {
        return Option(option, "");
    }

    //Options
    /**
     * Configure or get information about MediaInfo. (static version)
     *
     * @param Option The name of option
     * @param Value The value of option
     * @return Depends on the option: by default "" (nothing) means No, other means Yes
     */
    public static String Option_Static(String option, String value)
    {
        return (new MediaInfo()).Option(option, value);
    }

    /**
     * Configure or get information about MediaInfo. (static version)
     *
     * @param Option The name of option
     * @return Depends on the option: by default "" (nothing) means No, other means Yes
     */
    public static String Option_Static(String option)
    {
        return Option_Static(option, "");
    }

    /**
     * Gets the state of the library
     * @return                                state of the library (between 0 and 10000)
    */
    public native int State_Get();

    private native int Count_Get(int streamKind, int streamNumber);

    /**
     * Count of Streams of a Stream kind (StreamNumber not filled), or count of piece of
     * information in this Stream.
     *

     * @param StreamKind Kind of Stream (general, video, audio...)
     * @return number of Streams of the given Stream kind
     */
    public int Count_Get(StreamKind streamKind)
    {
        return Count_Get(streamKind.ordinal(), -1);
    }

    /**
     * Count of Streams of a Stream kind (StreamNumber not filled), or count of piece of
     * information in this Stream.
     *
     * @param StreamKind Kind of Stream (general, video, audio...)
     * @param StreamNumber Stream number in this kind of Stream (first, second...)
     * @return number of Streams of the given Stream kind
     */
    public int Count_Get(StreamKind streamKind, int streamNumber)
    {
        return Count_Get(streamKind.ordinal(), streamNumber);
    }
}
