using System;

namespace emutool
{
    public static class NumberUtils
    {
        public static ushort Reverse(ushort val)
        {
            var bytes = BitConverter.GetBytes(val);
            Array.Reverse(bytes);
            return BitConverter.ToUInt16(bytes, 0);
        }
    }
}
