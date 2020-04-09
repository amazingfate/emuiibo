using System.Net;
using System.IO;
using Newtonsoft.Json.Linq;

namespace emutool
{
    public static class FsUtils
    {
        public static void RecreateDirectory(string dir)
        {
            if(Directory.Exists(dir))
            {
                Directory.Delete(dir, true);
            }
            Directory.CreateDirectory(dir);
        }
        public static void CreateEmptyFile(string path)
        {
            File.Create(path).Dispose();
        }

        public static void SaveJSON(string path, JObject json)
        {
            File.WriteAllText(path, json.ToString());
        }

        public static void SaveFromURL(string url, string path)
        {
            using(var web = new WebClient())
            {
                web.DownloadFile(url, path);
            }
        }
    }
}
