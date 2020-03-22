#include "vdname.h"

#if defined(_WIN32)
#include <dshow.h>
#elif defined(__linux)
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#if defined(_WIN32)

std::string wide_to_multi(std::wstring const& src);

//参考： Geekなページ: ビデオ入力デバイス名を取得する
// https://www.geekpage.jp/programming/directshow/list-capture-device.php

bool getVideoDeviceNames(std::vector<std::string>& deviceNames)
{
  // COMを初期化
  CoInitialize(NULL);

  // デバイスを列挙するためのCreateDevEnumを生成
  ICreateDevEnum *pCreateDevEnum = NULL;
  CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
    IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);

  // VideoInputDeviceを列挙するためのEnumMonikerを生成 
  IEnumMoniker *pEnumMoniker = NULL;
  pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
    &pEnumMoniker, 0);
  if (pEnumMoniker == NULL) {
    // 接続された映像入力デバイスが一つも無い場合
    return false;
  }

  // EnumMonikerをResetする
  // Resetすると、先頭から数えなおす
  pEnumMoniker->Reset();

  deviceNames.clear();

  // 最初のMonikerを取得
  IMoniker *pMoniker = NULL;
  ULONG nFetched = 0;
  while (pEnumMoniker->Next(1, &pMoniker, &nFetched) == S_OK) {
    // IPropertyBagにbindする
    IPropertyBag *pPropertyBag;
    pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
      (void **)&pPropertyBag);

    // Friendly nameを取得するための入れ物
    VARIANT varFriendlyName;
    varFriendlyName.vt = VT_BSTR;

    // FriendlyNameを取得
    pPropertyBag->Read(L"FriendlyName", &varFriendlyName, 0);

    deviceNames.push_back(wide_to_multi(varFriendlyName.bstrVal));

    // 資源を解放
    VariantClear(&varFriendlyName);

    // 資源を解放
    pMoniker->Release();
    pPropertyBag->Release();
  }

  // 資源を解放
  pEnumMoniker->Release();
  pCreateDevEnum->Release();

  // COM終了
  CoUninitialize();

  return true;
}

std::string wide_to_multi(std::wstring const& src)
{
  std::size_t converted{};
  std::vector<char> dest(src.size() * sizeof(wchar_t) + 1, '\0');
  if (::_wcstombs_s_l(&converted, dest.data(), dest.size(), src.data(), _TRUNCATE, ::_create_locale(LC_ALL, "jpn")) != 0) {
    throw std::system_error{ errno, std::system_category() };
  }
  dest.resize(std::char_traits<char>::length(dest.data()));
  dest.shrink_to_fit();
  return std::string(dest.begin(), dest.end());
}

#elif defined(__linux)
bool is_video(const std::string &name);

bool getVideoDeviceNames(std::vector<std::string>& deviceNames)
{
	DIR *dp = opendir("/dev");
	if (dp == NULL) {
		std::cerr << "/dev が開けない" << std::endl;
		return false;
	}

	struct dirent *ep;
	std::vector<std::string> files;
	while ((ep = readdir(dp))) {
		std::string filename = ep->d_name;
		if (is_video(filename)) {
			files.push_back("/dev/" + filename);
		}
	}
	closedir(dp);

	sort(files.begin(), files.end());

    deviceNames.clear();
	for (size_t i = 0; i < files.size(); i++) {
		int fd = open(files[i].c_str(), O_RDWR);
		if (fd < 0)	continue;
		struct v4l2_capability vcap;
		int err = ioctl(fd, VIDIOC_QUERYCAP, &vcap);
		close(fd);
		if (err) continue;
        deviceNames.push_back(std::string((char *)vcap.card));
	}
    return true;
}

bool is_video(const std::string &name)
{
	size_t size = name.size();
	if (size > 5 && name.substr(0,5) == "video") {
		if (isdigit(name[5])) {
			return true;
		}
	}
	return false;
}
#endif