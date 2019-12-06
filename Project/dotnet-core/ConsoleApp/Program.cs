/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Microsoft Visual C# example for .NET Core console apps
//
// To make this example work, you must put MediaInfo.Dll
// in the "./bin/__ConfigurationName__/netcoreapp3.1/" folder with the other binaries
// and add MediaInfoLib/Source/MediaInfoDLL/MediaInfoDLL.cs to your project
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using System;
using System.IO;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using MediaInfoLib;

using Azure;
using Azure.Storage.Blobs;
using Azure.Storage.Blobs.Specialized;

namespace ConsoleApp
{
    class Program
    {
        static async Task Main(string[] args)
        {
            String ToDisplay;
            MediaInfo MI = new MediaInfo();

            ToDisplay = MI.Option("Info_Version", "0.7.0.0;MediaInfoDLL_Example_CS;0.7.0.0");
            if (ToDisplay.Length == 0)
            {
                Console.WriteLine("MediaInfo.Dll: this version of the DLL is not compatible");
                return;
            }

            if (ToDisplay == "Unable to load MediaInfo library")
            {
                Console.WriteLine("MediaInfo.Dll: was not found");
                return;
            }

            //An example of how to use the library
            ToDisplay += "\r\nOpen\r\n";
            var filePath = "D:\\TestMedia\\Blender\\BigBuckBunny-peach\\bbb_sunflower_1080p_30fps_normal.mp4";
            MI.Open(filePath);

            ToDisplay += "\r\n\r\nInform with Complete=true\r\n";
            MI.Option("Complete", "1");
            ToDisplay += MI.Inform();

            Console.WriteLine(ToDisplay);

            Console.WriteLine(ExampleReadingWithFileStream());

            Console.WriteLine(await ExampleReadingFromAzureBlobStorage());
        }


        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //
        // This example demonstrates how to parse a local file using a FileStream
        // MediaInfo does not need to parse the whole file.
        // 
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        static String ExampleReadingWithFileStream()
        {
            // Set the following for your content:
            var filePath = "D:\\TestMedia\\Blender\\BigBuckBunny-peach\\bbb_sunflower_1080p_30fps_normal.mp4";
            FileStream fileStream = new FileStream(filePath, FileMode.Open, FileAccess.Read);
            long contentLength = fileStream.Length;

            // Initialize MediaInfo
            var MI = new MediaInfo();

            // Prepare MediaInfo for buffered reading
            MI.Open_Buffer_Init(contentLength, 0);

            // The parsing loop using a fixed memory buffer for reading and marshaling
            const int localByteBufferSize = 64 * 1024;
            byte[] localByteBuffer = new byte[localByteBufferSize];
            int numberOfBytesRead;
            long totalNumberOfBytesRead = 0;
            do
            {
                //Reading data somewhere, do what you want for this.
                numberOfBytesRead = fileStream.Read(localByteBuffer, 0, localByteBufferSize);
                totalNumberOfBytesRead += numberOfBytesRead;

                // Send the buffer to MediaInfo
                System.Runtime.InteropServices.GCHandle GC = System.Runtime.InteropServices.GCHandle.Alloc(localByteBuffer, System.Runtime.InteropServices.GCHandleType.Pinned);
                IntPtr AddrOf_Buffer_IntPtr = GC.AddrOfPinnedObject();
                Status Result = (Status)MI.Open_Buffer_Continue(AddrOf_Buffer_IntPtr, (IntPtr)numberOfBytesRead);
                GC.Free();

                if ((Result & Status.Finalized) == Status.Finalized)
                    break;
                
                // Test if MediaInfo requests to go elsewhere
                long desiredSeekLocation = MI.Open_Buffer_Continue_GoTo_Get();
                if (desiredSeekLocation != -1)
                {
                    Int64 Position = fileStream.Seek(desiredSeekLocation, SeekOrigin.Begin); //Position the file
                    contentLength = fileStream.Length;
                    MI.Open_Buffer_Init(contentLength, Position); //Informing MediaInfo we have seek
                }
            }
            while (numberOfBytesRead > 0);

            Console.WriteLine($"Total Bytes read: {totalNumberOfBytesRead}/{contentLength} = {totalNumberOfBytesRead/ (float)contentLength}");

            // This is the end of the stream, MediaInfo must finish some work
            MI.Open_Buffer_Finalize();

            // Use MediaInfoLib as needed
            MI.Option("Complete", "1");
            string ToDisplay = MI.Inform();

            return ToDisplay;
        }


        
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //
        // This example demonstrates how to parse a file in Azure Blob Storage using a connection string for auth.
        // MediaInfo does not need to parse the whole remote file.
        // 
        // Uses BlobBaseClient.DownloadAsync for byte-range downloads.
        // Ref: https://docs.microsoft.com/en-us/dotnet/api/azure.storage.blobs.specialized.blobbaseclient.downloadasync
        // Ref: https://github.com/Azure/azure-sdk-for-net/blob/master/sdk/storage/Azure.Storage.Blobs/tests/BlobBaseClientTests.cs#L243
        // 
        // Add the following nuget PackageReference your project file:
        //   Azure.Storage.Blobs (https://www.nuget.org/packages/Azure.Storage.Blobs/)
        // 
        // Add to your implementation class:
        //   using Azure.Storage.Blobs.Specialized;
        //
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        static async Task<string> ExampleReadingFromAzureBlobStorage()
        {
            // Set the following for your content:
            var fileUri = new Uri("https://mystorageaccount.blob.core.windows.net/blendercontainer/bbb_sunflower_1080p_30fps_normal.mp4");
            var connectionString = "DefaultEndpointsProtocol=https;AccountName=mystorageaccount;AccountKey=...";

            var blobUriBuilder = new BlobUriBuilder(fileUri); // The blobUriBuild simplifies the parsing of Azure Storage Uris
            BlobBaseClient blobBaseClient = new BlobBaseClient(connectionString, blobUriBuilder.BlobContainerName, blobUriBuilder.BlobName);

            // Initialize MediaInfo
            var MI = new MediaInfo();
            
            // Do a HEAD on the file:
            var blobProperties = await blobBaseClient.GetPropertiesAsync();
            long contentLength = blobProperties.Value.ContentLength;

            // Prepare MediaInfo for buffered reading
            MI.Open_Buffer_Init(contentLength, 0);

            // The parsing loop using a fixed memory buffer for reading and marshaling
            const int localByteBufferSize = 64 * 1024;
            byte[] localByteBuffer = new byte[localByteBufferSize];
            int numberOfBytesRead;
            long currentPosition = 0;
            long desiredOffset = 0;
            long totalNumberOfBytesRead = 0;
            do
            {
                // Read remote byte-range into localByteBuffer
                var downloadResponse = await blobBaseClient.DownloadAsync(new HttpRange(desiredOffset, localByteBufferSize));
                numberOfBytesRead = (int)downloadResponse.Value.ContentLength;
                using (var ms = new MemoryStream(localByteBufferSize))
                {
                    downloadResponse.Value.Content.CopyTo(ms);
                    localByteBuffer = ms.GetBuffer();
                    currentPosition = downloadResponse.Value.Content.Position;
                }
                totalNumberOfBytesRead += numberOfBytesRead;

                // Send the buffer to MediaInfo
                GCHandle GC = GCHandle.Alloc(localByteBuffer, GCHandleType.Pinned);
                IntPtr AddrOf_Buffer_IntPtr = GC.AddrOfPinnedObject();
                Status Result = (Status)MI.Open_Buffer_Continue(AddrOf_Buffer_IntPtr, (IntPtr)numberOfBytesRead);
                GC.Free();

                if ((Result & Status.Finalized) == Status.Finalized)
                    break;

                // Test if MediaInfo requests to go elsewhere
                desiredOffset = MI.Open_Buffer_Continue_GoTo_Get();
                if (desiredOffset != -1)
                {
                    // Inform MediaInfo we have seek
                    MI.Open_Buffer_Init(contentLength, desiredOffset); 
                }
                else
                {
                    // Adjust the byte-range request offset
                    desiredOffset = currentPosition;
                }
            }
            while (numberOfBytesRead > 0  && desiredOffset != contentLength);

            Console.WriteLine($"Total Bytes read: {totalNumberOfBytesRead}/{contentLength} = {totalNumberOfBytesRead / (float)contentLength}");

            // This is the end of the stream, MediaInfo must finish some work
            MI.Open_Buffer_Finalize(); 

            // Use MediaInfoLib as needed
            MI.Option("Complete", "1");
            string ToDisplay = MI.Inform();

            return ToDisplay;
        }
    }
}
