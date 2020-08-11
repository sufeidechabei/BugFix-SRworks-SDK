//========= Copyright 2017, HTC Corporation. All rights reserved. ===========
#include <opencv.hpp>
#include <Windows.h>
#include "ViveSR_Enums.h"
#include "ViveSR_API_Enums.h"
#include "ViveSR_Client.h"
#include "ViveSR_Structs.h"
#include "ViveSR_PassThroughEnums.h"
#include "ViveSR_DepthEnums.h"
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#define DISTORTED_IMAGE_W 612
#define DISTORTED_IMAGE_H 460
#define DISTORTED_IMAGE_C 4
#define UNDISTORTED_IMAGE_W 1150
#define UNDISTORTED_IMAGE_H 750
#define UNDISTORTED_IMAGE_C 4
#define DEPTH_DATA_W 640
#define DEPTH_DATA_H 480
#define DEPTH_DATA_C0 4
#define DEPTH_DATA_C1 1

cv::Mat img_colorbar, img_jet;

double CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_PARAMS_MAX];
#define OFFSET_ID(x) (x-ViveSR::PassThrough::Param::CAMERA_BRIGHTNESS)

bool EnableKeyboardInput = true;

void CreateColorbarImage();
void Loop();
void GetDistortedCallback(int key);
void GetUndistortedCallback(int key);
void GetDepthCallback(int key);

void TestGetFrameData(bool isDistorted);
void TestGetDepthData();
struct StreamingControl
{
    enum ShowOption{
        SHOW_DISTORTED,
        SHOW_UNDISTORTED,
        SHOW_DEPTH,
        SHOW_MAX
    };
    inline bool CheckOption(ShowOption option) {
        return 0!=(option_&(1 << option));
    }
    inline void EnableOption(ShowOption option) {
        option_ |= (1 << option);
    }
    inline void DisableOption(ShowOption option) {
        option_ &= (0 << option);
    }
    uchar option_ = 0;
    bool streaming_ = false;
    std::mutex steaming_mutex_;
};
/* @brief Order by following enum
    CAMERA_CONTROL_STATUS = 400,
    CAMERA_CONTROL_DEFAULT_VALUE,
    CAMERA_CONTROL_MIN,
    CAMERA_CONTROL_MAX,
    CAMERA_CONTROL_STEP,
    CAMERA_CONTROL_DEFAULT_MODE,
    CAMERA_CONTROL_VALUE,
    CAMERA_CONTROL_MODE,
*/
const static size_t PARAM_LENGTH = (ViveSR::PassThrough::Param::CAMERA_CONTROL_MODE - ViveSR::PassThrough::Param::CAMERA_CONTROL_STATUS) + 1;
union ParamInfo {
    struct {
        int32_t nStatus;
        int32_t nDefaultValue;
        int32_t nMin;
        int32_t nMax;
        int32_t nStep;
        int32_t nDefaultMode;
        int32_t nValue;
        int32_t nMode;
    };
    int32_t param_arr[PARAM_LENGTH];
};
static StreamingControl g_streaming_control;
void StreamingShowImage();
int ID_PASSTHROUGH, ID_DEPTH;
/* @brief Order by following enum
    CAMERA_CRTL_BRIGHTNESS = CAMERA_BRIGHTNESS,
    CAMERA_CRTL_CONTRAST = CAMERA_BRIGHTNESS,
    CAMERA_CRTL_HUE = CAMERA_HUE,
    CAMERA_CRTL_SATURATION = CAMERA_SATURATION,
    CAMERA_CRTL_SHARPNESS = CAMERA_SHARPNESS,
    CAMERA_CRTL_GAMMA = CAMERA_GAMMA,
    CAMERA_CRTL_COLOR_ENABLE = CAMERA_COLOR_ENABLE,
    CAMERA_CRTL_WHITE_BALANCE = CAMERA_WHITE_BALANCE,
    CAMERA_CRTL_BACKLIGHT_COMPENSATION = CAMERA_BACKLIGHT_COMPENSATION,
    CAMERA_CRTL_GAIN = CAMERA_GAIN,
    CAMERA_CRTL_PAN = CAMERA_PAN,
    CAMERA_CRTL_TILT = CAMERA_TILT,
    CAMERA_CRTL_ROLL = CAMERA_ROLL,
    CAMERA_CRTL_ZOOM = CAMERA_ZOOM,
    CAMERA_CRTL_EXPOSURE = CAMERA_EXPOSURE,
    CAMERA_CRTL_IRIS = CAMERA_IRIS,
    CAMERA_CRTL_FOCUS = CAMERA_FOCUS,
*/
const static size_t QUALITIES_LENGTH = (ViveSR::PassThrough::CameraControlType::CAMERA_CRTL_FOCUS - ViveSR::PassThrough::CameraControlType::CAMERA_CRTL_BRIGHTNESS) + 1;
ParamInfo gCameraQualityParamInfo[QUALITIES_LENGTH];
std::string Quality[QUALITIES_LENGTH]=
{
	"Camera_Brightness","Camera_Contrast","Camera_Hue","Camera_Saturation","Camera_Sharpness",
	"Camera_Gamma","Camera_ColorEnable","Camera_WhiteBalance","Camera_BacklightCompensation","Camera_Gain",
	"Camera_Pan","Camera_Tilt","Camera_Roll","Camera_Zoom","Camera_Exposure",
	"Camera_Iris","Camera_Focus"
};
int main()
{
	CreateColorbarImage();
    std::thread* show_img_thread = nullptr;
	{
		int flag = 0;		
        int res = ViveSR::Error::INVALID_INPUT;
        if (ViveSR_GetContextInfo(NULL) != ViveSR::Error::WORK) {
            res = ViveSR_CreateContext("", 0);
        }
		if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_Initial failed %d\n", res);  goto stop; }
		else
		{
			res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_PASS_THROUGH, &ID_PASSTHROUGH);
			fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_PASSTHROUGH, res != ViveSR::Error::WORK ? "failed" : "success", res);
			if (res != ViveSR::Error::WORK) goto stop;

			res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_DEPTH, &ID_DEPTH);
			fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_DEPTH, res != ViveSR::Error::WORK ? "failed" : "success", res);
			if (res != ViveSR::Error::WORK) goto stop;

			Sleep(1000);

            ViveSR_InitialModule(ID_PASSTHROUGH);
            ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::DEPTH_UNDISTORT_GPU_TO_CPU_ENABLE, true);
            ViveSR_InitialModule(ID_DEPTH);


            int result = ViveSR::Error::FAILED;
            for (int i = ViveSR::PassThrough::CAMERA_Param::CAMERA_FCX_0; i < ViveSR::PassThrough::CAMERA_Param::CAMERA_PARAMS_MAX; i++)
            {
                result = ViveSR_GetParameterDouble(ID_PASSTHROUGH, i, &(CameraParameters[i]));
                if (result == ViveSR::Error::FAILED)
                    goto stop;
            }
			ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::VR_INIT, true);
			ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::Param::VR_INIT_TYPE, ViveSR::PassThrough::InitType::SCENE);

			res = ViveSR_StartModule(ID_PASSTHROUGH);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", 0, res); goto stop; }
			res = ViveSR_StartModule(ID_DEPTH);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", 1, res); goto stop; }

			res = ViveSR_LinkModule(ID_PASSTHROUGH, ID_DEPTH, ViveSR::LinkType::ACTIVE);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSRs_link %d to %d failed(%d)\n", 0, 1, res); goto stop; }

			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_Start failed\n"); goto stop; }
            else {
                g_streaming_control.streaming_ = true;
                show_img_thread = new std::thread(StreamingShowImage);
                Loop();
            }
		}
    stop:
        g_streaming_control.streaming_ = false;
        show_img_thread->join();
        delete show_img_thread;

        auto error = ViveSR_StopModule(ID_PASSTHROUGH);
        if (error == ViveSR::Error::WORK) { printf("Successfully release see through engine.\n"); }
        else printf("Fail to release see through engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        error = ViveSR_StopModule(ID_DEPTH);
        if (error == ViveSR::Error::WORK) { printf("Successfully release depth engine.\n"); }
        else printf("Fail to release depth engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        fprintf(stderr, "Exit. Please press any key to close it.\n");
       
        getchar();
	}
	return 0;
}

void Loop() {
	char str = 0;
    int res = ViveSR::Error::FAILED;
	while (true)
	{
        if (EnableKeyboardInput) {
            if (str != '\n' && str != EOF) fprintf(stderr, "wait for key event : \n");
            str = getchar();
            if (str == '`') break;
            else if (str == 'q') { // get all camera quality information
                const static auto CAMERA_CRTL_MAX = ViveSR::PassThrough::CAMERA_CRTL_FOCUS + 1;
                for (int i = ViveSR::PassThrough::CAMERA_CRTL_BRIGHTNESS; i < CAMERA_CRTL_MAX; i++) {
                    ParamInfo *target = &gCameraQualityParamInfo[OFFSET_ID(i)];
                    const static auto CAMERA_OPTION_MAX = ViveSR::PassThrough::CAMERA_CONTROL_MODE;
                    {
                        std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                        bool query_current_option = true;
                        auto res = ViveSR_GetParameterBool(ID_PASSTHROUGH, i, &query_current_option);//
                        for (int j = ViveSR::PassThrough::CAMERA_CONTROL_STATUS; j < CAMERA_OPTION_MAX; ++j)
                        {
                            auto param_ptr = &(target->param_arr[j - ViveSR::PassThrough::CAMERA_CONTROL_STATUS]);
                            if (ViveSR::Error::WORK != ViveSR_GetParameterInt(ID_PASSTHROUGH, j, param_ptr)) {
                                fprintf(stderr, "Can't get value on enumerate:%d \n", j);
                            }
                        }
                    }

                    if (gCameraQualityParamInfo[OFFSET_ID(i)].nStatus == ViveSR::Error::WORK) {
                        fprintf(stderr, "Get %-30s current value:%d\n", Quality[OFFSET_ID(i)].c_str(), target->nValue);
                        fprintf(stderr, "Get %-30s max value    :%d\n", Quality[OFFSET_ID(i)].c_str(), target->nMax);
                        fprintf(stderr, "Get %-30s min value    :%d\n", Quality[OFFSET_ID(i)].c_str(), target->nMin);
                        fprintf(stderr, "Get %-30s mode         :%s\n", Quality[OFFSET_ID(i)].c_str(), target->nMode == ViveSR::PassThrough::CTRL_MODE::CTRL_MANUAL ? "Manual" : "Auto");
                        fprintf(stderr, "Get %-30s mode default value    :%d\n", Quality[OFFSET_ID(i)].c_str(), target->nDefaultValue);
                        fprintf(stderr, "Get %-30s mode default mode    :%s\n", Quality[OFFSET_ID(i)].c_str(), target->nDefaultMode == ViveSR::PassThrough::CTRL_MODE::CTRL_MANUAL ? "Manual" : "Auto");
                    }
                    else	fprintf(stderr, "NOT support %-30s\n", Quality[OFFSET_ID(i)].c_str());
                    fprintf(stderr, "------------------------------------\n");
                }
            }
            else if (str == 'w') { // set all camera quality value (here we use maximum for demonstration)
                const static auto CAMERA_CRTL_MAX = ViveSR::PassThrough::CAMERA_CRTL_FOCUS;
                for (int i = ViveSR::PassThrough::CAMERA_CRTL_BRIGHTNESS; i < CAMERA_CRTL_MAX; i++) {
                    ParamInfo *target = &gCameraQualityParamInfo[OFFSET_ID(i)];
                    {
                        std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                        bool query_current_option = true;
                        auto res = ViveSR_GetParameterBool(ID_PASSTHROUGH, i, &query_current_option);
                        res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_STATUS, &(target->nStatus));
                        res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_MAX, &(target->nMax));
                        if (res == ViveSR::Error::WORK && target->nStatus == ViveSR::Error::WORK) {
                            fprintf(stderr, "before %-30s value: %d(%s)\n", Quality[OFFSET_ID(i)].c_str(), target->nValue, res == ViveSR::Error::WORK ? "OK" : "FAIL");
                            res = ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_VALUE, target->nMax);
                            auto res = ViveSR_SetParameterBool(ID_PASSTHROUGH, i, &query_current_option);
                            fprintf(stderr, "After %-30s value: %d(%s)\n", Quality[OFFSET_ID(i)].c_str(), target->nMax, res == ViveSR::Error::WORK ? "OK" : "FAIL");
                        }
                        else fprintf(stderr, "set %s failed(%d), status %s\n", Quality[OFFSET_ID(i)].c_str(), res, target->nStatus == ViveSR::Error::WORK ? "OK" : "NOT_SUPPORT");
                    }
                }
            }
            else if (str == 'e') {  // reset all camera quality to default status
                {
                    std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                    auto res = ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::RESET_AllCAMERA_QUALITY, true);
                    fprintf(stderr, "Send Command : RESET All CAMERA QUALITY SETTING(%d)\n", res);
                };
            }
            else if (str == 'a') { // get Camera_Brightness information
                ParamInfo *target = &gCameraQualityParamInfo[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)];
                bool query_current_option = true;
                {
                    std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                    auto res = ViveSR_GetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_BRIGHTNESS, &query_current_option);
                    //////////////////////////////////////////////////////////////////////////
                    const static auto CAMERA_OPTION_MAX = ViveSR::PassThrough::CAMERA_CONTROL_MODE + 1;
                    for (int i = ViveSR::PassThrough::CAMERA_CONTROL_STATUS; i < CAMERA_OPTION_MAX; ++i)
                        if (ViveSR::Error::WORK != ViveSR_GetParameterInt(ID_PASSTHROUGH, i, &(target->param_arr[i]))) {
                            fprintf(stderr, "Invalid operation on getting parameter on enumerate :%d \n", i);
                        }
                }
                //////////////////////////////////////////////////////////////////////////
                if (target->nStatus == ViveSR::Error::WORK) {
                    fprintf(stderr, "Get %-30s current value:%d \n",
                        Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str(),
                        target->nValue);
                    fprintf(stderr, "Get %-30s max value    :%d(%s)\n",
                        Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str(),
                        target->nMax);
                    fprintf(stderr, "Get %-30s min value    :%d(%s)\n",
                        Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str(),
                        target->nMin);
                    fprintf(stderr, "Get %-30s mode value    :%s(%s)\n",
                        Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str(),
                        target->nMode == ViveSR::PassThrough::CTRL_MODE::CTRL_MANUAL ? "Manual" : "Auto");
                }
                else	fprintf(stderr, "NOT support %-30s\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str());
            }
            else if (str == 's') { // set Camera_Brightness to max/min
                static bool flag = true;
                ParamInfo *target = &gCameraQualityParamInfo[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)];
                bool select_current_option = true;
                {
                    std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                    auto res = ViveSR_GetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_BRIGHTNESS, &select_current_option);
                    res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_MAX, &(target->nMax));
                    res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_MIN, &(target->nMin));
                    //
                    res = ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_VALUE, flag ? target->nMax : target->nMin);
                    res = ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_BRIGHTNESS, select_current_option);
                    res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_STATUS, &(target->nStatus));
                }
                if (target->nStatus == ViveSR::Error::WORK) {
                    fprintf(stderr, "Set %-30s value: %d(%s)\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str(), target->nValue, res == ViveSR::Error::WORK ? "OK" : "FAIL");
                    flag = !flag;
                }
                else	fprintf(stderr, "NOT support %-30s\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_BRIGHTNESS)].c_str());
            }
            else if (str == 'd') { // set Camera_WhiteBalance to Manual/Auto
                ParamInfo *target = &gCameraQualityParamInfo[OFFSET_ID(ViveSR::PassThrough::CAMERA_WHITE_BALANCE)];
                static bool flag = true;
                bool select_current_option = true;
                {
                    std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                    auto res = ViveSR_GetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_WHITE_BALANCE, &select_current_option);
                    res = ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_MODE, flag ? ViveSR::PassThrough::CTRL_MODE::CTRL_MANUAL : ViveSR::PassThrough::CTRL_MODE::CTRL_AUTO);
                    res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_STATUS, &(target->nStatus));
                }
                if (target->nStatus == ViveSR::Error::WORK) {
                    fprintf(stderr, "Set %-30s mode: %s(%s)\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_WHITE_BALANCE)].c_str(), flag ? "CTRL_Manual" : "CTRL_Auto", res == ViveSR::Error::WORK ? "OK" : "FAIL");
                    flag = !flag;
                }
                else	fprintf(stderr, "NOT support %-30s\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_WHITE_BALANCE)].c_str());

            }
            else if (str == 'f') { // set Camera_Exposure to Manual/Auto
                static bool flag = true;
                ParamInfo *target = &gCameraQualityParamInfo[OFFSET_ID(ViveSR::PassThrough::CAMERA_EXPOSURE)];
                bool select_current_option = true;
                {
                    std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                    auto res = ViveSR_GetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_EXPOSURE, &select_current_option);
                    res = ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_MODE, flag ? ViveSR::PassThrough::CTRL_MODE::CTRL_MANUAL : ViveSR::PassThrough::CTRL_MODE::CTRL_AUTO);
                    res = ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_EXPOSURE, select_current_option);
                    res = ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::CAMERA_CONTROL_STATUS, &(target->nStatus));
                }
                if (target->nStatus == ViveSR::Error::WORK) {
                    fprintf(stderr, "Set %-30s mode: %s(%s)\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_EXPOSURE)].c_str(), flag ? "CTRL_Manual" : "CTRL_Auto", res == ViveSR::Error::WORK ? "OK" : "FAIL");
                    flag = !flag;
                }
                else	fprintf(stderr, "NOT support %-30s\n", Quality[OFFSET_ID(ViveSR::PassThrough::CAMERA_EXPOSURE)].c_str());
            }
            else if (str == '1') {
                g_streaming_control.EnableOption(StreamingControl::SHOW_DISTORTED);
                fprintf(stderr, "Show distorted image\n");
            }
            else if (str == '2') {
                g_streaming_control.DisableOption(StreamingControl::SHOW_DISTORTED);
                fprintf(stderr, "Stop showing distorted image\n");
            }
            else if (str == '3') {
                g_streaming_control.EnableOption(StreamingControl::SHOW_UNDISTORTED);
                fprintf(stderr, "Show undistorted image\n");
            }
            else if (str == '4') {
                g_streaming_control.DisableOption(StreamingControl::SHOW_UNDISTORTED);
                fprintf(stderr, "Stop showing undistorted image\n");
            }
            else if (str == '5')
            {
                int width, height, channel;
                ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_WIDTH, &width);
                ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_HEIGHT, &height);
                ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_CHANNEL, &channel);
                fprintf(stderr, "Distorted width :%d, height: %d, channel : %d\n", width, height, channel);
            }
            else if (str == '6') {
                g_streaming_control.EnableOption(StreamingControl::SHOW_DEPTH);
                fprintf(stderr, "Show Depth image\n");
            }
            else if (str == '7') {
                g_streaming_control.DisableOption(StreamingControl::SHOW_DEPTH);
                fprintf(stderr, "Stop showing depth image\n");
            }

		}
	}
}
cv::Vec3f GetColour(float v, float vmin, float vmax)
{
	cv::Vec3f c = { 1.0f, 1.0f, 1.0f }; // white
	float dv;

	if (v < vmin)
		v = vmin;
	if (v > vmax)
		v = vmax;
	dv = vmax - vmin;

	if (v < (vmin + 0.25 * dv)) {
		c[0] = 0;
		c[1] = 4 * (v - vmin) / dv;
	}
	else if (v < (vmin + 0.5 * dv)) {
		c[0] = 0;
		c[2] = 1 + 4 * (vmin + 0.25 * dv - v) / dv;
	}
	else if (v < (vmin + 0.75 * dv)) {
		c[0] = 4 * (v - vmin - 0.5 * dv) / dv;
		c[2] = 0;
	}
	else {
		c[1] = 1 + 4 * (vmin + 0.75 * dv - v) / dv;
		c[2] = 0;
	}
	return(c);
}
void CreateColorbarImage() {
	int width_colorbar = 640;
	int height_colorbar = 60;
	int width_jet = 620;
	int height_jet = 30;
	img_colorbar = cv::Mat(height_colorbar, width_colorbar, CV_8UC3);
	img_jet = cv::Mat(height_jet, width_jet, CV_8UC3);
	for (int i = 0; i < width_jet; i++) {
		for (int j = 0; j < height_jet; j++) {
			img_jet.at<cv::Vec3b>(j, i) = (cv::Vec3b)(GetColour((float)i / width_jet, 0, 1) * 255);
		}
	}
	cv::Rect dstRect = cv::Rect(10, 20, width_jet, height_jet);
	cv::Mat dstROI = img_colorbar(dstRect);
	img_jet.copyTo(dstROI);
	cv::Scalar text_color = cv::Scalar(255, 255, 255);
	cv::putText(img_colorbar, cv::String("255"), cv::Point(617, 13), 0, 0.4, text_color, 1);
	cv::putText(img_colorbar, cv::String("192"), cv::Point(466, 13), 0, 0.4, text_color, 1);
	cv::putText(img_colorbar, cv::String("128"), cv::Point(310, 13), 0, 0.4, text_color, 1);
	cv::putText(img_colorbar, cv::String("64"), cv::Point(154, 13), 0, 0.4, text_color, 1);
	cv::putText(img_colorbar, cv::String("0"), cv::Point(5, 13), 0, 0.4, text_color, 1);
}

void StreamingShowImage()
{
#pragma region ALLOCATE_SEE_THROUGH
    int distorted_width, distorted_height, distorted_channel;
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_WIDTH, &distorted_width);
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_HEIGHT, &distorted_height);
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_CHANNEL, &distorted_channel);
    auto Distored_img_CV_Type =distorted_channel == 3 ? CV_8UC3 : CV_8UC4;
    auto Distored_img_CV_Code =distorted_channel == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
    //
    int undistorted_width, undistorted_height, undistorted_channel;
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_UNDISTORTED_WIDTH, &undistorted_width);
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_UNDISTORTED_HEIGHT, &undistorted_height);
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_UNDISTORTED_CHANNEL, &undistorted_channel);
    //initial see through data buffer
    std::unique_ptr<char[]> distorted_frame_left = std::make_unique<char[]>(distorted_width * distorted_height * distorted_channel);
    std::unique_ptr<char[]> distorted_frame_right = std::make_unique<char[]>(distorted_width * distorted_height * distorted_channel);
    std::unique_ptr<char[]> undistorted_frame_left = std::make_unique<char[]>(undistorted_width * undistorted_height * undistorted_channel);
    std::unique_ptr<char[]> undistorted_frame_right = std::make_unique<char[]>(undistorted_width * undistorted_height * undistorted_channel);
    float pose_left[16];
    float pose_right[16];
    std::unique_ptr<char[]> camera_params_see_through = std::make_unique<char[]>(1032);
    //
    unsigned int frameSeq;
    unsigned int timeStp;
    //
    int lux_left;
    int lux_right;
    int color_temperature_left;
    int color_temperature_right;
    int exposure_time_left;
    int exposure_time_right;
    int analog_gain_left;
    int analog_gain_right;
    int digital_gain_left;
    int digital_gain_right;
    const int see_throught_length = ViveSR::PassThrough::OutputMask::MAX;
    static std::vector<ViveSR::MemoryElement> see_through_element(see_throught_length);
    void* see_through_ptrs[ViveSR::PassThrough::OutputMask::MAX]{
        distorted_frame_left.get(),
        distorted_frame_right.get(),
        undistorted_frame_left.get(),
        undistorted_frame_right.get(),
        &frameSeq,
        &timeStp,
        pose_left,
        pose_right,
        &lux_left,
        &lux_right,
        &color_temperature_left,
        &color_temperature_right,
        &exposure_time_left,
        &exposure_time_right,
        &analog_gain_left,
        &analog_gain_right,
        &digital_gain_left,
        &digital_gain_right,
        camera_params_see_through.get()
    };
    int see_through_count = 0;
    for (int i = 0; i < see_throught_length; ++i)
    {
        if (see_through_ptrs[i])
        {
            see_through_element[see_through_count].mask = i;
            see_through_element[see_through_count].ptr = see_through_ptrs[i];
            see_through_count++;
        }
    }
#pragma endregion ALLOCATE_SEE_THROUGH
#pragma region ALLOCATE_DEPTH
    const static int DEPTH_ELEM_LENGTH = ViveSR::Depth::OutputMask::OUTPUT_MASK_MAX;
    int depth_img_width, depth_img_height, depth_img_channel, depth_color_img_channel;
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_WIDTH, &depth_img_width);
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_HEIGHT, &depth_img_height);
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_CHAANEL_1, &depth_img_channel);
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_CHAANEL_0, &depth_color_img_channel);
    static std::vector<ViveSR::MemoryElement> depth_element(DEPTH_ELEM_LENGTH);
    auto left_frame = std::make_unique<char[]>(depth_img_height*depth_img_width*depth_color_img_channel);
    auto depth_map = std::make_unique<float[]>(depth_img_height*depth_img_width*depth_img_channel);
    unsigned int frame_seq;	        // sizeof(unsigned int)
    unsigned int time_stp;		    // sizeof(unsigned int)
    float pose[16];		            // sizeof(float) * 16
    
    
    auto camera_params = std::make_unique<char[]>(1032);
    void* depth_ptrs[ViveSR::Depth::OutputMask::OUTPUT_MASK_MAX]{
        left_frame.get(),
        depth_map.get(),
        &frame_seq,
        &time_stp,
        pose,
        &lux_left,
        &color_temperature_left,
        &exposure_time_left,
        &analog_gain_left,
        &digital_gain_left,
        camera_params.get(),
    };
    int depth_count = 0;
    for (int i = 0; i < DEPTH_ELEM_LENGTH; ++i)
    {
        if (depth_ptrs[i])
        {
            depth_element[depth_count].mask = i;
            depth_element[depth_count].ptr = depth_ptrs[i];
            depth_count++;
        }
    }
#pragma endregion ALLOCATE_DEPTH
    auto last_frame_timestamp = std::chrono::high_resolution_clock::now();
    using namespace std::chrono_literals;
    const static auto MIN_FRAME_INTERVAL = 0.3333333ms;
    while (g_streaming_control.streaming_)
    {
#pragma region FPS_CONTROL
        //FPS
        auto now_timestamp = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now_timestamp - last_frame_timestamp);
        if (MIN_FRAME_INTERVAL > duration) {
            std::this_thread::sleep_for(MIN_FRAME_INTERVAL - duration);
        }
        last_frame_timestamp = now_timestamp;
#pragma endregion FPS_CONTROL
#pragma region SHOW_SEETROUGH
        //See through
        if (g_streaming_control.CheckOption(StreamingControl::SHOW_DISTORTED) ||
            g_streaming_control.CheckOption(StreamingControl::SHOW_UNDISTORTED))
        {
            int res = ViveSR::FAILED;
            {
                std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                res = ViveSR_GetModuleData(ID_PASSTHROUGH, see_through_element.data(), see_through_count);
            }
            //
            if (res == ViveSR::WORK) {
                if (g_streaming_control.CheckOption(StreamingControl::SHOW_DISTORTED)) {
                    cv::Mat imgLeft = cv::Mat(distorted_height, distorted_width, Distored_img_CV_Type, distorted_frame_left.get()).clone();
                    cv::Mat imgRight = cv::Mat(distorted_height, distorted_width, Distored_img_CV_Type, distorted_frame_left.get()).clone();
                    cv::cvtColor(imgLeft, imgLeft, Distored_img_CV_Code);
                    cv::cvtColor(imgRight, imgRight, Distored_img_CV_Code);
                    cv::imshow("Callback Distorted Left", imgLeft);
                    cv::imshow("Callback Distorted Right", imgRight);
                    //fprintf(stderr, "%f %f %f\n", pose_l[12], pose_l[13], pose_l[14]);
                    //fprintf(stderr, "%f %f %f\n", pose_r[12], pose_r[13], pose_r[14]);
                    cv::waitKey(1);
                }
                if (g_streaming_control.CheckOption(StreamingControl::SHOW_UNDISTORTED)) {
                    auto CV_Type = undistorted_channel == 3 ? CV_8UC3 : CV_8UC4;
                    auto CV_Code = undistorted_channel == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
                    cv::Mat imgLeft = cv::Mat(undistorted_height, undistorted_width, CV_Type, undistorted_frame_left.get()).clone();
                    cv::Mat imgRight = cv::Mat(undistorted_height, undistorted_width, CV_Type, undistorted_frame_left.get()).clone();
                    cv::cvtColor(imgLeft, imgLeft, CV_Code);
                    cv::cvtColor(imgRight, imgRight, CV_Code);
                    cv::imshow("Callback Undistorted Left", imgLeft);
                    cv::imshow("Callback Undistorted Right", imgRight);
                    //fprintf(stderr, "%f %f %f\n", pose_l[12], pose_l[13], pose_l[14]);
                    //fprintf(stderr, "%f %f %f\n", pose_r[12], pose_r[13], pose_r[14]);
                    cv::waitKey(1);
                }
            }

        }
#pragma endregion SHOW_SEETROUGH
#pragma region SHOW_DEPTH
        if (g_streaming_control.CheckOption(StreamingControl::SHOW_DEPTH))
        {
            int res = ViveSR::FAILED;
            {
                std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                res = ViveSR_GetModuleData(ID_DEPTH, depth_element.data(), depth_count);
            }
            if (res == ViveSR::WORK) {
                auto CV_Type = DISTORTED_IMAGE_C == 3 ? CV_8UC3 : CV_8UC4;
                auto CV_Code = DISTORTED_IMAGE_C == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
                cv::Mat depth = cv::Mat(DEPTH_DATA_H, DEPTH_DATA_W, CV_32FC1, depth_map.get()).clone();
                cv::Mat img = cv::Mat(DEPTH_DATA_H, DEPTH_DATA_W, CV_Type, left_frame.get()).clone();
                cv::cvtColor(img, img, CV_Code);
                //
                cv::Mat show = cv::Mat(depth_img_height, depth_img_width, CV_8UC3);
                double min, max;
                cv::threshold(depth, depth, 0, 255, CV_THRESH_TOZERO);
                cv::minMaxIdx(depth, &min, &max);
                for (int i = 0; i < depth_img_width; i++) {
                    for (int j = 0; j < depth_img_height; j++) {
                        show.at<cv::Vec3b>(j, i) = (cv::Vec3b)(GetColour(depth.at<float>(j, i) / 255, 0, 1) * 255);
                    }
                }
                int center_x = depth_img_width / 2, center_y = depth_img_height / 2, rect = 5;
                float ROI = MIN(CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_FLEN_0], CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_FLEN_1]) * tan(35 * 3.14159 / 180);
                float sum = 0, avg = 0, distance;
                int negitive = 0, posivte_inf = 0;
                for (int w = depth_img_width / 2 - rect; w < depth_img_width / 2 + rect; w++) {
                    for (int h = depth_img_height / 2 - rect; h < depth_img_height / 2 + rect; h++) {
                        if (depth_map[w + depth_img_width * h] >= DBL_MAX)
                            posivte_inf++;
                        else if (depth_map[w + depth_img_width * h] < 0.0)
                            negitive++;
                        else
                            sum += depth_map[w + depth_img_width * h];
                    }
                }
                if (rect * rect * 4 - negitive - posivte_inf > 0)
                    avg = sum / (rect * rect * 4 - negitive - posivte_inf);
                else
                    avg = 0;
                if (avg != 0)
                    distance = avg;
                else
                    distance = 0;

                if (avg < 0.0)
                    printf(" avg  %f,   sum    %f,    num    %d,  negitive   %d,   inf  %d\n", avg, sum, rect * rect * 4 - negitive - posivte_inf, negitive, posivte_inf);

                cv::rectangle(show, cv::Point(center_x - ROI, center_y - ROI),
                    cv::Point(center_x + ROI, center_y + ROI), cv::Scalar(255, 255, 255), 2, 8);
                cv::rectangle(show, cv::Point(center_x - 5, center_y - 5),
                    cv::Point(center_x + 5, center_y + 5), cv::Scalar(255, 255, 255), 2, 8);

                cv::rectangle(img, cv::Point(center_x - ROI, center_y - ROI),
                    cv::Point(center_x + ROI, center_y + ROI), cv::Scalar(255, 255, 255), 2, 8);
                cv::rectangle(img, cv::Point(center_x - 5, center_y - 5),
                    cv::Point(center_x + 5, center_y + 5), cv::Scalar(255, 255, 255), 2, 8);

                char text[100];
                sprintf(text, "%.2f cm", distance);
                cv::putText(show, cv::String(text), cv::Point(10, 55), 0, 2, cv::Scalar(255, 255, 255), 3);
                cv::vconcat(show, img_colorbar, show);
                cv::imshow("Callback Depth", show);
                cv::imshow("Callback Depth-Color", img);
                cv::waitKey(1);
            }
        }
#pragma endregion SHOW_DEPTH
    }
}

