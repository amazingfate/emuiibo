using System;
using System.IO;
using Newtonsoft.Json.Linq;

namespace emutool
{
    public static class AmiiboUtils
    {
        public class Date
        {
            public ushort Year { get; set; }

            public byte Month { get; set; }

            public byte Day { get; set; }

            public JObject ToJSON()
            {
                return new JObject()
                {
                    ["y"] = Year,
                    ["m"] = Month,
                    ["d"] = Day,
                };
            }
        }

        public class CharacterId
        {
            public ushort GameCharacterId { get; set; }

            public byte CharacterVariant { get; set; }
        }

        public class AmiiboId
        {
            public CharacterId CharacterId { get; set; }

            public byte Series { get; set; }

            public ushort ModelNumber { get; set; }

            public byte FigureType { get; set; }

            public AmiiboId()
            {
                CharacterId = new CharacterId();
            }

            public JObject ToJSON()
            {
                return new JObject()
                {
                    ["game_character_id"] = CharacterId.GameCharacterId,
                    ["character_variant"] = CharacterId.CharacterVariant,
                    ["figure_type"] = FigureType,
                    ["series"] = Series,
                    ["model_number"] = ModelNumber,
                };
            }
        }

        public class Amiibo
        {
            public const int UuidLength = 10;

            public string Name { get; set; }

            public Date FirstWriteDate { get; set; }

            public byte[] Uuid { get; set; }

            public AmiiboId Id { get; set; }

            public Date LastWriteDate { get; set; }

            public ushort WriteCounter { get; set; }

            public uint Version { get; set; }

            public string MiiCharInfoFileName { get; set; }

            public AmiiboAPI.Amiibo OriginalAmiibo { get; set; }

            public Amiibo()
            {
                FirstWriteDate = new Date();
                Uuid = new byte[UuidLength];
                Id = new AmiiboId();
                LastWriteDate = new Date();
                WriteCounter = 0;
                Version = 0;
            }

            public void Save(string dir, bool random_uuid, bool save_image)
            {
                try
                {
                    FsUtils.RecreateDirectory(dir);
                    var json = new JObject()
                    {
                        ["name"] = Name,
                        ["write_counter"] = WriteCounter,
                        ["version"] = Version,
                        ["mii_charinfo_file"] = MiiCharInfoFileName,
                        ["first_write_date"] = FirstWriteDate.ToJSON(),
                        ["last_write_date"] = LastWriteDate.ToJSON(),
                        ["id"] = Id.ToJSON(),
                    };
                    if(!random_uuid)
                    {
                        var uuid_array = new JArray();
                        foreach(var uuid_byte in Uuid)
                        {
                            uuid_array.Add(uuid_byte);
                        }
                        json["uuid"] = uuid_array;
                    }
                    FsUtils.SaveJSON(Path.Combine(dir, "amiibo.json"), json);
                    FsUtils.CreateEmptyFile(Path.Combine(dir, "amiibo.flag"));
                    if(save_image)
                    {
                        FsUtils.SaveFromURL(OriginalAmiibo.ImageURL, Path.Combine(dir, "amiibo.png"));
                    }
                }
                catch(Exception ex)
                {
                    ExceptionUtils.LogExceptionMessage(ex);
                }
            }

        }

        public static Amiibo BuildAmiibo(AmiiboAPI.Amiibo api_amiibo, string name)
        {
            ExceptionUtils.Unless(api_amiibo != null, "Invalid input amiibo");

            var amiibo = new Amiibo();
            amiibo.OriginalAmiibo = api_amiibo;
            
            try
            {
                amiibo.Name = name;
                amiibo.MiiCharInfoFileName = "mii-charinfo.bin";

                var cur_date = DateTime.Now;
                amiibo.FirstWriteDate.Year = (ushort)cur_date.Year;
                amiibo.FirstWriteDate.Month = (byte)cur_date.Month;
                amiibo.FirstWriteDate.Day = (byte)cur_date.Day;
                amiibo.LastWriteDate = amiibo.FirstWriteDate;

                var id = api_amiibo.AmiiboId;
                ExceptionUtils.Unless(id.Length == 16, "Invalid amiibo ID");

                var character_game_id_str = id.Substring(0, 4);
                var character_variant_str = id.Substring(4, 2);
                var figure_type_str = id.Substring(6, 2);
                var model_no_str = id.Substring(8, 4);
                var series_str = id.Substring(12, 2);

                // Swap endianness for this number
                var character_game_id_be = ushort.Parse(character_game_id_str, System.Globalization.NumberStyles.HexNumber);
                amiibo.Id.CharacterId.GameCharacterId = NumberUtils.Reverse(character_game_id_be);

                amiibo.Id.CharacterId.CharacterVariant = byte.Parse(character_variant_str, System.Globalization.NumberStyles.HexNumber);
                amiibo.Id.FigureType = byte.Parse(figure_type_str, System.Globalization.NumberStyles.HexNumber);
                amiibo.Id.ModelNumber = ushort.Parse(model_no_str, System.Globalization.NumberStyles.HexNumber);
                amiibo.Id.Series = byte.Parse(series_str, System.Globalization.NumberStyles.HexNumber);

                // Generate a random UUID
                Random rnd = new Random();
                rnd.NextBytes(amiibo.Uuid);
            }
            catch(Exception ex)
            {
                ExceptionUtils.LogExceptionMessage(ex);
            }

            return amiibo;
        }
    }
}
