#ifndef OSD_HPP
#define OSD_HPP

#include <virtdisk.h>

#include <string>
#include <iostream>

namespace OSL
{
  HANDLE OpenVirtualDisk(const std::wstring& file)
  {
    DWORD fRet;
    HANDLE hvhd = INVALID_HANDLE_VALUE;
    OPEN_VIRTUAL_DISK_PARAMETERS params = { 0 };
    params.Version = OPEN_VIRTUAL_DISK_VERSION_3;

    VIRTUAL_STORAGE_TYPE vst = 
    {
      VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN,
      { 0xEC984AEC, 0xA0F9, 0x47e9, { 0x90, 0x1F, 0x71, 0x41, 0x5A, 0x66, 0x34, 0x5B } }
    };

    fRet = OpenVirtualDisk(
             &vst,
             file.c_str(),
             (VIRTUAL_DISK_ACCESS_MASK) 0,
             OPEN_VIRTUAL_DISK_FLAG_NONE,
             &params,
             &hvhd);

    if (fRet != ERROR_SUCCESS)
    {
      std::cout << "OpenVirtualDisk failed " << fRet << "\n";
    }

    return hvhd;
  }

  HANDLE AttachVHD(const std::string& source)
  {
    HANDLE hvhd = OpenVirtualDisk(
      std::wstring(source.begin(), source.end())
    );

    ATTACH_VIRTUAL_DISK_PARAMETERS params = { 0 };
    params.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    auto fRet = AttachVirtualDisk(
                  hvhd,
                  NULL,
                  ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY|ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER,
                  0,
                  &params,
                  NULL);

    if (fRet != ERROR_SUCCESS)
    {
      std::cout << "AttachVirtualDisk failed, error : " << fRet << "\n";
      return INVALID_HANDLE_VALUE;
    }

    return hvhd;
  }

  auto GetPhysicalDiskPath(HANDLE hvhd)
  {
    wchar_t buf[128] = { L'\0' };
    ULONG bufSize = sizeof(buf);

    auto fRet = GetVirtualDiskPhysicalPath(hvhd, &bufSize, buf);

    if (fRet != ERROR_SUCCESS)
    {
      std::cout << "GetVirtualDiskPhysicalPath failed, error : " << fRet << "\n";
      return std::string();
    }

    char phyDiskPath[128] = { '\0' };
    wchar_t *wc = buf;

    while (*wc)
    {
      phyDiskPath[strlen(phyDiskPath)] = *((char *)wc); 
      wc++;
    }

    return std::string(phyDiskPath);
  }

  PVOLUME_BITMAP_BUFFER GetVolumeInUseBitmap(HANDLE hvolume)
  {
    STARTING_LCN_INPUT_BUFFER startLCN;
    PVOLUME_BITMAP_BUFFER bitmap = NULL;
    DWORD dwAllocatedSize = 64 * 1024;
    DWORD BytesReturned = 0;

    startLCN.StartingLcn.QuadPart = 0;

    for (;;)
    {
      bitmap = (VOLUME_BITMAP_BUFFER *) LocalAlloc(LMEM_FIXED, dwAllocatedSize);

      BOOL fRet = DeviceIoControl(
                    hvolume, 
                    FSCTL_GET_VOLUME_BITMAP, 
                    &startLCN,
                    sizeof(startLCN),
                    bitmap, 
                    dwAllocatedSize, 
                    &BytesReturned, 
                    NULL);

      if (fRet)
      {
        // DumpData("bitmap", bitmap->Buffer, (bitmap->BitmapSize.QuadPart)/8 + ( ((bitmap->BitmapSize.QuadPart)%8) ? 1 : 0 ));
        return bitmap;
      }
      else if (GetLastError() != ERROR_MORE_DATA)
      {
        std::cout << "FSCTL_GET_VOLUME_BITMAP failed, error : " << GetLastError() << "\n";
        return NULL;
      }

      LocalFree(bitmap);
      dwAllocatedSize *= 2;
    }
  }

  NTFS_VOLUME_DATA_BUFFER GetNTFSVolumeData(HANDLE hDevice)
  {
    DWORD fRet, BytesReturned;
    NTFS_VOLUME_DATA_BUFFER nvdb = {0};

    fRet = DeviceIoControl(
             hDevice,
             FSCTL_ALLOW_EXTENDED_DASD_IO,
             nullptr,
             0,
             nullptr,
             0,
             &BytesReturned,
             nullptr);

    if (!fRet)
    {
      std::cout << "FSCTL_ALLOW_EXTENDED_DASD_IO failed, error : " << GetLastError() << "\n";
      return {};
    }

    fRet = DeviceIoControl(
             hDevice,
             FSCTL_GET_NTFS_VOLUME_DATA,
             nullptr,
             0,
             &nvdb,
             sizeof (NTFS_VOLUME_DATA_BUFFER),
             &BytesReturned,
             nullptr);

    if (!fRet) 
    {
      std::cout << "FSCTL_GET_NTFS_VOLUME_DATA failed, error : " << GetLastError() << "\n";
      return {};
    }

    return nvdb;
  }

  LONGLONG GetPartitionLength(HANDLE hVolume)
  {
    DWORD nReturned;
    GET_LENGTH_INFORMATION  gli = { 0 };

    if (!DeviceIoControl(
           hVolume,
           IOCTL_DISK_GET_LENGTH_INFO,
           NULL,
           0,
           &gli,
           sizeof (GET_LENGTH_INFORMATION),
           &nReturned,
           NULL)) 
    {
      std::cout << "IOCTL_DISK_GET_LENGTH_INFO failed, error : " << GetLastError() << "\n";
      return 0;
    }

    return gli.Length.QuadPart;
  }

  auto EnumerateVolumes(void)
  {
    char _guid[128];
    BOOL fRet = TRUE;
    char _buf[MAX_PATH];
    DWORD nReturned = 0;

    std::vector<std::vector<std::string>> out;

    HANDLE hFind = FindFirstVolumeA(_guid, sizeof(_guid) / sizeof(char));

    if (hFind == INVALID_HANDLE_VALUE)
    {
      std::cout << "FindFirstVolumeW failed, error " << GetLastError() << "\n";
      return out;
    }

    do
    {
      std::vector<std::string> names = { _guid };

      fRet = GetVolumePathNamesForVolumeNameA(
        _guid,
        _buf,
        MAX_PATH,
        &nReturned);

      if (!fRet)
      {
        std::cout << "GetVolumePathNamesForVolumeNameA failed, error " << GetLastError() << "\n";
      }

      char *path = _buf;

      while (*path)
      {
        names.push_back(path);
        path += strlen(path) + 1;
      }

      out.push_back(names);

      fRet = FindNextVolumeA(hFind, _guid, sizeof(_guid) / sizeof(char));

      if (!fRet)
      {
        if (GetLastError() != ERROR_NO_MORE_FILES) 
        {
          std::cout << "FindNextVolumeA failed, error : " << GetLastError() << "\n";
        }
      }

    } while(fRet);

    FindVolumeClose(hFind);

    return out;
  }
}
#endif