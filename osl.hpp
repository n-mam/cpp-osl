#ifndef OSL_HPP
#define OSL_HPP

#include <Json.hpp>

#include <filesystem>

namespace OSL
{

void GetVolumeList(Json& json)
{

}

void GetSnapshotList(Json& json)
{

}

void GetDirectoryList(Json& json)
{
  std::string dir;

  if (json.HasKey("Directory"))
  {
    dir = json.GetKey("Directory");
  }

  if (!dir.size())
  {
    dir = std::filesystem::current_path().string();
  }

  std::vector<Json> list;

  for (auto& e : std::filesystem::directory_iterator(dir))
  {
    Json j;

    j.SetKey("name", e.path().filename().string());
    
    if (std::filesystem::is_directory(e))
    {
      j.SetKey("type", "directory");
    }
    else
    {
      j.SetKey("type", "file");
    }

    list.push_back(j);
  }

  json.SetKey("Directory", dir);
  json.SetKey("List", Json::JsonListToArray(list));
}

void GetFreeDriveList(Json& json)
{

}

}

#endif //OSL_HPP