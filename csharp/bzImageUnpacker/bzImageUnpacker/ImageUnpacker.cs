using System;
using System.IO;

namespace bzImageUnpacker
{
    class ImageUnpacker
    {
        readonly byte[] GzipMagic = new byte[] { 0x1F, 0x8B, 0x08 };

        static int StreamToStream(Stream inStream, Stream outStream)
        {
            const int BufferSize = 1024 * 1024;
            byte[] buffer = new byte[BufferSize];
            int bytesRead = 0, totalCount= 0;

            while ((bytesRead = inStream.Read(buffer, 0, BufferSize)) > 0)
            {
                outStream.Write(buffer, 0, bytesRead);
                totalCount += bytesRead;
            }
            return totalCount;
        }

        bool BytesEqual(byte[] left, byte[] right)
        {
            if (left.Length != right.Length)
                return false;

            for (int i = 0; i < left.Length; i++)
                if (left[i] != right[i])
                    return false;

            return true;
        }
        
        bool FindSignature(Stream stream)
        {
            byte[] buf = new byte[GzipMagic.Length];

            // last 1-2 bytes
            if ((stream.Length - stream.Position - 1) < 3)
                return false;

            stream.Read(buf, 0, 3);

            if (BytesEqual(buf, GzipMagic))
            {
                stream.Position -= 3;
                return true;
            }

            int readByte;
            while ((readByte = stream.ReadByte()) != -1)
            {
                buf[0] = buf[1];
                buf[1] = buf[2];
                buf[2] = (byte)readByte;

                if (BytesEqual(buf, GzipMagic))
                {
                    stream.Position -= 3;
                    return true;
                }
            }

            return false;
        }

        public bool Unpack(string filePath)
        {
            using (var inFileStream = File.OpenRead(filePath))
            {
                // try while not EOF
                while (inFileStream.Position < inFileStream.Length - 1)
                {
                    if (FindSignature(inFileStream))
                        Console.WriteLine("Found gzip magic at 0x{0:X8}", inFileStream.Position);

                    FileStream outFileStream = null;
                    try
                    {
                        outFileStream = File.OpenWrite(filePath + "_unpacked");
                        var gzipStream = new System.IO.Compression.GZipStream(
                            inFileStream, System.IO.Compression.CompressionMode.Decompress);
                        if (StreamToStream(gzipStream, outFileStream) != 0)
                        {
                            Console.WriteLine("Success: unpacked image was saved in file " + outFileStream.Name);
                            return true;
                        }
                    }
                    catch (Exception exc)
                    {
                        // InvalidDataException means wrong data format (not gzip),
                        // its ok, lets search more
                        // everything else is not ok, so rethrow exception
                        if (exc is InvalidDataException)
                        {
                            Console.WriteLine("Uncompressing failure - not gzip format");
                            continue;
                        }

                        throw;
                    }
                    finally
                    {
                        if (outFileStream != null)
                            outFileStream.Close();
                    }
                }

                return false;
            }
        }
    }
}
