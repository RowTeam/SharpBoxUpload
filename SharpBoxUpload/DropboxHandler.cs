using System;
using System.Net;
using System.Text;
using System.Collections;
using System.Collections.Generic;

namespace SharpBoxUpload
{
    class DropboxHandler
    {
        WebClient webClient;

        // Dropbox API URL 入口点列表
        static Hashtable dropboxAPI = new Hashtable() {
            {"uploadFile", "https://content.dropboxapi.com/2/files/upload" }
        };

        public bool PutFile(string path, byte[] data)
        {
            string command = $@"{{""path"": ""{path}"",""mode"": ""overwrite"",""autorename"": false,""mute"": true}}";

            webClient.Headers["Content-Type"] = "application/octet-stream";
            webClient.Headers["Dropbox-API-Arg"] = command;

            try
            {
                byte[] responseArray = webClient.UploadData((string)dropboxAPI["uploadFile"], data);
                return true;
            }
            catch (Exception ex)
            {
                while (ex != null)
                {
                    Console.WriteLine("[!] ERROR: " + ex.Message);
                    ex = ex.InnerException;
                }
                return false;
            }
        }

        public DropboxHandler(string accessToken)
        {
            webClient = new WebClient();

            IWebProxy defaultProxy = WebRequest.DefaultWebProxy;
            if (defaultProxy != null)
            {
                defaultProxy.Credentials = CredentialCache.DefaultCredentials;
                webClient.Proxy = defaultProxy;
            }
            webClient.Headers.Add("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:49.0) Gecko/20100101 Firefox/49.0");
            webClient.Headers.Add("Authorization", "Bearer " + accessToken);
        }
    }
}
