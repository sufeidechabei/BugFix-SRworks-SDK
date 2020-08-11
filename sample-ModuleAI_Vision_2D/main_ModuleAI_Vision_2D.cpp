//========= Copyright 2017, HTC Corporation. All rights reserved. ===========
#include <opencv.hpp>
#include <Windows.h>

#include "ViveSR_Enums.h"
#include "ViveSR_API_Enums.h"
#include "ViveSR_Client.h"
#include "ViveSR_Structs.h"
#include "ViveSR_PassThroughEnums.h"
#include "ViveSR_AISceneEnum.h"
#include <thread>
#include <memory>
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

#define STR_REFINETYPE(x) (x==ViveSR::AIVision::SceneLabelRefineType::REFINE_NONE ? "REFINE_NONE" \
								: x==ViveSR::AIVision::SceneLabelRefineType::REFINE_LARGE_HOMOGENEOUS_REGION ? "REFINE_LARGE_HOMOGENEOUS_REGION" \
								: "REFINE_SMALL_HOMOGENEOUS_REGION" )

#define REFINETYPESWITCH(x) (x==ViveSR::AIVision::SceneLabelRefineType::REFINE_NONE ? ViveSR::AIVision::SceneLabelRefineType::REFINE_LARGE_HOMOGENEOUS_REGION \
								: x==ViveSR::AIVision::SceneLabelRefineType::REFINE_LARGE_HOMOGENEOUS_REGION ? ViveSR::AIVision::SceneLabelRefineType::REFINE_SMALL_HOMOGENEOUS_REGION \
								: ViveSR::AIVision::SceneLabelRefineType::REFINE_NONE )


double CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_PARAMS_MAX];

cv::Mat ColorMap;
bool EnableKeyboardInput = true;

void Loop();

void CreateColorMap();

struct StreamingControl
{
    enum ShowOption {
        SHOW_DISTORTED,
        SHOW_UNDISTORTED,
        SHOW_AIVISION,
        SHOW_MAX
    };
    inline bool CheckOption(ShowOption option) {
        return 0 != (option_&(1 << option));
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
static StreamingControl g_streaming_control;
void StreamingShowImage();
int ID_PASSTHROUGH, ID_AI_VISION;
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
ParamInfo gCameraQualityParamInfo[17];

int main()
{
    std::thread* show_img_thread = nullptr;
	CreateColorMap();
	{
        int flag = 0;
        int res = ViveSR::Error::INVALID_INPUT;
        if (ViveSR_GetContextInfo(NULL) != ViveSR::Error::WORK) {
            res = ViveSR_CreateContext("", 0);
        }
		if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_Initial failed %d\n", res); goto stop; }
		else
		{
			
			res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_PASS_THROUGH, &ID_PASSTHROUGH);
			fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_PASSTHROUGH, res != ViveSR::Error::WORK ? "failed" : "success", res);
			if (res != ViveSR::Error::WORK) goto stop;

			res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_AI_SCENE, &ID_AI_VISION);
			fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_AI_VISION, res != ViveSR::Error::WORK ? "failed" : "success", res);
			if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_InitialModule(ID_PASSTHROUGH);
            ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::DISTORT_GPU_TO_CPU_ENABLE, true);
            if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_InitialModule(ID_AI_VISION);
            if (res != ViveSR::Error::WORK) goto stop;

			Sleep(1000);

            for (int i = ViveSR::PassThrough::CAMERA_Param::CAMERA_FCX_0; i < ViveSR::PassThrough::CAMERA_Param::CAMERA_PARAMS_MAX; i++)
            {
                res = ViveSR_GetParameterDouble(ID_PASSTHROUGH, i, &(CameraParameters[i]));
                if (res == ViveSR::Error::FAILED)
                    goto stop;
            }

			ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::VR_INIT, true);
			ViveSR_SetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::Param::VR_INIT_TYPE, ViveSR::PassThrough::InitType::SCENE);

			res = ViveSR_StartModule(ID_PASSTHROUGH);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", 0, res); goto stop; }
			res = ViveSR_StartModule(ID_AI_VISION);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", 1, res); goto stop; }

			res = ViveSR_LinkModule(ID_PASSTHROUGH, ID_AI_VISION, ViveSR::LinkType::ACTIVE);
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
        error = ViveSR_StopModule(ID_AI_VISION);
        if (error == ViveSR::Error::WORK) { printf("Successfully release ai vision engine.\n"); }
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
            std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
			if (str == '`') break;
            else if (str == '1')
            {
                g_streaming_control.EnableOption(StreamingControl::SHOW_DISTORTED);
                fprintf(stderr, "Show distorted image\n");
            }
            else if (str == '2')
            {
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
                int img_crop_w, img_crop_h;
                int img_label_w, img_label_h;
                int img_ch, label_number;
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_Crop_H, &img_crop_h);
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_Crop_W, &img_crop_w);
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Label_H, &img_label_h);
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Label_W, &img_label_w);
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_CH, &img_ch);
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_CH, &img_ch);
                ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_CH, &img_ch);
                fprintf(stderr, "Image width :%d, height: %d, channel : %d\n", img_crop_w, img_crop_h, img_ch);
                fprintf(stderr, "Label width :%d, height: %d, channel : %d\n", img_label_w, img_label_h, 1);
                fprintf(stderr, "ProbabiliryTable width :%d, height: %d, channel : %d\n", img_label_w, img_label_h, img_ch);
			}
            else if (str == '6') {
                g_streaming_control.EnableOption(StreamingControl::SHOW_AIVISION);
                fprintf(stderr, "Stop showing AI vision image\n");
            }
            else if (str == '7') {
                g_streaming_control.DisableOption(StreamingControl::SHOW_AIVISION);
                fprintf(stderr, "Stop showing AI vision image\n");
            }
			else if (str == 'r') {
				static int refine_type = ViveSR::AIVision::SceneLabelRefineType::REFINE_NONE;
				res = ViveSR_SetParameterInt(ID_AI_VISION, ViveSR::AIVision::REFINE_TYPE, refine_type);
				fprintf(stderr,"[AI_VISION] refine type : %s(%s)\n", STR_REFINETYPE(refine_type), res == ViveSR::Error::WORK ? "OK" : "FAIL");
				refine_type = REFINETYPESWITCH(refine_type);
			}
		}
	}
}
void StreamingShowImage()
{
#pragma region ALLOCATE_SEE_THROUGH
    int distorted_width, distorted_height, distorted_channel;
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_WIDTH, &distorted_width);
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_HEIGHT, &distorted_height);
    ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_CHANNEL, &distorted_channel);
    auto Distored_img_CV_Type = distorted_channel == 3 ? CV_8UC3 : CV_8UC4;
    auto Distored_img_CV_Code = distorted_channel == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
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
#pragma region ALLOCATE_AI_VISION
    double blendingAlpha = 0.3;
    int img_crop_w, img_crop_h;
    int img_label_w, img_label_h;
    int img_ch;
    ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_Crop_H, &img_crop_h);
    ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_Crop_W, &img_crop_w);
    ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Label_H, &img_label_h);
    ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Label_W, &img_label_w);
    ViveSR_GetParameterInt(ID_AI_VISION, ViveSR::AIVision::Param::Img_CH, &img_ch);
    auto ptr_frame_pri = std::make_unique<char[]>(img_crop_w*img_crop_h*img_ch);
    auto ptr_label_pri = std::make_unique<char[]>(img_label_w*img_label_h);
    const int ai_vision_length = 2;
    static std::vector<ViveSR::MemoryElement> ai_vision(ai_vision_length);
    ai_vision[0].ptr = ptr_frame_pri.get();
    ai_vision[0].mask = ViveSR::AIVision::OutputMask::FRAME_LEFT;
    ai_vision[1].ptr = ptr_label_pri.get();
    ai_vision[1].mask = ViveSR::AIVision::OutputMask::LABEL_LEFT;
#pragma endregion
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
#pragma region SHOW_AI_VISION
        if (g_streaming_control.CheckOption(StreamingControl::SHOW_AIVISION)) {
            int res = ViveSR::FAILED;
            {
                std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
                res = ViveSR_GetModuleData(ID_AI_VISION, ai_vision.data(), ai_vision_length);
            }
            if (res == ViveSR::WORK) {
                auto CV_Code = CV_RGBA2BGR;
                auto CV_Type = img_ch == 3 ? CV_8UC3 : CV_8UC4;
                cv::Mat img_frame = cv::Mat(img_crop_h, img_crop_w, CV_8UC4, ptr_frame_pri.get()).clone();
                cv::Mat img_label_frame = cv::Mat(img_label_h, img_label_w, CV_8UC1, ptr_label_pri.get()).clone();
                // colorize
                std::vector<cv::Mat>channels;
                for (int i = 0; i < 3; i++) {
                    channels.push_back(img_label_frame);
                }
                cv::Mat img_lebalcolor;
                cv::Mat merImg;
                cv::merge(channels, merImg);
                cv::LUT(merImg, ColorMap, img_lebalcolor);

                // blending
                cv::Mat img_blended = cv::Mat(img_label_h, img_label_w, CV_8UC3);
                cv::cvtColor(img_frame, img_frame, CV_Code);
                cv::addWeighted(img_frame, blendingAlpha, img_lebalcolor, (1 - blendingAlpha), 0.0, img_blended);
                cv::imshow("Callback Scene Frame", img_frame);
                cv::imshow("Callback Scene Label", img_lebalcolor);
                cv::imshow("Callback Scene Label from pro", img_blended);
                cv::waitKey(1);
            }
        }
        
#pragma endregion

    }
}


void CreateColorMap()
{
	ColorMap = cv::Mat::zeros(cv::Size(1, 256), CV_8UC3);

	ColorMap.at<cv::Vec3b>(cv::Point(0, 0)) = cv::Vec3b(0, 0, 0);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 1)) = cv::Vec3b(3, 200, 4);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 2)) = cv::Vec3b(140, 255, 143);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 3)) = cv::Vec3b(205, 102, 11);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 4)) = cv::Vec3b(255, 31, 0);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 5)) = cv::Vec3b(120, 120, 180);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 6)) = cv::Vec3b(7, 51, 255);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 7)) = cv::Vec3b(51, 6, 255);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 8)) = cv::Vec3b(0, 173, 255);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 9)) = cv::Vec3b(20, 150, 160);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 10)) = cv::Vec3b(230, 230, 230);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 11)) = cv::Vec3b(0, 235, 255);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 12)) = cv::Vec3b(120, 120, 120);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 13)) = cv::Vec3b(255, 224, 112);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 14)) = cv::Vec3b(50, 50, 80);
	ColorMap.at<cv::Vec3b>(cv::Point(0, 15)) = cv::Vec3b(255, 0, 255);

}