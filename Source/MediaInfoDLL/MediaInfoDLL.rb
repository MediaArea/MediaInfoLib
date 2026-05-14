require 'rbconfig'
require 'ffi'

module MediaInfo extend(FFI::Library)
    Stream = enum(:General,
                    :Video,
                    :Audio,
                    :Text,
                    :Other,
                    :Image,
                    :Menu,
                    :Max)

    Info = enum(:Name,
                :Text,
                :Measure,
                :Options,
                :Name_Text,
                :Measure_Text,
                :Info,
                :HowTo,
                :Max)

    InfoOptions = enum(:ShowInInform,
                       :Reserved,
                       :ShowInSupported,
                       :TypeOfValue,
                       :Max)

    FileOptions = enum(:Nothing,    0x00,
                       :NoRecursive, 0x01,
                       :CloseAll,    0x02,
                       :Max,         0x04)

    class MediaInfo extend(FFI::Library)
        if ENV['LIBMEDIAINFO_PATH'] != nil
            ffi_lib(ENV['LIBMEDIAINFO_PATH'])
        else
            if  RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/
                ffi_lib('MediaInfo.dll')
            else # assume unix
                ffi_lib('mediainfo')
            end
        end

        attach_function(:MediaInfo_New, [], :pointer)
        attach_function(:MediaInfo_Delete, [:pointer], :void)
        attach_function(:MediaInfo_Open, [:pointer, :pointer], :size_t)
        attach_function(:MediaInfo_Open_Buffer_Init, [:pointer, :uint64, :uint64], :size_t)
        attach_function(:MediaInfo_Open_Buffer_Continue, [:pointer, :buffer_in, :size_t], :size_t)
        attach_function(:MediaInfo_Open_Buffer_Continue_GoTo_Get, [:pointer], :uint64)
        attach_function(:MediaInfo_Open_Buffer_Finalize, [:pointer], :size_t)
        attach_function(:MediaInfo_Open_NextPacket, [:pointer], :size_t)
        attach_function(:MediaInfo_Close, [:pointer], :size_t)
        attach_function(:MediaInfo_Inform, [:pointer], :pointer)
        attach_function(:MediaInfo_Get, [:pointer, Stream, :size_t, :pointer, Info, Info], :pointer)
        attach_function(:MediaInfo_Option, [:pointer, :pointer, :pointer], :pointer)
        attach_function(:MediaInfo_State_Get, [:pointer], :size_t)
        attach_function(:MediaInfo_Count_Get, [:pointer, Stream, :size_t], :size_t)

        def initialize()
            # try to guess wchar_t size and endianness
            if  RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/
                @wchar_bytesize = 2
                @wchar_type = :uint16
                @wchar_pack = 'S*'
                @wchar_encoding = 'utf-16'
            else # assume unix
                @wchar_bytesize = 4
                @wchar_type = :uint32
                @wchar_pack = 'L*'
                @wchar_encoding = 'utf-32'
            end

            if [1].pack("I") == [1].pack("N") # big endian
                @wchar_encoding += 'be'
            else
                @wchar_encoding += 'le'
            end

            @ptr = New()
            ObjectSpace.define_finalizer( self, self.class.finalize(@ptr))
        end

        def self.finalize(ptr)
            proc {
                MediaInfo_Delete(ptr)
            }
        end

        def New()
            if @ptr != FFI::Pointer::NULL
                Delete()
            end
            @ptr = MediaInfo_New()
        end

        def Delete()
            MediaInfo_Delete(@ptr)
            @ptr = FFI::Pointer::NULL
        end

        def Open(file)
            FFI::MemoryPointer.new(@wchar_type, file.length + 1) do |p|
                p.write_bytes(file.encode(@wchar_encoding))
                return MediaInfo_Open(@ptr, p)
            end
        end

        def Open_Buffer_Init(size, offset)
            MediaInfo_Open_Buffer_Init(@ptr, size, offset)
        end

        def Open_Buffer_Continue(buffer, size)
            FFI::Buffer.new(:uint8, size) do |b|
                b.write_bytes(buffer, 0, size)
                return MediaInfo_Open_Buffer_Continue(@ptr, b, size)
            end
        end

        def Open_Buffer_Continue_GoTo_Get()
            MediaInfo_Open_Buffer_Continue_GoTo_Get(@ptr)
        end

        def Open_Buffer_Finalize()
            MediaInfo_Open_Buffer_Finalize(@ptr)
        end

        def Open_NextPacket()
            MediaInfo_Open_NextPacket(@ptr)
        end

        def Close()
            MediaInfo_Close(@ptr)
        end

        def Inform()
            CWideStringPointerToString(MediaInfo_Inform(@ptr))
        end

        def Get(streamKind, streamNumber, parameter, infoKind = :Text, searchKind = :Name)
            if parameter.is_a? Numeric
                return CWideStringPointerToString(MediaInfo_GetI(@ptr, streamKind, streamNumber, parameter, infoKind))
            else
                FFI::MemoryPointer.new(@wchar_type, parameter.length + 1) do |p|
                    p.write_bytes(parameter.encode(@wchar_encoding))
                    return CWideStringPointerToString(MediaInfo_Get(@ptr, streamKind, streamNumber, p, infoKind, searchKind))
                end
            end
        end

        def Option(parameter, value = "")
            FFI::MemoryPointer.new(@wchar_type, parameter.length + 1) do |pp|
                pp.write_bytes(parameter.encode(@wchar_encoding))
                FFI::MemoryPointer.new(@wchar_type, value.length + 1) do |pv|
                    pv.write_bytes(value.encode(@wchar_encoding))
                    return CWideStringPointerToString(MediaInfo_Option(@ptr, pp, pv))
                end
            end
        end

        def State_Get()
            MediaInfo_State_Get(@ptr)
        end

        def Count_Get(streamKind, streamNumber = -1)
            MediaInfo_Count_Get(@ptr, streamKind, streamNumber)
        end

        private :MediaInfo_New
        private :MediaInfo_Delete
        private :MediaInfo_Open
        private :MediaInfo_Open_Buffer_Init
        private :MediaInfo_Open_Buffer_Continue
        private :MediaInfo_Open_Buffer_Continue_GoTo_Get
        private :MediaInfo_Open_Buffer_Finalize
        private :MediaInfo_Open_NextPacket
        private :MediaInfo_Close
        private :MediaInfo_Inform
        private :MediaInfo_Get
        private :MediaInfo_Option
        private :MediaInfo_State_Get
        private :MediaInfo_Count_Get
        private

        def CWideStringPointerToString(ptr)
            offset = 0
            array = []
            until (char = ptr.get(@wchar_type, offset)) == 0
                array.append(char)
                offset += @wchar_bytesize
            end

            codec = Encoding::Converter.new(@wchar_encoding, "utf-8", :undef => :replace)
            codec.convert(array.pack(@wchar_pack))
        end
    end

    class MediaInfoList extend(FFI::Library)
        if ENV['LIBMEDIAINFO_PATH'] != nil
            ffi_lib(ENV['LIBMEDIAINFO_PATH'])
        else
            if  RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/
                ffi_lib('MediaInfo.dll')
            else # assume unix
                ffi_lib('mediainfo')
            end
        end

        attach_function(:MediaInfoList_New, [], :pointer)
        attach_function(:MediaInfoList_Delete, [:pointer], :void)
        attach_function(:MediaInfoList_Open, [:pointer, :pointer, FileOptions], :size_t)
        attach_function(:MediaInfoList_Close, [:pointer, :size_t], :size_t)
        attach_function(:MediaInfoList_Inform, [:pointer, :size_t], :pointer)
        attach_function(:MediaInfoList_Get, [:pointer, :size_t, Stream, :size_t, :pointer, Info, Info], :pointer)
        attach_function(:MediaInfoList_Option, [:pointer, :pointer, :pointer], :pointer)
        attach_function(:MediaInfoList_State_Get, [:pointer], :size_t)
        attach_function(:MediaInfoList_Count_Get, [:pointer, :size_t, Stream, :size_t], :size_t)

        def initialize()
            # try to guess wchar_t size and endianness
            if  RbConfig::CONFIG['host_os'] =~ /mswin|mingw|cygwin/
                @wchar_bytesize = 2
                @wchar_type = :uint16
                @wchar_pack = 'S*'
                @wchar_encoding = 'utf-16'
            else # assume unix
                @wchar_bytesize = 4
                @wchar_type = :uint32
                @wchar_pack = 'L*'
                @wchar_encoding = 'utf-32'
            end

            if [1].pack("I") == [1].pack("N") # big endian
                @wchar_encoding += 'be'
            else
                @wchar_encoding += 'le'
            end

            @ptr = New()
            ObjectSpace.define_finalizer( self, self.class.finalize(@ptr))
        end

        def self.finalize(ptr)
            proc {
                MediaInfoList_Delete(ptr)
            }
        end

        def New()
            if @ptr != FFI::Pointer::NULL
                Delete()
            end
            @ptr = MediaInfoList_New()
        end

        def Delete()
            MediaInfoList_Delete(@ptr)
            @ptr = FFI::Pointer::NULL
        end

        def Open(file, fileOptions = :Nothing)
            FFI::MemoryPointer.new(@wchar_type, file.length + 1) do |p|
                p.write_bytes(file.encode(@wchar_encoding))
                return MediaInfoList_Open(@ptr, p, fileOptions)
            end
        end

        def Close(filePos = -1)
            MediaInfoList_Close(@ptr, filePos)
        end

        def Inform(filePos = -1)
            CWideStringPointerToString(MediaInfoList_Inform(@ptr, filePos))
        end

        def Get(filePos, streamKind, streamNumber, parameter, infoKind = :Text, searchKind = :Name)
            if parameter.is_a? Numeric
                return CWideStringPointerToString(MediaInfoList_GetI(@ptr, filePos, streamKind, streamNumber, parameter, infoKind))
            else
                FFI::MemoryPointer.new(@wchar_type, parameter.length + 1) do |p|
                    p.write_bytes(parameter.encode(@wchar_encoding))
                    return CWideStringPointerToString(MediaInfoList_Get(@ptr, filePos, streamKind, streamNumber, p, infoKind, searchKind))
                end
            end
        end

        def Option(parameter, value = "")
            FFI::MemoryPointer.new(@wchar_type, parameter.length + 1) do |pp|
                pp.write_bytes(parameter.encode(@wchar_encoding))
                FFI::MemoryPointer.new(@wchar_type, value.length + 1) do |pv|
                    pv.write_bytes(value.encode(@wchar_encoding))
                    return CWideStringPointerToString(MediaInfoList_Option(@ptr, pp, pv))
                end
            end
        end

        def State_Get()
            MediaInfoList_State_Get(@ptr)
        end

        def Count_Get(filePos, streamKind, streamNumber = -1)
            MediaInfoList_Count_Get(@ptr, filePos, streamKind, streamNumber)
        end

        private :MediaInfoList_New
        private :MediaInfoList_Delete
        private :MediaInfoList_Open
        private :MediaInfoList_Close
        private :MediaInfoList_Inform
        private :MediaInfoList_Get
        private :MediaInfoList_Option
        private :MediaInfoList_State_Get
        private :MediaInfoList_Count_Get
        private

        def CWideStringPointerToString(ptr)
            offset = 0
            array = []
            until (char = ptr.get(@wchar_type, offset)) == 0
                array.append(char)
                offset += @wchar_bytesize
            end

            codec = Encoding::Converter.new(@wchar_encoding, "utf-8", :undef => :replace)
            codec.convert(array.pack(@wchar_pack))
        end
    end
end
