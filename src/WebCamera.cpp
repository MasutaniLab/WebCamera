// -*- C++ -*-
/*!
 * @file  WebCamera.cpp
 * @brief Web Camera RTC with common camera interface version 2.0
 * @date $Date$
 *
 * $Id$
 */

#include "WebCamera.h"
#include <sstream>
#include <vector>
#include <string>
#define HMONITOR HANDLE //HMONITORが未定義となるエラーを回避するため
#include <dshow.h>

using namespace std;

bool getVideoDeviceNames(std::vector<std::string>& deviceNames);
std::string wide_to_multi(std::wstring const& src);

// Module specification
// <rtc-template block="module_spec">
static const char* webcamera_spec[] =
  {
    "implementation_id", "WebCamera",
    "type_name",         "WebCamera",
    "description",       "Web Camera RTC with common camera interface version 2.0",
    "version",           "2.0.2",
    "vendor",            "MasutaniLab",
    "category",          "ImageProcessing",
    "activity_type",     "PERIODIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables
    "conf.default.camera_id", "0",
    "conf.default.output_color_format", "RGB",
    "conf.default.camera_param_filename", "NONE",
    "conf.default.undistortion_flag", "false",
    "conf.default.cap_continuous_flag", "true",
    "conf.default.compression_ratio", "75",
    "conf.default.frame_width", "640",
    "conf.default.frame_height", "480",
    "conf.default.deviceName", "NONE",

    // Widget
    "conf.__widget__.camera_id", "text",
    "conf.__widget__.output_color_format", "radio",
    "conf.__widget__.camera_param_filename", "text",
    "conf.__widget__.undistortion_flag", "radio",
    "conf.__widget__.cap_continuous_flag", "radio",
    "conf.__widget__.compression_ratio", "slider.1",
    "conf.__widget__.frame_width", "text",
    "conf.__widget__.frame_height", "text",
    "conf.__widget__.deviceName", "text",
    // Constraints
    "conf.__constraints__.output_color_format", "(RGB,GRAY,JPEG,PNG)",
    "conf.__constraints__.undistortion_flag", "(true,false)",
    "conf.__constraints__.cap_continuous_flag", "(true,false)",
    "conf.__constraints__.compression_ratio", "0<=x<=100",

    "conf.__type__.camera_id", "int",
    "conf.__type__.output_color_format", "string",
    "conf.__type__.camera_param_filename", "string",
    "conf.__type__.undistortion_flag", "string",
    "conf.__type__.cap_continuous_flag", "string",
    "conf.__type__.compression_ratio", "int",
    "conf.__type__.frame_width", "int",
    "conf.__type__.frame_height", "int",
    "conf.__type__.deviceName", "string",

    ""
  };
// </rtc-template>

/*!
 * @brief constructor
 * @param manager Maneger Object
 */
WebCamera::WebCamera(RTC::Manager* manager)
    // <rtc-template block="initializer">
  : RTC::DataFlowComponentBase(manager),
    m_CameraImageOut("CameraImage", m_CameraImage),
    m_CameraCaptureServicePort("CameraCaptureService")

    // </rtc-template>
{
}

/*!
 * @brief destructor
 */
WebCamera::~WebCamera()
{
}



RTC::ReturnCode_t WebCamera::onInitialize()
{
  RTC_INFO(("onInitialize()"));
  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers
  
  // Set OutPort buffer
  addOutPort("CameraImage", m_CameraImageOut);
  
  // Set service provider to Ports
  m_CameraCaptureServicePort.registerProvider("CameraCaptureService", "Img::CameraCaptureService", m_CameraCaptureService);
  
  // Set service consumers to Ports
  
  // Set CORBA Service Ports
  addPort(m_CameraCaptureServicePort);
  
  // </rtc-template>

  // <rtc-template block="bind_config">
  // Bind variables and configuration variable
  bindParameter("camera_id", m_camera_id, "0");
  bindParameter("output_color_format", m_output_color_format, "RGB");
  bindParameter("camera_param_filename", m_camera_param_filename, "NONE");
  bindParameter("undistortion_flag", m_undistortion_flag, "false");
  bindParameter("cap_continuous_flag", m_cap_continuous_flag, "true");
  bindParameter("compression_ratio", m_compression_ratio, "75");
  bindParameter("frame_width", m_frame_width, "640");
  bindParameter("frame_height", m_frame_height, "480");
  bindParameter("deviceName", m_deviceName, "NONE");
  // </rtc-template>
  
  isFileLoad = false;
  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t WebCamera::onFinalize()
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WebCamera::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WebCamera::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t WebCamera::onActivated(RTC::UniqueId ec_id)
{
  RTC_INFO(("onActivated()"));
  //Open camera device
  string fullname;
  int id = findVideoDevice(m_deviceName, fullname);
  if (id < 0) {
    RTC_WARN(("デバイス %s が見つからない", m_deviceName.c_str()));
    id = m_camera_id;
  } else {
    RTC_INFO(("デバイス正式名： %s", fullname.c_str()));
  }
  RTC_INFO(("デバイス番号： %d", id));
  if(!cam_cap.open(id))
  {
    RTC_ERROR(("Unable to open selected video device ID:[%d].", m_camera_id));
    return RTC::RTC_ERROR;
  }
  coil::sleep(1);
  cam_cap.set(CV_CAP_PROP_FRAME_WIDTH, m_frame_width);
  cam_cap.set(CV_CAP_PROP_FRAME_HEIGHT, m_frame_height);	
  //Get and show the camera device properties
  cam_cap >> src_image;
  width = src_image.cols;
  height = src_image.rows;
  depth = src_image.depth();

  RTC_INFO(("Image size: %d x %d", width, height));
  RTC_INFO(("Depth     : %d", depth));
  RTC_INFO(("Channles  : %d", src_image.channels()));

  //Check channels of camera device
  nchannels = (m_output_color_format == "GRAY") ? 1 : 3;

  if(nchannels > src_image.channels())
  {
    if(m_output_color_format == "RGB" || m_output_color_format == "JPEG" || m_output_color_format == "PNG")
    {
      RTC_INFO(("Convert GRAY image to RGB color image"));
    }
  }
  else if( nchannels < src_image.channels() )
  {
    RTC_INFO(("Convert RGB color image to GRAY image"));
  }
  else
  {
    if (m_output_color_format == "RGB" || m_output_color_format == "JPEG" || m_output_color_format == "PNG")
    {
      RTC_INFO(("Convert BGR color image to RGB color image"));
    } else if (m_output_color_format == "GRAY")
    {
      RTC_INFO(("Gray image"));
    }
  }

  //Load camera parameter
  //If camera parameter file could not be found, whole camera parameters are set to zero.
  RTC_INFO(("Loading camera parameter file: %s", m_camera_param_filename.c_str()));

  cv::FileStorage fs(m_camera_param_filename, cv::FileStorage::READ);
  if(fs.isOpened())
  {
    isFileLoad = true;
    fs["image_width"] >> cam_param.imageSize.width;
    fs["image_height"] >> cam_param.imageSize.height;
    fs["camera_matrix"] >> cam_param.cameraMatrix;
    fs["distortion_coefficients"] >> cam_param.distCoeffs;

    RTC_INFO(("================================================="));
    RTC_INFO(("Camera Parameter"));
    RTC_INFO(("================================================="));

    RTC_INFO(("Image size: %d x %d", cam_param.imageSize.width, cam_param.imageSize.height));
    ostringstream ss;
    ss << "Camera Matrix: " << cam_param.cameraMatrix;
    RTC_INFO((ss.str().c_str()));
    ss.str("");
    ss.clear(stringstream::goodbit);
    ss << "Distortion coefficients: " << cam_param.distCoeffs;
    RTC_INFO((ss.str().c_str()));

    //Set distortion coefficient to make rectify map
    CameraParam *param;
    param = &cam_param;
    cv::initUndistortRectifyMap(param->cameraMatrix, param->distCoeffs, cv::Mat(),
      cv::getOptimalNewCameraMatrix(param->cameraMatrix, param->distCoeffs, param->imageSize, 1, param->imageSize, 0),
      param->imageSize, CV_16SC2, param->map1, param->map2);

  }
  else
  {
    RTC_WARN(( "Unable to open selected camera parameter file: %s", m_camera_param_filename.c_str() ));
    RTC_WARN(( "Camera parameters are set to zero" ));

    isFileLoad = false;

    cam_param.imageSize.width = width;
    cam_param.imageSize.height = height;
    //Clear camera intrinsic parameter to zero
    cam_param.cameraMatrix = cv::Mat_<double>(3,3);
    cam_param.cameraMatrix.at<double>(0,0) = 0.0;
    cam_param.cameraMatrix.at<double>(0,1) = 0.0;
    cam_param.cameraMatrix.at<double>(0,2) = 0.0;
    cam_param.cameraMatrix.at<double>(1,0) = 0.0;
    cam_param.cameraMatrix.at<double>(1,1) = 0.0;
    cam_param.cameraMatrix.at<double>(1,2) = 0.0;
    cam_param.cameraMatrix.at<double>(2,0) = 0.0;
    cam_param.cameraMatrix.at<double>(2,1) = 0.0;
    cam_param.cameraMatrix.at<double>(2,2) = 1.0;

    //Clear distortion parameter to zero
    cam_param.distCoeffs = cv::Mat_<double>(5,1);
    cam_param.distCoeffs.at<double>(0) = 0.0;
    cam_param.distCoeffs.at<double>(1) = 0.0;
    cam_param.distCoeffs.at<double>(2) = 0.0;
    cam_param.distCoeffs.at<double>(3) = 0.0;
    cam_param.distCoeffs.at<double>(4) = 0.0;

  }
  //Set default capture mode
  m_CameraCaptureService.m_cap_continuous = coil::toBool(m_cap_continuous_flag, "true", "false");

  RTC_INFO(("Capture mode: %s", m_cap_continuous_flag.c_str()));
  RTC_INFO(("Capture start!!"));
  return RTC::RTC_OK;
}


RTC::ReturnCode_t WebCamera::onDeactivated(RTC::UniqueId ec_id)
{
  RTC_INFO(("onDeactivated()"));
  RTC_INFO(("Capture stop!!"));

  //Release the device handler and allocated image buffer
  src_image.release();
  cam_cap.release();

  return RTC::RTC_OK;
}


RTC::ReturnCode_t WebCamera::onExecute(RTC::UniqueId ec_id)
{
  //Capture mode select
  if(m_CameraCaptureService.m_cap_continuous || (m_CameraCaptureService.m_cap_count > 0))
  {
    if( m_CameraCaptureService.m_cap_count > 0) --m_CameraCaptureService.m_cap_count;

    //Check image channels for captured image via camera
    nchannels = ( m_output_color_format == "GRAY") ? 1 : 3;

    //Image Capture
    cam_cap >> src_image;
    if( src_image.empty() )
    {
      RTC_WARN(("Capture image data is empty!!"));
      return RTC::RTC_OK;
    }

    //***********************************************************************************
    //***********************************************************************************
    //Following part is the template for common camera interface
    //Don't change following part
    //***********************************************************************************
    //***********************************************************************************

    //Set timestamp
    setTimestamp(m_CameraImage);
    m_CameraImage.data.captured_time = m_CameraImage.tm;

    //Color conversion process
    cv::Mat proc_image;
    if(nchannels > src_image.channels())
    {
      if( m_output_color_format == "RGB" || m_output_color_format == "JPEG" || m_output_color_format == "PNG")
        cv::cvtColor(src_image, proc_image, CV_GRAY2RGB);
      nchannels = 3;
    }
    else if( nchannels < src_image.channels() )
    {
      if( m_output_color_format == "GRAY" )
      {
        cv::cvtColor(src_image, proc_image, CV_BGR2GRAY);
        nchannels = 1;
      }
    }
    else
    {
      if( m_output_color_format == "RGB" )
        cv::cvtColor(src_image, proc_image, CV_BGR2RGB);
      else
        proc_image = src_image;
    }
    //Set camera parameter to output structure data
    m_CameraImage.data.intrinsic.matrix_element[0] = cam_param.cameraMatrix.at<double>(0,0);
    m_CameraImage.data.intrinsic.matrix_element[1] = cam_param.cameraMatrix.at<double>(0,2);
    m_CameraImage.data.intrinsic.matrix_element[2] = cam_param.cameraMatrix.at<double>(1,1);
    m_CameraImage.data.intrinsic.matrix_element[3] = cam_param.cameraMatrix.at<double>(1,2);
    m_CameraImage.data.intrinsic.matrix_element[4] = cam_param.cameraMatrix.at<double>(2,2);


    //Copy undistortion matrix
    m_CameraImage.data.intrinsic.distortion_coefficient.length(cam_param.distCoeffs.rows);
    cv::Mat distortion_temp;
    if(coil::toBool(m_undistortion_flag, "true", "false"))
    {
      if(isFileLoad)
      {
        distortion_temp = proc_image.clone();
        cv::undistort(distortion_temp, proc_image, cam_param.cameraMatrix, cam_param.distCoeffs, cam_param.cameraMatrix);

      }
      //Copy distortion coefficient to output parameter
      for(int j(0); j < 5; ++j)
        m_CameraImage.data.intrinsic.distortion_coefficient[j] = 0.0; 
    }
    else
    {
      //Copy distortion coefficient to output parameter
      for(int j(0); j < 5; ++j)
        m_CameraImage.data.intrinsic.distortion_coefficient[j] = cam_param.distCoeffs.at<double>(j);
    }

    //Copy image parameter to output data based on TimedCameraImage structure
    m_CameraImage.data.image.width = width;
    m_CameraImage.data.image.height = height;

    //Transmission image data creation based on selected image compression mode
    if( m_output_color_format == "RGB")
    {
      m_CameraImage.data.image.format = Img::CF_RGB;
      m_CameraImage.data.image.raw_data.length( width * height * nchannels);
      for( int i(0); i< height; ++i )
        memcpy(&m_CameraImage.data.image.raw_data[ i * width * nchannels], &proc_image.data[ i * proc_image.step ], width * nchannels);
    }
    else if(m_output_color_format == "JPEG")
    {
      m_CameraImage.data.image.format = Img::CF_JPEG;
      //Jpeg encoding using OpenCV image compression function
      std::vector<int> compression_param = std::vector<int>(2); 
      compression_param[0] = CV_IMWRITE_JPEG_QUALITY;
      compression_param[1] = m_compression_ratio;
      //Encode raw image data to jpeg data
      std::vector<uchar> compressed_image;
      cv::imencode(".jpg", proc_image, compressed_image, compression_param);
      //Copy encoded jpeg data to Outport Buffer
      m_CameraImage.data.image.raw_data.length(compressed_image.size());
      memcpy(&m_CameraImage.data.image.raw_data[0], &compressed_image[0], sizeof(unsigned char) * compressed_image.size());
    }
    else if(m_output_color_format == "PNG")
    {
      m_CameraImage.data.image.format = Img::CF_PNG;
      //Jpeg encoding using OpenCV image compression function
      std::vector<int> compression_param = std::vector<int>(2); 
      compression_param[0] = CV_IMWRITE_PNG_COMPRESSION;
      compression_param[1] = (int)((double)m_compression_ratio/10.0);
      if(compression_param[1] == 10)
        compression_param[1] = 9;
      RTC_INFO(("PNG compression ratio: %d", compression_param[1]));


      //Encode raw image data to jpeg data
      std::vector<uchar> compressed_image;
      cv::imencode(".png", proc_image, compressed_image, compression_param);
      //Copy encoded jpeg data to Outport Buffer
      m_CameraImage.data.image.raw_data.length(compressed_image.size());
      memcpy(&m_CameraImage.data.image.raw_data[0], &compressed_image[0], sizeof(unsigned char) * compressed_image.size());
    }
    else
    {
      m_CameraImage.data.image.format = Img::CF_GRAY;

      RTC_INFO(("Selected image compression mode is not defined. Please confirm correct compression mode!"));
      m_CameraImage.data.image.raw_data.length( width * height * nchannels);
      for( int i(0); i< height; ++i )
        memcpy(&m_CameraImage.data.image.raw_data[ i * width * nchannels], &proc_image.data[ i * proc_image.step ], width * nchannels);			
    }
    //Output image data via OutPort
    m_CameraImageOut.write();
    //***********************************************************************************
    //End of the template part
    //***********************************************************************************
  }
  else
  {
    RTC_DEBUG(("Waiting capture mode command via ServicePort"));
    return RTC::RTC_OK;
  }
  return RTC::RTC_OK;
}


RTC::ReturnCode_t WebCamera::onAborting(RTC::UniqueId ec_id)
{
  RTC_INFO(("onAborting()"));
  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t WebCamera::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WebCamera::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WebCamera::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WebCamera::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


int WebCamera::findVideoDevice(const std::string &name, std::string &fullname)
{
  vector<string> deviceNames;

  if (!getVideoDeviceNames(deviceNames)) {
    return -1;
  }

  for (int i = 0; i < deviceNames.size(); i++) {
    RTC_INFO(("%d: %s", i, deviceNames[i].c_str()));
  }

  for (int i = 0; i < deviceNames.size(); i++) {
    if (deviceNames[i].find(name) != string::npos) {
      fullname = deviceNames[i];
      return i;
    }
  }
  fullname.clear();
  return -1;
}

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

extern "C"
{
 
  void WebCameraInit(RTC::Manager* manager)
  {
    coil::Properties profile(webcamera_spec);
    manager->registerFactory(profile,
                             RTC::Create<WebCamera>,
                             RTC::Delete<WebCamera>);
  }
  
};


