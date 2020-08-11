//========= Copyright 2017, HTC Corporation. All rights reserved. ===========
#include "headers.h"
#define GUI_VIEWER

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

AppViewer viewer;
cv::Mat img_colorbar, img_jet;
double CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_PARAMS_MAX];
//AI_VisionModuleInfo moduleAI_VisionInfo;

bool EnableKeyboardInput = true;
bool EnableEffect = false;
int semanticStage = 0, semanticProgress = 0;


void Loop();

void TestGetFrameData(bool isDistorted);
void TestGetDepthData();
void TestGetRigidReconstructionData();
void UpdateSemanticProgress(int stage, int percentage);

int ID_PASSTHROUGH, ID_DEPTH, ID_RIGID_RECONSTRUCTION, ID_AI_VISION;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		viewer.StopProcess();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		return TRUE;
	}

	return TRUE;
}

#ifndef GUI_VIEWER
bool EnableDepth = true;

bool LiveExtraction = true;
bool ShowReconsInfo = true;

bool EnableSector = false;
float SectorSize = 0.3f;

float adpMaxGrid, adpMinGrid, adpThres;
float renderWhiteColor = 0;
void GetDistortedCallback(int key);
void GetUndistortedCallback(int key);
void GetDepthCallback(int key);
void Get3DViewerBackgroundCallback(int key);
void GetRigidReconstructionCallback(int key);

void TestGetFrameData(bool isDistorted);
void TestGetDepthData();
void TestGetRigidReconstructionData();
void ShowExportProgressCallback(int stage, int percentage);
void UpdateSemanticProgress(int stage, int percentage);
#endif

int main()
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
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
			if(res != ViveSR::Error::WORK) goto stop;

			res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_DEPTH, &ID_DEPTH);
			fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_DEPTH, res != ViveSR::Error::WORK ? "failed" : "success", res);
			if (res != ViveSR::Error::WORK) goto stop;
            
			res = ViveSR_CreateModule(ViveSR::ModuleType::ENGINE_RIGID_RECONSTRUCTION, &ID_RIGID_RECONSTRUCTION);
			fprintf(stderr, "ViveSR_CreateModule %d %s(%d)\n", ID_RIGID_RECONSTRUCTION, res != ViveSR::Error::WORK ? "failed" : "success", res);
			if (res != ViveSR::Error::WORK) goto stop;

            res = ViveSR_InitialModule(ID_PASSTHROUGH);
            ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::DISTORT_GPU_TO_CPU_ENABLE, true);
			res = ViveSR_InitialModule(ID_PASSTHROUGH);
			ViveSR_SetParameterBool(ID_PASSTHROUGH, ViveSR::PassThrough::Param::DEPTH_UNDISTORT_GPU_TO_CPU_ENABLE, true);
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

			ViveSR_SetParameterInt(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::CONFIG_QUALITY, 3);
			ViveSR_SetParameterInt(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::MESH_REFRESH_INTERVAL, 300);
			ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, true);
			ViveSR_SetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SECTOR_SIZE, 0.8);

			res = ViveSR_StartModule(ID_PASSTHROUGH);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", ID_PASSTHROUGH, res); goto stop; }
			res = ViveSR_StartModule(ID_DEPTH);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", ID_DEPTH, res); goto stop; }
            res = ViveSR_StartModule(ID_RIGID_RECONSTRUCTION);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSR_StartModule %d failed(%d)\n", ID_DEPTH, res); goto stop; }

			res = ViveSR_LinkModule(ID_PASSTHROUGH, ID_DEPTH, ViveSR::LinkType::ACTIVE);
			if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSRs_link %d to %d failed(%d)\n", ID_PASSTHROUGH, ID_DEPTH, res); goto stop; }

			res = ViveSR_LinkModule(ID_DEPTH, ID_RIGID_RECONSTRUCTION, ViveSR::LinkType::ACTIVE);
            if (res != ViveSR::Error::WORK) { fprintf(stderr, "ViveSRs_link %d to %d failed(%d)\n", ID_DEPTH, ID_RIGID_RECONSTRUCTION, res); goto stop; }


            viewer.Initialize(ID_PASSTHROUGH, ID_DEPTH, ID_RIGID_RECONSTRUCTION);
            viewer.SetProjection(CameraParameters[ViveSR::PassThrough::CAMERA_Param::CAMERA_FLEN_0]);
            Loop();
        }
	stop:
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
	}
	return 0;
}

void Loop() {
	int res = ViveSR::Error::FAILED;
	char str = 0;
#ifdef GUI_VIEWER
	viewer.Process();
#else
    void UpdateSemanticProgress(int stage, int perc)
    {
        if (stage == (int)ViveSR::RigidReconstruction::ExportStage::SCENE_UNDERSTANDING_PASS_1) semanticStage = 0;
        if (stage == (int)ViveSR::RigidReconstruction::ExportStage::SCENE_UNDERSTANDING_PASS_2) semanticStage = 1;

        semanticProgress = perc;
    }
	while (true)
	{
		if (EnableKeyboardInput) {
			if (str != '\n' && str != EOF) fprintf(stderr, "wait for key event : \n");
			str = getchar();

			if (str == '`') break;
			else if (str == '1')
				fprintf(stderr, "ViveSR_RegisterDistortedDataCallback %d\n", ViveSR_RegisterCallback(ID_PASSTHROUGH, ViveSR::PassThrough::Callback::BASIC, GetDistortedCallback));
			else if (str == '2')
				fprintf(stderr, "ViveSR_UnregisterDistortedDataCallback %d\n", ViveSR_UnregisterCallback(ID_PASSTHROUGH, ViveSR::PassThrough::Callback::BASIC, GetDistortedCallback));
			else if (str == '3')
				fprintf(stderr, "ViveSR_RegisterUndistortedDataCallback %d\n", ViveSR_RegisterCallback(ID_PASSTHROUGH, ViveSR::PassThrough::Callback::BASIC, GetUndistortedCallback));
			else if (str == '4')
				fprintf(stderr, "ViveSR_UnregisterUndistortedDataCallback %d\n", ViveSR_UnregisterCallback(ID_PASSTHROUGH, ViveSR::PassThrough::Callback::BASIC, GetUndistortedCallback));
			else if (str == '5')
			{
				int width, height, channel;
				ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_WIDTH, &width);
				ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_HEIGHT, &height);
				ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_CHANNEL, &channel);
				fprintf(stderr, "Distorted width :%d, height: %d, channel : %d\n", width, height, channel);
			}
			else if (str == '6')
				fprintf(stderr, "ViveSR_RegisterDepthDataCallback %d\n", ViveSR_RegisterCallback(ID_DEPTH, ViveSR::Depth::Callback::BASIC, GetDepthCallback));
			else if (str == '7')
				fprintf(stderr, "ViveSR_UnregisterDepthDataCallback %d\n", ViveSR_UnregisterCallback(ID_DEPTH, ViveSR::Depth::Callback::BASIC, GetDepthCallback));
			else if (str == '9') {
				EnableDepth = !EnableDepth;
				fprintf(stderr, "EnableDepth %s\n", EnableDepth ? "On" : "Off");
				fprintf(stderr, "ViveSR_ChangeModuleLinkStatus %d\n", ViveSR_ChangeModuleLinkStatus(ID_PASSTHROUGH, ID_DEPTH, EnableDepth ? ViveSR::SRWork_Link_Method::SR_ACTIVE : ViveSR::SRWork_Link_Method::SR_NONE));
			}
			else if (str == 'q') {
				fprintf(stderr, "ViveSR_RegisterDepthDataCallback %d\n", ViveSR_RegisterCallback(ID_DEPTH, ViveSR::Depth::Callback::BASIC, Get3DViewerBackgroundCallback));
				fprintf(stderr, "ViveSR_RegisterRigidReconstructionDataCallback %d\n", ViveSR_RegisterCallback(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Callback::BASIC, GetRigidReconstructionCallback));
			}
			else if (str == 'w') {
				res = ViveSR_SetCommandBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::START, true);	// send start cmd
				fprintf(stderr, "Send Command : START RIGID RECONSTRUCTION(%d)\n", res);
			}
			else if (str == 'e') {
				res = ViveSR_SetCommandBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::STOP, true);		// send stop cmd
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
				fprintf(stderr, "%s SECTOR_GROUPING\n", EnableSector ? "ENABLE" : "DISABLE");
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
				res = ViveSR_SetCommandBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::EXTRACT_POINT_CLOUD, LiveExtraction);		// toggle on-line point cloud extraction
				fprintf(stderr, "Send Command : %s EXTRACT POINT CLOUD(%d)\n", LiveExtraction ? "START" : "STOP", res);
			}
			// save to obj (VivePro.obj)
			else if (str == 'y') {
				char obj_name[] = "VivePro";
				res = ViveSR_SetCommandString(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::EXPORT_MODEL_RIGHT_HAND, obj_name);			// call save model to model name
				fprintf(stderr,"Send Command : Save Reconstruction Data to %s(%d)\n", obj_name, res);
			}			
            else if (str == 'u') {
                bool bEnableSectorGrouping, bEnableFrustumCulling;
                ViveSR_GetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, &bEnableSectorGrouping);
                ViveSR_GetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, &bEnableFrustumCulling);
                
                // scene understanding is incompatible with these following functionalities
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, false);
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, false);

                res = ViveSR_RegisterCallback(ID_RIGID_RECONSTRUCTION, (int)ViveSR::RigidReconstruction::Callback::SCENE_UNDERSTANDING_PROGRESS, UpdateSemanticProgress);
                fprintf(stderr, "Send Command : Register Semantic Scene Understanding Progress Callback %d\n", res);                
                
                char obj_name[] = "SceneObjList";
                res = ViveSR_SetCommandString(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Cmd::EXPORT_SCENE_UNDERSTANDING_FOR_UNITY, obj_name);
                fprintf(stderr, "Send Command : Save Semantic Scene Understanding Data to %s(%d)\n", obj_name, res);

                // get progress bar from event listener 
                {
                    int lastSemanticProgress = 0;
                    int lastSemanticStage = 0;
                    do
                    {
                        if (lastSemanticProgress < semanticProgress || lastSemanticStage < semanticStage) {
                            fprintf(stderr, "Semantic: stage %d, progress %d\n", semanticStage, semanticProgress);
                            lastSemanticStage = semanticStage;
                            lastSemanticProgress = semanticProgress;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    } while ( lastSemanticProgress < 100 || lastSemanticStage < 1 );
                }

                // reset the progress information and settings
                semanticProgress = 0;
                semanticStage = 0;
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, bEnableSectorGrouping);
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, bEnableFrustumCulling);
                fprintf(stderr, "Semantic scene understanding complete\n");
            }
			else if (str == 'g'){
				bool isDistorted = false;
				TestGetFrameData(isDistorted);
			}
			else if (str == 'o') {
				static bool flag_extract_depth_mesh = false;
				flag_extract_depth_mesh = !flag_extract_depth_mesh;
				res = ViveSR_SetCommandBool(ID_DEPTH, ViveSR::Depth::Cmd::EXTRACT_DEPTH_MESH, flag_extract_depth_mesh);
				fprintf(stderr, "Command : %s EXTRACT_DEPTH_MESH(%d)\n", flag_extract_depth_mesh ? "START" : "STOP", res);
			}
			else if (str == 'p') {
				static bool flag_enable_depth_fps_control = false;
				int nFPS = 1;
				// Fixed module(depth module in this case) to constant N FPS(1 in this case).
				flag_enable_depth_fps_control = !flag_enable_depth_fps_control;
				res = ViveSR_SetParameterBool(ID_DEPTH, ViveSR::Module_Param::ENABLE_FPSCONTROL, flag_enable_depth_fps_control);
				res = ViveSR_SetParameterInt(ID_DEPTH, ViveSR::Module_Param::SET_FPS, nFPS);
				std::string msg = flag_enable_depth_fps_control ? std::string("ENABLE_FPSCONTROL : ") + std::to_string(nFPS) : std::string("DISABLE_FPSCONTROL");
				fprintf(stderr, "[Depth Module] %s\n", msg.c_str());
			}
			else if (str == 'f') {
				EnableSector = !EnableSector;
				// 0329-To-Do: use the first one (ENUM 38)
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
            else if (str == '.') { 
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_ENABLE, true);
                bool enableVision;
                ViveSR_GetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_MACHINE_VISION, &enableVision);

                // reset live extraction loop state
                if (!enableVision)
                {
                    // scene understanding is incompatible with these following functionalities
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, false);
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, false);

                    renderWhiteColor = 0.0f; // semantic preview requires color on mesh
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::LITE_POINT_CLOUD_MODE, false);
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::LIVE_ADAPTIVE_MODE, true);
                    ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::FULL_POINT_CLOUD_MODE, false);

                    res = ViveSR_RegisterCallback(ID_RIGID_RECONSTRUCTION, (int)ViveSR::RigidReconstruction::Callback::SCENE_UNDERSTANDING_PROGRESS, UpdateSemanticProgress);
                    fprintf(stderr, "Send Command : Register Semantic Scene Understanding Progress Callback %d\n", res);
                }
                else renderWhiteColor = 1.0f; // resume mesh color to be white when semantic preview is off 
    
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_MACHINE_VISION, !enableVision);
                fprintf(stderr, "SET SEMANTIC VISUALIZATION %d\n", !enableVision);

                if (!enableVision) {
                    int lastSemanticProgress = 0;
                    int lastSemanticStage = 0;
                    do 
                    {
                        if (lastSemanticProgress < semanticProgress || lastSemanticStage < semanticStage)
                        {
                            fprintf(stderr, "Semantic: stage %d, progress %d\n", semanticStage, semanticProgress);
                            lastSemanticStage = semanticStage;
                            lastSemanticProgress = semanticProgress;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    } while ( lastSemanticProgress < 100 || lastSemanticStage < 1 );

                }
                // reset the progress information
                semanticProgress = 0;
                semanticStage = 0;

                fprintf(stderr, "Semantic scene understanding complete\n");
            }
            else if (str == ';') {
                SceneUnderstandingConfig *config = new SceneUnderstandingConfig;
                ViveSR_GetParameterStruct(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void**>(&config));
                fprintf(stderr, "Semantic config: maxNumInst for chair %d, floor %d, wall %d, ceiling %d\n", config[0].nChairMaxInst, config[0].nFloorMaxInst, config[0].nWallMaxInst, config[0].nCeilingMaxInst);
                
                // set config for scene understanding
                config[0].nChairMaxInst = 4;
                config[0].nFloorMaxInst = 2;
                ViveSR_SetParameterStruct(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, config);
                
                // read again
                ViveSR_GetParameterStruct(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void**>(&config));
                fprintf(stderr, "Semantic config: maxNumInst for chair %d, floor %d, wall %d, ceiling %d\n", config[0].nChairMaxInst, config[0].nFloorMaxInst, config[0].nWallMaxInst, config[0].nCeilingMaxInst);
            }
            else if (str == '[') {
                ViveSR_SetParameterBool(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_ENABLE, true);
                bool bSemanticEnabled;
                fprintf(stderr, "Semantic scene understanding is enabled");
            }
        }
	}
#endif
}

#ifndef GUI_VIEWER
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
void GetDistortedCallback(int key)
{
	char *ptr_l, *ptr_r;
	static int width, height, channel;
	static bool first = true;
	if (first) {
		ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_WIDTH, &width);
		ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_HEIGHT, &height);
		ViveSR_GetParameterInt(ID_PASSTHROUGH, ViveSR::PassThrough::OUTPUT_DISTORTED_CHANNEL, &channel);
		fprintf(stderr, "Distorted width :%d, height: %d, channel : %d\n", width, height, channel);
		first = false;
	}

	ViveSR_GetPointer(key, ViveSR::PassThrough::DataMask::DISTORTED_FRAME_LEFT, (void**)&ptr_l);
	ViveSR_GetPointer(key, ViveSR::PassThrough::DataMask::DISTORTED_FRAME_RIGHT, (void**)&ptr_r);
	auto CV_Type = channel == 3 ? CV_8UC3 : CV_8UC4;
	auto CV_Code = channel == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
	cv::Mat imgLeft = cv::Mat(height, width, CV_Type, ptr_l).clone();
	cv::Mat imgRight = cv::Mat(height, width, CV_Type, ptr_r).clone();
	cv::cvtColor(imgLeft, imgLeft, CV_Code);
	cv::cvtColor(imgRight, imgRight, CV_Code);
	cv::imshow("Callback Distorted Left", imgLeft);
	cv::imshow("Callback Distorted Right", imgRight);
	//fprintf(stderr, "%f %f %f\n", pose_l[12], pose_l[13], pose_l[14]);
	//fprintf(stderr, "%f %f %f\n", pose_r[12], pose_r[13], pose_r[14]);
	cv::waitKey(1);
}
void GetUndistortedCallback(int key)
{
	char *ptr_l, *ptr_r;
	ViveSR_GetPointer(key, ViveSR::PassThrough::DataMask::UNDISTORTED_FRAME_LEFT, (void**)&ptr_l);
	ViveSR_GetPointer(key, ViveSR::PassThrough::DataMask::UNDISTORTED_FRAME_RIGHT, (void**)&ptr_r);
	auto CV_Type = UNDISTORTED_IMAGE_C == 3 ? CV_8UC3 : CV_8UC4;
	auto CV_Code = UNDISTORTED_IMAGE_C == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
	cv::Mat imgLeft = cv::Mat(UNDISTORTED_IMAGE_H, UNDISTORTED_IMAGE_W, CV_Type, ptr_l).clone();
	cv::Mat imgRight = cv::Mat(UNDISTORTED_IMAGE_H, UNDISTORTED_IMAGE_W, CV_Type, ptr_r).clone();
	cv::cvtColor(imgLeft, imgLeft, CV_Code);
	cv::cvtColor(imgRight, imgRight, CV_Code);
	cv::imshow("Callback Undistorted Left", imgLeft);
	cv::imshow("Callback Undistorted Right", imgRight);
	cv::waitKey(1);
}
void GetDepthCallback(int key)
{
	char *ptr_l;
	float *ptr_d;
	unsigned int *ptr_f;
	unsigned int *ptr_t;
	float *ptr_pose;
	int *ptr_num_vertices;
	int *ptr_byte_vertex;
	float *ptr_vertices;
	int *ptr_num_indices;
	int *ptr_indices;
	ViveSR_GetPointer(key, 0, (void**)&ptr_l);
	ViveSR_GetPointer(key, 1, (void**)&ptr_d);
	ViveSR_GetPointer(key, 2, (void**)&ptr_f);
	ViveSR_GetPointer(key, 3, (void**)&ptr_t);
	ViveSR_GetPointer(key, 4, (void**)&ptr_pose);
	ViveSR_GetPointer(key, 5, (void**)&ptr_num_vertices);
	ViveSR_GetPointer(key, 6, (void**)&ptr_byte_vertex);
	ViveSR_GetPointer(key, 7, (void**)&ptr_vertices);
	ViveSR_GetPointer(key, 8, (void**)&ptr_num_indices);
	ViveSR_GetPointer(key, 9, (void**)&ptr_indices);

	unsigned int FrameSeq = ptr_f[0], TimeStp = ptr_t[0];
	auto CV_Type = DISTORTED_IMAGE_C == 3 ? CV_8UC3 : CV_8UC4;
	auto CV_Code = DISTORTED_IMAGE_C == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
	cv::Mat depth = cv::Mat(DEPTH_DATA_H, DEPTH_DATA_W, CV_32FC1, ptr_d).clone();
	cv::Mat img = cv::Mat(DEPTH_DATA_H, DEPTH_DATA_W, CV_Type, ptr_l).clone();
	cv::cvtColor(img, img, CV_Code);

	//    you can press 'o' to enable live mesh('u' for disabling this function)
	//    vertices data usage :
	//    char *vertice_buf = malloc(vertexStride*numVert);
	//    memcpy(vertice_buf, vertices, vertexStride*numVert);
	//    idx data usage :
	//    int *idx_buf = (int*)malloc(sizeof(int)*numIdx);
	//    memcpy(idx_buf, idx, sizeof(int)*numIdx);
	//int numVert = ptr_num_vertices[0];
	//int vertexStride = ptr_byte_vertex[0];
	int numIdx = ptr_num_indices[0];
	//if (vertexStride*numVert>0)		printf("vertices data size is %d\n", vertexStride*numVert);
	if (numIdx>0)		printf("idx data size is %d\n", numIdx);

	const int width = 640, height = 480;
	cv::Mat show = cv::Mat(height, width, CV_8UC3);
	double min, max;
	cv::threshold(depth, depth, 0, 255, CV_THRESH_TOZERO);
	cv::minMaxIdx(depth, &min, &max);
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			show.at<cv::Vec3b>(j, i) = (cv::Vec3b)(GetColour(depth.at<float>(j, i) / 255, 0, 1) * 255);
		}
	}
	//depth.convertTo(show, CV_8UC3, -1.0, 255);
	//cv::applyColorMap(show, show, cv::COLORMAP_JET);
	int center_x = width / 2, center_y = height / 2, rect = 5;
	float ROI = MIN(CameraParameters.FocalLength_L, CameraParameters.FocalLength_L) * tan(35 * 3.14159 / 180);
	float sum = 0, avg = 0, distance;
	int negitive = 0, posivte_inf = 0;
	for (int w = width / 2 - rect; w < width / 2 + rect; w++) {
		for (int h = height / 2 - rect; h < height / 2 + rect; h++) {
			if (ptr_d[w + width * h] >= DBL_MAX)
				posivte_inf++;
			else if (ptr_d[w + width * h] < 0.0)
				negitive++;
			else
				sum += ptr_d[w + width * h];
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

	if (avg <0.0)
		printf(" avg  %f,   sum    %f,    num    %f,  negitive   %d,   inf  %d\n", avg, sum, rect * rect * 4 - negitive - posivte_inf);

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

#include "viewer.h"

float pSizeScale;
char *recons_data_ptr = nullptr;
char *recons_pose = nullptr;
char *num_vert = nullptr;
char *byte_vert = nullptr;
char *vertices = nullptr;
char *num_idx = nullptr;
char *idx = nullptr;

char *pcld_type = nullptr;
char *pcld_collider_num = nullptr;
char *pcld_num_vert = nullptr;
char *pcld_num_idx = nullptr;
char *pcld_vertices = nullptr;
char *pcld_indices = nullptr;

char *ptr_sector_num = nullptr;
char *sector_id_list = nullptr;
char *sector_vertices_num = nullptr;
char *sector_indices_num = nullptr;

void Get3DViewerBackgroundCallback(int key)
{
	static PointCloudViewer viewer(CameraParameters.FocalLength_L);
	char *ptr_l;
	ViveSR_GetPointer(key, ViveSR::Depth::DataMask::LEFT_FRAME, (void**)&ptr_l);

	static int lastNumVerts = -1;
	if (recons_data_ptr)
	{
		int numVert = *((int*)num_vert);
		if (lastNumVerts != numVert)
		{
			lastNumVerts = numVert;
			viewer.UpdateGeometry((float*)vertices, numVert, *(int*)byte_vert, *(int*)ptr_sector_num, (int*)sector_id_list, (int*)sector_vertices_num, (int*)sector_indices_num, (int*)idx, *(int*)num_idx, EnableSector);
		}
		//viewer.UpdateCamera((float*)recons_pose, true);
	}
	float undistort_pose[16];
	int result = ViveSR::SR_Error::FAILED;
	const int NUM_DATA = 1;
	DataInfo *dataInfos = new DataInfo[NUM_DATA];
	dataInfos[0].mask = ViveSR::PassThrough::DataMask::POSE_LEFT;
	dataInfos[0].ptr = undistort_pose;
	if (ViveSR::SR_Error::WORK != ViveSR_GetMultiDataSize(ID_PASSTHROUGH, dataInfos, NUM_DATA)) return;
	if (ViveSR::SR_Error::WORK != ViveSR_GetMultiData(ID_PASSTHROUGH, dataInfos, NUM_DATA)) return;

	ViveSR_GetMultiData(ID_PASSTHROUGH, dataInfos, NUM_DATA);
	viewer.UpdateCamera(undistort_pose, false);
	viewer.UpdateBackground(ptr_l, DEPTH_DATA_W, DEPTH_DATA_H);
	viewer.Draw(pSizeScale, renderWhiteColor);
	delete[] dataInfos;
}
void GetRigidReconstructionCallback(int key)
{
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::FRAME_SEQ, (void**)&recons_data_ptr);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::POSEMTX44, (void**)&recons_pose);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::NUM_VERTICES, (void**)&num_vert);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::BYTEPERVERT, (void**)&byte_vert);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::VERTICES, (void**)&vertices);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::NUM_INDICES, (void**)&num_idx);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::INDICES, (void**)&idx);

	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::CLDTYPE, (void**)&pcld_type);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::COLLIDERNUM, (void**)&pcld_collider_num);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::CLD_NUM_VERTS, (void**)&pcld_num_vert);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::CLD_NUMIDX, (void**)&pcld_num_idx);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::CLD_VERTICES, (void**)&pcld_vertices);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::CLD_INDICES, (void**)&pcld_indices);

	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::SECTOR_NUM, (void**)&ptr_sector_num);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::SECTOR_ID_LIST, (void**)&sector_id_list);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::SECTOR_VERT_NUM, (void**)&sector_vertices_num);
	ViveSR_GetPointer(key, ViveSR::RigidReconstruction::DataMask::SECTOR_IDX_NUM, (void**)&sector_indices_num);

	float pointRenderSize;
	ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::POINTCLOUD_POINTSIZE, &pointRenderSize);
	pSizeScale = (pointRenderSize / 0.01f);
}

void ShowExportProgressCallback(int stage, int percentage)
{
	int totalPercentage;
	if (stage == ViveSR::RigidReconstruction::ExportStage::EXTRACT_MODEL)
		totalPercentage = (int)((float)percentage * 0.25f);
	else if (stage == ViveSR::RigidReconstruction::ExportStage::COMPACT_TEXTURE)
		totalPercentage = 25 + (int)((float)percentage * 0.25f);
	else if (stage == ViveSR::RigidReconstruction::ExportStage::SAVE_MODEL)
		totalPercentage = 50 + (int)((float)percentage * 0.25f);
	else if (stage == ViveSR::RigidReconstruction::ExportStage::EXTRACT_COLLIDER)
		totalPercentage = 75 + (int)((float)percentage * 0.25f);

	std::cout << "Exporting Progress: " << totalPercentage << "%" << std::endl;
}
void UpdateSemanticProgress(int stage, int percentage)
{
	if (stage == (int)ViveSR::RigidReconstruction::ExportStage::SCENE_UNDERSTANDING_PASS_1) semanticStage = 0;
	if (stage == (int)ViveSR::RigidReconstruction::ExportStage::SCENE_UNDERSTANDING_PASS_2) semanticStage = 1;
	semanticProgress = percentage;
}

#endif