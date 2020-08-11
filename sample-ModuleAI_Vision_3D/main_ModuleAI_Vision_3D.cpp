//========= Copyright 2017, HTC Corporation. All rights reserved. ===========
#include <opencv.hpp>
#include <Windows.h>
#include <thread>
#include "ViveSR_Enums.h"
#include "ViveSR_API_Enums.h"
#include "ViveSR_Client.h"
#include "ViveSR_Structs.h"
#include "ViveSR_PassThroughEnums.h"
#include "ViveSR_DepthEnums.h"
#include "ViveSR_RigidReconstructionEnums.h"
#include <memory>
#include <chrono>
#include <mutex>

#include "viewer.h"

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

bool EnableKeyboardInput = true;
bool EnableEffect = false;
bool EnableDepth = true;

bool LiveExtraction = true;
bool ShowReconsInfo = true;

bool EnableSector = false;
float SectorSize = 0.3f;

float adpMaxGrid, adpMinGrid, adpThres;
float renderWhiteColor = 0;

void Loop();
void UpdateViewer();
void GetDepthParallelRun();

int ID_PASSTHROUGH, ID_DEPTH, ID_RIGID_RECONSTRUCTION;

PointCloudViewer* viewer = nullptr;
std::thread* viewerThread = nullptr;
extern int semanticExportNetProgress;
struct StreamingControl
{
    enum ShowOption {
        SHOW_DEPTH,
        SHOW_RIGID_RECONSTRUCTION,
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
    float pose[16];
    std::unique_ptr<char[]> left_frame;//nullptr
    std::mutex steaming_mutex_;
};
static StreamingControl g_streaming_control;

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

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_CLOSE_EVENT:

        if (viewer)
        {
            viewerThread->join();
            delete viewerThread;
        }
        auto error = ViveSR_StopModule(ID_PASSTHROUGH);
        if (error == ViveSR::Error::WORK) { printf("Successfully release see through engine.\n"); }
        else printf("Fail to release see through engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        error = ViveSR_StopModule(ID_DEPTH);
        if (error == ViveSR::Error::WORK) { printf("Successfully release depth engine.\n"); }
        else printf("Fail to release depth engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        error = ViveSR_StopModule(ID_RIGID_RECONSTRUCTION);
        if (error == ViveSR::Error::WORK) { printf("Successfully release reconstruction engine.\n"); }
        else printf("Fail to release depth engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        fprintf(stderr, "Exit. Please press any key to close it.\n");
        return TRUE;
    }

    return TRUE;
}

int main()
{
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    CreateColorbarImage();
    std::thread* stream_thread = nullptr;
    {
        int flag = 0;
        int res = ViveSR::Error::INITIAL_FAILED;
        if (ViveSR_GetContextInfo(NULL) != ViveSR::Error::WORK) {
            res = ViveSR_CreateContext("", 0);
        }
        if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_Initial failed %d\n", res); goto stop; }
        else
        {
            res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_PASS_THROUGH, &ID_PASSTHROUGH);
            fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_PASSTHROUGH, res != ViveSR::Error::WORK ? "failed" : "success", res);
            if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_DEPTH, &ID_DEPTH);
            fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_DEPTH, res != ViveSR::Error::WORK ? "failed" : "success", res);
            if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_RIGID_RECONSTRUCTION, &ID_RIGID_RECONSTRUCTION);
            fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_RIGID_RECONSTRUCTION, res != ViveSR::Error::WORK ? "failed" : "success", res);
            if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_InitialModule(ID_PASSTHROUGH);
            ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::DISTORT_GPU_TO_CPU_ENABLE, true);
            if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_InitialModule(ID_DEPTH);
            if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_InitialModule(ID_RIGID_RECONSTRUCTION);
            if (res != ViveSR::Error::WORK) goto stop;

            Sleep(1000);

            for (int i = ViveSR::PassThrough::CAMERA_Param::CAMERA_FCX_0; i < ViveSR::PassThrough::CAMERA_Param::CAMERA_PARAMS_MAX; i++)
            {
                res = ViveSR_GetParameterDouble(ID_PASSTHROUGH, i, &(CameraParameters[i]));
                if (res == ViveSR::Error::FAILED)
                    goto stop;
            }

            res = ViveSR_StartModule(ID_PASSTHROUGH);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule See through failed(%d)\n", res); goto stop; }
            res = ViveSR_StartModule(ID_DEPTH);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule depth failed(%d)\n", res); goto stop; }
            res = ViveSR_StartModule(ID_RIGID_RECONSTRUCTION);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule rigid reconstruction failed(%d)\n", res); goto stop; }

            res = ViveSR_LinkModule(ID_PASSTHROUGH, ID_DEPTH, ViveSR::LinkType::ACTIVE);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSRs_link See through to depth failed(%d)\n", res); goto stop; }
            res = ViveSR_LinkModule(ID_DEPTH, ID_RIGID_RECONSTRUCTION, ViveSR::LinkType::ACTIVE);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSRs_link depth to rigid reconstruction failed(%d)\n", res); goto stop; }

            if (res != ViveSR::Error::WORK) fprintf(stderr, "ViveSR_Start failed\n");
            else {
                g_streaming_control.streaming_ = true;
                stream_thread = new std::thread(GetDepthParallelRun);
                Loop();
            }
        }

    stop:
        if (viewer)
        {
            viewerThread->join();
            delete viewerThread;
        }
        auto error = ViveSR_StopModule(ID_PASSTHROUGH);
        if (error == ViveSR::Error::WORK) { printf("Successfully release see through engine.\n"); }
        else printf("Fail to release see through engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        error = ViveSR_StopModule(ID_DEPTH);
        if (error == ViveSR::Error::WORK) { printf("Successfully release depth engine.\n"); }
        else printf("Fail to release depth engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        error = ViveSR_StopModule(ID_RIGID_RECONSTRUCTION);
        if (error == ViveSR::Error::WORK) { printf("Successfully release reconstruction engine.\n"); }
        else printf("Fail to release depth engine. please refer the code %d of ViveSR::Error.\n", error);
        //
        fprintf(stderr, "Exit. Please press any key to close it.\n");
        getchar();
    }
    return 0;
}

void Loop() {
    int res = ViveSR::Error::FAILED;

    while (true)
    {
        if (EnableKeyboardInput) {
            fprintf(stderr, "wait for key event : \n");
            char str = getchar();
            std::lock_guard<std::mutex> guard(g_streaming_control.steaming_mutex_);
            if (str == '`') break;
            if (str == 'j') {
                // denoise method, range : 1~5(default)
                int value = 5;
                res = ViveSR_SetParameterInt(ID_DEPTH, ViveSR::Depth::Param::DENOISE_MEDIAN_FILTER, value);
                fprintf(stderr, "Depth change DENOISE_M to %d(%d)\n", value, res);
            }
            if (str == 'k') {
                // denoise method, range : 1~7(default 3), set value 0 to turn-off
                int value = 7;
                res = ViveSR_SetParameterInt(ID_DEPTH, ViveSR::Depth::Param::DENOISE_GUIDED_FILTER, value);
                fprintf(stderr, "Depth change DENOISE_G to %d(%d)\n", value, res);
            }
            if (str == 'l') {
                // depth confidence threshold, default 0.05, set thr value 0 to turn off
                double threshold = 1.0;
                res = ViveSR_SetParameterDouble(ID_DEPTH, ViveSR::Depth::Param::CONFIDENCE_THRESHOLD, threshold);
                fprintf(stderr, "Depth change CONFIDENCE_TH %f(%d)\n", threshold, res);
            }
            if (str == 'b') {
                // enhance local area depth confidence(suitable for low texture target, like wall, floor, desk ...)
                // default turn-off
                static bool flag_enable_depth_refine = false;
                flag_enable_depth_refine = !flag_enable_depth_refine;
                res = ViveSR_SetParameterBool(ID_DEPTH, ViveSR::Depth::Cmd::ENABLE_REFINEMENT, flag_enable_depth_refine);
                fprintf(stderr, "Send Command : %s DEPTH REFINEMENT(%d)\n", flag_enable_depth_refine ? "ENABLE" : "DISABLE", res);
            }
            if (str == 'n') {
                // enhance depth accuracy in complex environment
                // default turn-off
                static bool flag_enable_edge_enhance = true;
                flag_enable_edge_enhance = !flag_enable_edge_enhance;
                res = ViveSR_SetParameterBool(ID_DEPTH, ViveSR::Depth::Cmd::ENABLE_EDGE_ENHANCE, flag_enable_edge_enhance);
                fprintf(stderr, "Send Command : %s DEPTH EDGE ENHANCE(%d)\n", flag_enable_edge_enhance ? "ENABLE" : "DISABLE", res);
            }
            if (str == 'm') {
                // change depth case 
                // CLOSE_RANGE : available depth range is more wide, but higher cpu and gpu loading
                static bool flag = true;
                int _case = flag ? ViveSR::Depth::DepthCase::CLOSE_RANGE : ViveSR::Depth::DepthCase::DEFAULT;
                res = ViveSR_SetParameterBool(ID_DEPTH, ViveSR::Depth::Cmd::CHANGE_DEPTH_CASE, _case);
                fprintf(stderr, "Send Command : Change Depth case to : %s(%d)\n", flag ? "CLOSE_RANGE" : "DEFAULT", res);
                flag = !flag;
            }
            else if (str == '6')
                g_streaming_control.EnableOption(StreamingControl::SHOW_DEPTH);
            //fprintf(stderr, "ViveSR_RegisterDepthDataCallback %d\n", ViveSR_RegisterCallback(ID_DEPTH, ViveSR::Depth::Callback::BASIC, GetDepthCallback));
            else if (str == '7')
                g_streaming_control.EnableOption(StreamingControl::SHOW_DEPTH);
            //fprintf(stderr, "ViveSR_UnregisterDepthDataCallback %d\n", ViveSR_UnregisterCallback(ID_DEPTH, ViveSR::Depth::Callback::BASIC, GetDepthCallback));
            else if (str == '9') {
                EnableDepth = !EnableDepth;
                fprintf(stderr, "EnableDepth %s\n", EnableDepth ? "On" : "Off");
                fprintf(stderr, "ViveSR_ChangeModuleLinkStatus %d\n", ViveSR_LinkModule(ID_PASSTHROUGH, ID_DEPTH, EnableDepth ? ViveSR::LinkType::ACTIVE : ViveSR::LinkType::NONE));
            }
            else if (str == 'q') {
                if (!viewerThread)
                    viewerThread = new std::thread(&UpdateViewer);
            }
            else if (str == 'w') {
                res = ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::START, true);	// send start cmd
                fprintf(stderr, "Send Command : START RIGID RECONSTRUCTION(%d)\n", res);
            }
            else if (str == 'e') {
                res = ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::STOP, true);		// send stop cmd
                fprintf(stderr, "Send Command : STOP RIGID RECONSTRUCTION(%d)\n", res);
            }
            // switch between: lite-point-cloud (only fov) / full-point-cloud / live-adaptive-mesh
            else if (str == 'r') {
                static int mode = 0;
                mode = (mode + 1) % 3;
                bool liveRCV, liveAdaptive, livePC;
                if (mode == 0) { liveRCV = true;	liveAdaptive = false;	livePC = false;	renderWhiteColor = 0.0f; }
                else if (mode == 1) { liveRCV = false;	liveAdaptive = true;	livePC = false;	renderWhiteColor = 1.0f; }
                else { liveRCV = false;	liveAdaptive = false;	livePC = true;	renderWhiteColor = 0.0f; }
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::LITE_POINT_CLOUD_MODE, liveRCV);
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::LIVE_ADAPTIVE_MODE, liveAdaptive);
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::FULL_POINT_CLOUD_MODE, livePC);
                fprintf(stderr, "Reconstruction data : %s mode\n", liveRCV ? "LITE_POINT_CLOUD" : (liveAdaptive ? "LIVE_ADAPTIVE" : "FULL_POINT_CLOUD"));

                EnableSector = liveRCV ? false : true;
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, EnableSector);
                fprintf(stderr, "ENABLE_SECTOR_GROUPING %d\n", EnableSector ? 0 : -1);
            }
            // adjust the live adaptive mesh: lowest quality
            else if (str == 'a' || str == 'z') {
                ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ADAPTIVE_MAX_GRID, &adpMaxGrid);
                adpMaxGrid *= (str == 'a') ? 2.0f : 0.5f;
                ViveSR_SetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ADAPTIVE_MAX_GRID, adpMaxGrid);
                fprintf(stderr, "max grid size : %f\n", adpMaxGrid);
            }
            // adjust the live adaptive mesh: finest quality
            else if (str == 's' || str == 'x') {
                ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ADAPTIVE_MIN_GRID, &adpMinGrid);
                adpMinGrid *= (str == 's') ? 2.0f : 0.5f;
                ViveSR_SetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ADAPTIVE_MIN_GRID, adpMinGrid);
                fprintf(stderr, "min grid size : %f\n", adpMinGrid);
            }
            // adjust the live adaptive mesh: adaptive-error-threshold
            else if (str == 'd' || str == 'c') {
                ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ADAPTIVE_ERROR_THRES, &adpThres);
                adpThres += (str == 'd') ? 0.02f : -0.02f;
                ViveSR_SetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ADAPTIVE_ERROR_THRES, adpThres);
                fprintf(stderr, "error threshold : %f\n", adpThres);
            }
            // enable / disable live-extraction, if is disable, reconstruction engine will not do live extraction
            else if (str == 't') {
                LiveExtraction = !LiveExtraction;
                res = ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::EXTRACT_POINT_CLOUD, LiveExtraction);		// toggle on-line point cloud extraction
                fprintf(stderr, "Send Command : %s EXTRACT POINT CLOUD(%d)\n", LiveExtraction ? "START" : "STOP", res);
            }
            // save to obj (VivePro.obj)
            else if (str == 'y') {
                char obj_name[] = "VivePro";
                res = ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::EXPORT_MODEL_RIGHT_HAND, obj_name);			// call save model to model name
                fprintf(stderr, "Send Command : Save Reconstruction Data to %s(%d)\n", obj_name, res);
            }
            else if (str == 'y') {
                char obj_name[] = "VivePro";
                res = ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::EXPORT_MODEL_RIGHT_HAND, obj_name);			// call save model to model name
                fprintf(stderr, "Send Command : Save Reconstruction Data to %s(%d)\n", obj_name, res);
            }
            else if (str == 'u') {
                bool bEnableSectorGrouping, bEnableFrustumCulling;
                ViveSR_GetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, &bEnableSectorGrouping);
                ViveSR_GetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, &bEnableFrustumCulling);

                // scene understanding is incompatible with these following functionalities
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, false);
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, false);
                // get progress bar from event listener
                std::thread thd([&, bEnableSectorGrouping, bEnableFrustumCulling] {
                    semanticExportNetProgress = 0; // reset before querying progress 
                    do {
                        fprintf(stderr, "%s", std::string("Processing: " + std::to_string(semanticExportNetProgress) + " %\n").c_str());
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    } while (semanticExportNetProgress < 100);
                    fprintf(stderr, "Export complete\n");
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, bEnableSectorGrouping);
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, bEnableFrustumCulling);
                });
                thd.detach();
                fprintf(stderr, "Semantic scene understanding complete\n");
            }
            else if (str == 'f') {
                EnableSector = !EnableSector;
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, EnableSector);

                fprintf(stderr, "ENABLE_SECTOR_GROUPING %d\n", EnableSector ? 0 : -1);
                if (EnableSector) {
                    float sector_size;
                    int sector_num;
                    ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SECTOR_SIZE, &sector_size);
                    ViveSR_GetParameterInt(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SECTOR_NUM_PER_SIDE, &sector_num);
                    fprintf(stderr, "SECTOR_SIZE %f\n", sector_size);
                    fprintf(stderr, "SECTOR_NUM_PER_SIDE %d\n", sector_num);
                }
            }
        }
    }
}
void GetDepthParallelRun()
{
#pragma region ALLOCATE_DEPTH
    const static int DEPTH_ELEM_LENGTH = ViveSR::Depth::OutputMask::OUTPUT_MASK_MAX;
    int depth_img_width, depth_img_height, depth_img_channel, depth_color_img_channel;
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_WIDTH, &depth_img_width);
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_HEIGHT, &depth_img_height);
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_CHAANEL_1, &depth_img_channel);
    ViveSR_GetParameterInt(ID_DEPTH, ViveSR::Depth::OUTPUT_CHAANEL_0, &depth_color_img_channel);
    static std::vector<ViveSR::MemoryElement> depth_element(DEPTH_ELEM_LENGTH);
    g_streaming_control.left_frame = std::make_unique<char[]>(depth_img_height*depth_img_width*depth_color_img_channel);
    auto depth_map = std::make_unique<float[]>(depth_img_height*depth_img_width*depth_img_channel);
    unsigned int frame_seq;
    unsigned int time_stp;

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

    auto camera_params = std::make_unique<char[]>(1032);
    void* depth_ptrs[ViveSR::Depth::OutputMask::OUTPUT_MASK_MAX]{
        g_streaming_control.left_frame.get(),
        depth_map.get(),
        &frame_seq,
        &time_stp,
        g_streaming_control.pose,
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
                cv::Mat img = cv::Mat(DEPTH_DATA_H, DEPTH_DATA_W, CV_Type, g_streaming_control.left_frame.get()).clone();
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

void UpdateViewer()
{
    viewer = new PointCloudViewer(CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_FLEN_0]);
    const int rigid_reconstruction_length = ViveSR::RigidReconstruction::OutputMask::MODEL_CHUNK_IDX + 1;
    static std::vector<ViveSR::MemoryElement> rigid_reconstruction_element(rigid_reconstruction_length);
#pragma region ALLOCATE_RECONSTRCUTION
    unsigned int frame_seq;	        // sizeof(unsigned int)
    auto posemtx44 = std::make_unique<float[]>(16);               // sizeof(float) * 16
    int num_vertices;	            // sizeof(int)
    int bytepervert;	            // sizeof(int)
    auto vertices = std::make_unique<float[]>(8 * 2500000);	            // sizeof(float) * 8 * 2500000
    int num_indices;	            // sizeof(int)
    auto indices = std::make_unique<int[]>(2500000);	                // sizeof(int) * 2500000
    int cldtype;	                // sizeof(int)
    unsigned int collidernum;	    // sizeof(unsigned int)
    auto cld_num_verts = std::make_unique<unsigned int[]>(200);	// sizeof(unsigned int) * 200
    auto cld_numidx = std::make_unique<unsigned int[]>(200);	    // sizeof(unsigned int) * 200
    auto cld_vertices = std::make_unique<float[]>(3 * 50000);	        // sizeof(float) * 3 * 50000
    auto cld_indices = std::make_unique<int[]>(100000);	            // sizeof(int) * 100000
    int sector_num;                 // sizeof(int)
    auto sector_id_list = std::make_unique<int[]>(1000000);            // sizeof(int) * 1000000
    auto sector_vert_num = std::make_unique<int[]>(1000000);           // sizeof(int) * 1000000
    auto sector_idx_num = std::make_unique<int[]>(1000000);            // sizeof(int) * 1000000
    int model_chunk_num;            // sizeof(int)
    int model_chunk_idx;            // sizeof(int)
    void* ptrs[rigid_reconstruction_length]{
        &frame_seq,	        // sizeof(unsigned int)
        posemtx44.get(),            // sizeof(float) * 16
        &num_vertices,	    // sizeof(int)
        &bytepervert,	        // sizeof(int)
        vertices.get(),	            // sizeof(float) * 8 * 2500000
        &num_indices,	        // sizeof(int)
        indices.get(),	            // sizeof(int) * 2500000
        &cldtype,	            // sizeof(int)
        &collidernum,	        // sizeof(unsigned int)
        cld_num_verts.get(),	    // sizeof(unsigned int) * 200
        cld_numidx.get(),	        // sizeof(unsigned int) * 200
        cld_vertices.get(),	        // sizeof(float) * 3 * 50000
        cld_indices.get(),	        // sizeof(int) * 100000
        &sector_num,          // sizeof(int)
        sector_id_list.get(),       // sizeof(int) * 1000000
        sector_vert_num.get(),      // sizeof(int) * 1000000
        sector_idx_num.get(),       // sizeof(int) * 1000000
        &model_chunk_num,     // sizeof(int)
        &model_chunk_idx,     // sizeof(int)
    };
    int nCount = 0;
    for (int i = 0; i < rigid_reconstruction_length; ++i)
    {
        if (ptrs[i])
        {
            rigid_reconstruction_element[nCount].mask = i;
            rigid_reconstruction_element[nCount].ptr = ptrs[i];
            nCount++;
        }
    }
    float pSizeScale = 0.f;
    float pointRenderSize;
    {
        std::lock_guard<std::mutex>(g_streaming_control.steaming_mutex_);
        ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::POINTCLOUD_POINTSIZE, &pointRenderSize);
    }
    pSizeScale = (pointRenderSize * 100.f);
#pragma endregion ALLOCATE_RECONSTRCUTION
    while (!viewer->shutDown)
    {
        static int lastNumVerts = -1;
        int result = ViveSR::Error::FAILED;
        {
            std::lock_guard<std::mutex>(g_streaming_control.steaming_mutex_);
            result = ViveSR_GetModuleData(ID_RIGID_RECONSTRUCTION, rigid_reconstruction_element.data(), nCount);
        }
        if (result == ViveSR::Error::WORK)
        {
            int numVert = num_vertices;

            if (lastNumVerts != numVert)
            {
                lastNumVerts = numVert;
                viewer->UpdateGeometry(
                    (float*)vertices.get(),
                    numVert,
                    bytepervert,
                    sector_num,
                    sector_id_list.get(),
                    sector_vert_num.get(),
                    sector_idx_num.get(),
                    indices.get(),
                    num_indices,
                    EnableSector
                );
            }
            if (g_streaming_control.left_frame)
            {
                std::lock_guard<std::mutex>(g_streaming_control.steaming_mutex_);
                viewer->UpdateBackground(g_streaming_control.left_frame.get(), DEPTH_DATA_W, DEPTH_DATA_H);
                viewer->UpdateCamera((float*)(g_streaming_control.pose), false);
            }
        }
        viewer->Draw(pSizeScale, renderWhiteColor);
    }

    delete viewer;
}
