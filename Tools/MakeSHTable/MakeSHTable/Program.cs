using System;

namespace MakeSHTableApp
{
    internal class Program
    {
        static UInt16 CalcTemp(UInt16 value)
        {
            float tempCalc;
            float tempTemp;

            tempTemp = (1.78f / ((value + 2) / 33046.0f) - 3.57f);
            tempTemp *= 1000.0f;
            tempCalc = (float)(1.0f / (0.0007610373573f + (0.0002728524832 * Math.Log(tempTemp)) + (Math.Pow(Math.Log(tempTemp), 3) * 0.0000001022822735f)));
            tempCalc -= 273.15f;
            if (tempCalc < 0) tempCalc = 0;
            if (tempCalc > 99) tempCalc = 99;
            return (UInt16)(tempCalc * 10.0f);
        }

        static void Main(string[] args)
        {
            for (UInt16 value = 0; value < (1024 << 3); value += 8)
            {
                var temp = CalcTemp(value);
                Console.Write(temp.ToString() + ", ");
            }
        }
    }
}