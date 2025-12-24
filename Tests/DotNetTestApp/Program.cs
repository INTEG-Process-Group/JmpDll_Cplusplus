using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace ConsoleApp1
{
    class Program
    {

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public class RegistryKey
        {
            public IntPtr _keyPtr { get; private set; }
            public IntPtr _valuePtr { get; private set; }



            public RegistryKey(string key)
            {
                _keyPtr = Marshal.StringToHGlobalAnsi(key);
                int sizeOf = Marshal.SizeOf(_keyPtr);
                _valuePtr = IntPtr.Zero;
            }



            public void Unpack(IntPtr ptr)
            {
                _keyPtr = Marshal.ReadIntPtr(ptr);
                string s = Marshal.PtrToStringAnsi(_keyPtr);
                _valuePtr = Marshal.ReadIntPtr(ptr, 4);
                string s2 = Marshal.PtrToStringAnsi(_valuePtr);
            }



            public string GetKey()
            {
                string s = Marshal.PtrToStringAnsi(_keyPtr);
                return s;
            }



            public string GetValue()
            {
                string s = Marshal.PtrToStringAnsi(_valuePtr);
                return s;
            }
        }



        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct Temperature
        {
            public double tempC;
            public double tempF;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct Environ
        {
            public double tempC;
            public double tempF;
            public double humidity;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct TenVolt
        {
            public ushort rawIn1;
            public ushort rawIn2;
            public ushort rawIn3;
            public ushort rawIn4;
            public ushort rawOut1;
            public ushort rawOut2;
        }



        const string DLL_NAME = "JmpDll.dll";

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int GetDllVersion(StringBuilder versionString);


        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int CreateConnection(string ipAddress, StringBuilder connectionUUID);


        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int GetInput(string connectionUUID, int inputNumber);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int GetOutput(string connectionUUID, int inputNumber);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double ControlOutput(string connectionUUID, string command, int channelNumber);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double CloseOutput(string connectionUUID, int channelNumber);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double OpenOutput(string connectionUUID, int channelNumber);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double ToggleOutput(string connectionUUID, int channelNumber);


        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern string ReadRegistryKeys(string connectionUUID, IntPtr registryKeys, int keyCount);


        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double EnumerateDevices(string connectionUUID, [In, Out] StringBuilder[] connectedDevices);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double GetTemperature(string connectionUUID, string deviceId, out Temperature temperature);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern double GetTemperatureByChannel(string connectionUUID, int channel, out Temperature temperature);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int GetEnviron(string connectionUUID, string deviceId, out Environ environ_struct);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int GetTenVolt(string connectionUUID, string deviceId, out TenVolt ten_volt_struct);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int SetTenVolt(string connectionUUID, string deviceId, ref TenVolt ten_volt_struct);

        [System.Runtime.InteropServices.DllImport(DLL_NAME, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        public static extern int SetTenVoltChannelPercentage(string connectionUUID, int channel, double percentage);




        static void Main(string[] args)
        {
            StringBuilder versionString = new StringBuilder();
            GetDllVersion(versionString);
            Console.WriteLine("version: " + versionString);

            StringBuilder connectionUUID = new StringBuilder();
            CreateConnection("10.0.0.62", connectionUUID);

            StringBuilder environConnectionUUID = new StringBuilder();
            CreateConnection("10.0.0.202", environConnectionUUID);

            StringBuilder tenVoltConnectionUUID = new StringBuilder();
            CreateConnection("10.0.0.96", tenVoltConnectionUUID);

            Thread.Sleep(1000);

            StringBuilder[] connectedDevices = new StringBuilder[10];
            for (int i = 0; i < 10; i++)
            {
                connectedDevices[i] = new StringBuilder();
            }
            EnumerateDevices(connectionUUID.ToString(),  connectedDevices);
            Console.WriteLine(connectedDevices);

            //for (int j = 7; j >= 0; j--)
            //{
            //    OpenOutput(connectionUUID.ToString(), j + 1);
            //    Thread.Sleep(100);
            //}

            //for (int j = 0; j < 8; j++)
            //{
            //    ToggleOutput(connectionUUID.ToString(), j + 1);
            //    Thread.Sleep(100);
            //    ToggleOutput(connectionUUID.ToString(), j + 1);
            //    Thread.Sleep(100);
            //}

            //for (int j = 7; j >= 0; j--)
            //{
            //    CloseOutput(connectionUUID.ToString(), j + 1);
            //    Thread.Sleep(100);
            //}



            // i want the module that has the first channel of temperature.  to 
            //  do this we ready the registry key for Type28_1.  this gives us 
            //  the module id in hex.  we will use that to issue the read request
            int tempNumber = 1;
            RegistryKey tempDeviceIdKey = new RegistryKey($"externals/deviceorder/Type28_{tempNumber}");
            RegistryKey versionKey = new RegistryKey("$version");
            RegistryKey[] regKeys = new RegistryKey[2];
            regKeys[0] = tempDeviceIdKey;
            regKeys[1] = versionKey;

            ReadRegistryKeys(connectionUUID.ToString(), regKeys);




            string moduleId = tempDeviceIdKey.GetValue();
            for (int i = 0; i < 4000000; i++)
            {
                var start = DateTime.Now;
                Temperature tempStruct;
                //GetTemperature(connectionUUID.ToString(), moduleId, out temp);
                GetTemperatureByChannel(connectionUUID.ToString(), 1, out tempStruct);
                var elapsed = DateTime.Now - start;
                Console.WriteLine(elapsed);
                Console.WriteLine("Temp C: " + tempStruct.tempC + ", TempF: " + tempStruct.tempF);
                File.AppendAllText("temps.dat", DateTime.Now + ", Temp C: " + tempStruct.tempC + ", TempF: " + tempStruct.tempF + "\n");

                Environ environ = new Environ();
                GetEnviron(environConnectionUUID.ToString(), "6B00100000350C7E", out environ);
                Console.WriteLine("Temp C: " + environ.tempC + ", TempF: " + environ.tempF + ", Humidity: " + environ.humidity);

                TenVolt tenVolt = new TenVolt();
                GetTenVolt(tenVoltConnectionUUID.ToString(), "C6111110510121FD", out tenVolt);
                Console.WriteLine($"Ten Volt In Raw 1: {tenVolt.rawIn1}");

                SetTenVoltChannelPercentage(tenVoltConnectionUUID.ToString(), 5, 50);

                //tenVolt.rawOut1 = (ushort)(i * 100);
                //tenVolt.rawOut2 = (ushort)(i * 100);
                //SetTenVolt(tenVoltConnectionUUID.ToString(), "C6111110510121FD", ref tenVolt);

                //ControlOutput(connectionUUID.ToString(), "Toggle", 1);

                Thread.Sleep(500);

                for (int j = 1; j <= 12; j++)
                {
                    int input = GetInput(connectionUUID.ToString(), j);
                    Console.WriteLine($"input {j}: {input}");
                }

                for (int j = 1; j <= 12; j++)
                {
                    int output = GetOutput(connectionUUID.ToString(), j);
                    Console.WriteLine($"output {j}: {output}");
                }
            }

            Console.WriteLine("done.");

            Console.ReadLine();

        }



        /**
         * Reads an array of registry keys from the JNIOR with the given connectionUUID.  
         * 
         * this is a helper function since there is additional work that needs 
         * to be done to handle memory allocation.
         */
        private static void ReadRegistryKeys(string connectionUUID, RegistryKey[] registryKeys)
        {
            // allocate memory for the array of registry key structs
            int structSize = Marshal.SizeOf(typeof(RegistryKey));
            int totalSize = structSize * registryKeys.Length;
            IntPtr arrayPtr = Marshal.AllocHGlobal(totalSize);

            // pack our registry key structs into our allocated memory
            IntPtr currentPtr = arrayPtr;
            for (int i = 0; i < 2; i++)
            {
                Marshal.StructureToPtr(registryKeys[i], currentPtr, true);
                currentPtr = new IntPtr(currentPtr.ToInt64() + structSize);
            }

            // issue the read command to the DLL
            ReadRegistryKeys(connectionUUID.ToString(), arrayPtr, registryKeys.Length);

            // retrieve modified data from memory by unpacking the registry key.  
            //  we dont use Marshal.PtrToStructure() method since we dont want 
            //  to replace the references within the array
            currentPtr = arrayPtr;
            for (int i = 0; i < registryKeys.Length; i++)
            {
                registryKeys[i].Unpack(currentPtr);
                Console.WriteLine($"Struct {i}: Key = {registryKeys[i].GetKey()}, Value = {registryKeys[i].GetValue()}");
                currentPtr = new IntPtr(currentPtr.ToInt64() + structSize);
            }

            // free the allocated memory
            Marshal.FreeHGlobal(arrayPtr);
        }
    }
}
