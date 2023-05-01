/*
* This file is part of FshThumbnailHandler, a Windows thumbnail handler for FSH images.
*
* Copyright (c) 2009, 2010, 2012, 2013, 2023 Nicholas Hayes
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace launchmsi
{
    class Program
    {
        internal enum Platform
        {
            X86,
            X64,
            Unknown
        }

        private const ushort PROCESSOR_ARCHITECTURE_INTEL = 0;
        private const ushort PROCESSOR_ARCHITECTURE_IA64 = 6;
        private const ushort PROCESSOR_ARCHITECTURE_AMD64 = 9;

        [StructLayout(LayoutKind.Sequential)]
        internal struct SYSTEM_INFO
        {
            public ushort wProcessorArchitecture;
            public ushort wReserved;
            public uint dwPageSize;
            public IntPtr lpMinimumApplicationAddress;
            public IntPtr lpMaximumApplicationAddress;
            public UIntPtr dwActiveProcessorMask;
            public uint dwNumberOfProcessors;
            public uint dwProcessorType;
            public uint dwAllocationGranularity;
            public ushort wProcessorLevel;
            public ushort wProcessorRevision;
        };

        [DllImport("kernel32.dll")]
        internal static extern void GetNativeSystemInfo(ref SYSTEM_INFO lpSystemInfo);

        private static Platform GetPlatform()
        {
            SYSTEM_INFO sysInfo = new SYSTEM_INFO();

            GetNativeSystemInfo(ref sysInfo);

            switch (sysInfo.wProcessorArchitecture)
            {
                case PROCESSOR_ARCHITECTURE_IA64:
                case PROCESSOR_ARCHITECTURE_AMD64:
                    return Platform.X64;

                case PROCESSOR_ARCHITECTURE_INTEL:
                    return Platform.X86;

                default:
                    return Platform.Unknown;
            }
        }

        private static void RunInstall(string msi)
        {
            ProcessStartInfo si = new ProcessStartInfo(msi);
            Process p = Process.Start(si);
            p.WaitForExit();
        }

        static void Main(string[] args)
        {
            try
            {
                string dir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                string installer64 = Path.Combine(dir, "FshThumbnailhandler-x64.msi");
                string xpinstall64 = Path.Combine(dir, "FshthumbSetup-x64.msi"); // Xp x64 installer
                string installer86 = Path.Combine(dir, "FshThumbnailhandler.msi");
                string xpinstall86 = Path.Combine(dir, "FshthumbSetup.msi"); //Xp x86 installer

                Platform arch = GetPlatform();
                if (arch == Platform.X64)
                {
                    if (Environment.OSVersion.Version.Major >= 6)
                    {
                        RunInstall(installer64);
                    }
                    else
                    {
                        RunInstall(xpinstall64);
                    }
                }
                else if (arch == Platform.X86)
                {
                    if (Environment.OSVersion.Version.Major >= 6)
                    {
                        RunInstall(installer86);
                    }
                    else
                    {
                        RunInstall(xpinstall86);
                    }
                }
            }
            catch (Exception ex)
            {
                System.Windows.Forms.MessageBox.Show(ex.Message, "Error launching msi installer");
            }

        }

    }
}
