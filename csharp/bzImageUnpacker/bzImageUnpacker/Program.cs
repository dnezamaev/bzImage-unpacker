using System;

namespace bzImageUnpacker
{
    class Program
    {
        static void PrintHelp(string[] args)
        {
            Console.Write("Unpacks bzImage file.\n" +
                "Usage:\n" +
                System.IO.Path.GetFileName(System.Reflection.Assembly.GetExecutingAssembly().Location)
                + " <bzImage_path>\n");
        }

        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                PrintHelp(args);
                return;
            }

            var unpacker = new ImageUnpacker();

            try
            {
                if (!unpacker.Unpack(args[0]))
                    Console.WriteLine("Error: packed image was not found in file " + args[0]);
            }
            catch (Exception exc)
            {
                Console.WriteLine("ERROR: " + exc.Message);
            }
        }
    }
}
