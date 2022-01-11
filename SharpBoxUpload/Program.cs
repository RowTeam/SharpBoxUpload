using System;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;

namespace SharpBoxUpload
{
    class Program
    {
        private static DropboxHandler dbHandler;

        private static void UploadFile(string file, string currentTimeDay)
        {
            try
            {
                Console.WriteLine($"[*] Start uploading {file}.");
                FileInfo info = new FileInfo(file);
                string dbPath = $"/{currentTimeDay}/{info.Name}";
                Console.WriteLine($"  [+] {info.FullName} -> {dbPath} ({info.Length / 1024} kb)");

                if (dbHandler.PutFile(dbPath, File.ReadAllBytes(info.FullName)))
                    Console.WriteLine("  [+] Success...");
                else
                    Console.WriteLine("[!] Failure...");
            }
            catch (Exception ex)
            {
                Console.WriteLine("[!] UploadFile Error: " + ex.Message);
            }
        }

        private static void UploadFiles(string dirPath, string currentTimeDay)
        {
            try
            {
                string[] files = Directory.GetFiles(dirPath, "*.zip");
                if (files.Length == 0)
                {
                    Console.WriteLine($"[!] No *.zip file found in {dirPath} directory.");
                    return;
                }
                Console.WriteLine($"[*] Start uploading {files.Length} files.");
                foreach (string file in files)
                {
                    FileInfo info = new FileInfo(file);
                    string dbPath = $"/{currentTimeDay}/{info.Name}";
                    Console.WriteLine($"  [+] {info.FullName} -> {dbPath} ({info.Length / 1024} kb)");

                    if (dbHandler.PutFile(dbPath, File.ReadAllBytes(info.FullName)))
                        Console.WriteLine("  [+] Success...");
                    else
                        Console.WriteLine("[!] Failure...");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[!] UploadFiles Error: " + ex.Message);
            }
        }

        private static void Uages()
        {
            string assembly = System.Reflection.Assembly.GetExecutingAssembly().GetName().Name;
            Console.WriteLine($@"
Uages:
    {assembly} dbToken                          Upload files ending in .zip by default
    {assembly} dbToken dirPath                  Upload files ending in .zip by default
    {assembly} dbToken filePath                 Upload the specified file
");
        }

        /// <summary>
        /// 证书（忽略）
        /// </summary>
        private static bool ValidateRemoteCertificate(object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors error)
        {
            if (SslPolicyErrors.None == error)
            {
                return true;
            }
#if DEBUG
            Console.WriteLine("X509Certificate [{0}] Policy Error: '{1}'",
                cert.Subject,
                error.ToString());
#endif
            return false;
        }

        static void Main(string[] args)
        {
            ServicePointManager.ServerCertificateValidationCallback += ValidateRemoteCertificate;
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls;

            string currentTimeDay = DateTime.Now.ToString("yyyy-MM-dd");

            if (args.Length < 1)
            { Uages(); return; }

            dbHandler = new DropboxHandler(args[0]);

            string dirPath = Environment.CurrentDirectory;
            if (args.Length == 2)
                dirPath = args[1];

            if (Directory.Exists(dirPath))
                UploadFiles(dirPath, currentTimeDay);
            else
                UploadFile(dirPath, currentTimeDay);

            Console.WriteLine("[*] Finish...");
        }
    }
}
