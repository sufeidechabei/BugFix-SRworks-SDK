#include "headers.h"

#define DISTORTED_W 612
#define DISTORTED_H 460
#define UNDISTORTED_W 1150
#define UNDISTORTED_H 750
#define DEPTH_W 640
#define DEPTH_H 480

#define VIEWER_W 1280
#define VIEWER_H 800

// Var<bool>: // label name, def value, is checkbox(T) or button(F)
// Var<string>: // label name, value(optional)
// Var<float>: // label name, value, slider min, max
pangolin::Var<std::string> AppViewerLabel("appStateUI.[==== Application Viewer ====]");
pangolin::Var<int> FPSControl("appStateUI.SeeThru FPS Limit:", 60, 1, 60);
pangolin::Var<std::string> ViewerCurrentFPS("appStateUI.Current Render FPS:", "0");
pangolin::Var<std::string> SRWorksFPS("appStateUI.Current SRWorks FPS:", "0");
//.
pangolin::Var<bool> SeeThruMODLabel("seeThruUI.<<<<< See-Through Menu >>>>>", false, false);
pangolin::Var<bool> ShowDistort("seeThruUI.Show Distortion Image", false, true);
pangolin::Var<bool> ShowUndistort("seeThruUI.Show Undistorted Image", false, true);
pangolin::Var<std::string> UndistImgInfo("seeThruUI.Undist-W/H/CH:");
pangolin::Var<std::string> DistImgInfo("seeThruUI.Dist-W/H/CH:");
//
pangolin::Var<bool> DepthMODLabel("depthUI.<<<< Depth Engine Menu >>>>", false, false);
pangolin::Var<bool> EnableDepthEngine("depthUI.Enable Depth Engine", true, true);
pangolin::Var<bool> ShowDepth("depthUI.Show Depth Image", false, true);
pangolin::Var<bool> EnableDepthRefine("depthUI.Enable Depth Refinement", true, true);
pangolin::Var<bool> EnableEdgeEnhance("depthUI.Enable Edge Enhance", false, true);
pangolin::Var<std::string> DepthImgInfo("depthUI.Depth-W/H:");
//
pangolin::Var<bool> ReconsMODLabel("reconsUI.<<< 3D Reconstruction Menu >>>", false, false);
pangolin::Var<bool> RunRigidReconstruction("reconsUI.Start Reconstruction", false, true);
pangolin::Var<bool> DrawReconstruct("reconsUI.Show Reconstruction", false, true);
pangolin::Var<bool> SwitchGeoDisplayMode("reconsUI.Switch Display Mode", false, false);
pangolin::Var<std::string> DispMode("reconsUI.[Display Mode]:", "");
pangolin::Var<bool> EnableSector("reconsUI.Enable Sector", false, true);
pangolin::Var<bool> MeshPreviewButton("reconsUI.Preview Scanned Mesh", false, false);
pangolin::Var<std::string> MeshPreviewPercent("reconsUI.[Mesh Preview Percentage]:", "0%");
pangolin::Var<bool> SaveMeshButton("reconsUI.Save Mesh to VivePro-obj", false, false);
pangolin::Var<std::string> ExportPercent("reconsUI.[Export Percentage]:", "0%");
pangolin::Var<std::string> VertexNum("reconsUI.[Updated Vertex Num]:", "0");
pangolin::Var<std::string> TriangleNum("reconsUI.[Updated Triangle Num]:", "0");
pangolin::Var<float> AdptMinGrid("reconsUI.[Adpat-Mesh MinGridSize]", 2.0f, 2.0f, 64.0f);
pangolin::Var<float> AdptMaxGrid("reconsUI.[Adpat-Mesh MaxGridSize]", 2.0f, 2.0f, 64.0f);
pangolin::Var<float> AdptDivideThre("reconsUI.[Adpat-Mesh Divide Thres]", 0.3f, 0.2f, 0.7f);
//
pangolin::Var<bool> semanticLabel("semanticUI.<<< Scene Understanding Menu >>>", false, false);
pangolin::Var<std::string> semanticProgressUI("semanticUI.[Status]:", "");
pangolin::Var<bool> semanticExport("semanticUI.Export Scene Info", false, false);
pangolin::Var<bool> semanticPreview("semanticUI.Enable Machine Vision", false, false);
pangolin::Var<bool> semanticFilter("semanticUI.Enable refinement", false, true);
pangolin::Var<bool> semanticTable("semanticUI.View/Export Table", true, true);
pangolin::Var<bool> semanticBed("semanticUI.View/Export Bed", true, true);
pangolin::Var<bool> semanticChair("semanticUI.View/Export Chair", true, true);
pangolin::Var<bool> semanticFloor("semanticUI.View/Export Floor", true, true);
pangolin::Var<bool> semanticWall("semanticUI.View/Export Wall", true, true);
pangolin::Var<bool> semanticCeiling("semanticUI.View/Export Ceiling", true, true);
pangolin::Var<bool> semanticMonitor("semanticUI.View/Export Monitor", true, true);
pangolin::Var<bool> semanticWindow("semanticUI.View/Export Window", true, true);
pangolin::Var<bool> semanticPerson("semanticUI.View/Export Person", true, true);
pangolin::Var<bool> semanticFurniture("semanticUI.View/Export Furniture", true, true);
pangolin::Var<bool> semanticDoor("semanticUI.View/Export Door", true, true);

// image module
char distortData_L[4 * DISTORTED_W * DISTORTED_H];
char distortData_R[4 * DISTORTED_W * DISTORTED_H];
char undistortData_L[4 * UNDISTORTED_W * UNDISTORTED_H];
char undistortData_R[4 * UNDISTORTED_W * UNDISTORTED_H];
float depthData[DEPTH_W * DEPTH_H];
char cropSeeThruData[4 * DEPTH_W * DEPTH_H];
float camera_pose[16];
long long SRFrames = 0;

// reconstruction
// PS: may not be thread-safe
char *num_vert = nullptr;
char *byte_vert = nullptr;
char *vertices = nullptr;
char *num_idx = nullptr;
char *idx = nullptr;
char *ptr_sector_num = nullptr;
char *sector_id_list = nullptr;
char *sector_vertices_num = nullptr;
char *sector_indices_num = nullptr;
char *model_chunk_num = nullptr;
char *model_chunk_idx = nullptr;
int exportPercentage = 0;
int renderMeshPercentage = 0;

int semanticExportStage = 0, semanticExportProgress = 0, semanticExportNetProgress = 0;
std::mutex vive_api_mutex;
std::mutex crop_through_mutex;
std::mutex see_through_mutex;
std::mutex depthMutex;
std::mutex reconsMutex;
//std::mutex semanticMutex;

bool modelPreviewStarted = false;
//bool modelPreviewMoveToNextPart = false;

//////////////////////////////////////////////////////////////////////////
bool g_get_seethrough_threading = true;
bool g_get_depth_map_threading = true;
bool g_get_reconstruction_threading = true;
void GetSeeThroughTask(
    int ID_PASSTHROUGH,
    int distorted_width,
    int distorted_height,
    int distorted_channel,
    int undistorted_width,
    int undistorted_height,
    int undistorted_channel) {
#pragma region ALLOCATE_SEE_THROUGH
    
    auto Distored_img_CV_Type = distorted_channel == 3 ? CV_8UC3 : CV_8UC4;
    auto Distored_img_CV_Code = distorted_channel == 3 ? CV_RGB2BGR : CV_RGBA2BGRA;
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
    auto last_frame_timestamp = std::chrono::high_resolution_clock::now();
    using namespace std::chrono_literals;
    const static auto MIN_FRAME_INTERVAL = 33ms;
    while (g_get_seethrough_threading)
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
        int res = ViveSR::FAILED;
        //See through
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            res = ViveSR_GetModuleData(ID_PASSTHROUGH, see_through_element.data(), see_through_count);
        }
   
        
        if (res == ViveSR::WORK) {
            if (see_through_mutex.try_lock())
            {
                memcpy(distortData_L, distorted_frame_left.get(), sizeof(char) * 4 * distorted_width * distorted_height);
                memcpy(distortData_R, distorted_frame_right.get(), sizeof(char) * 4 * distorted_width * distorted_height);
                memcpy(undistortData_L, undistorted_frame_left.get(), sizeof(char) * 4 * undistorted_width* undistorted_height);
                memcpy(undistortData_R, undistorted_frame_right.get(), sizeof(char) * 4 * undistorted_width * undistorted_height);
                memcpy(camera_pose, pose_left, sizeof(float) * 16);

                see_through_mutex.unlock();
            }
            if (crop_through_mutex.try_lock()) {
                auto ptr = undistortData_L;
                ptr += (UNDISTORTED_W * 4) * 135;	// skip upper 135 rows
                ptr += 255 * 4;						// skip front 255 pixels
                for (int row = 0; row < DEPTH_H; ++row)
                {
                    memcpy(cropSeeThruData + (DEPTH_W * 4) * row, ptr, sizeof(char) * 4 * DEPTH_W);
                    ptr += UNDISTORTED_W * 4;
                }
                ++SRFrames;
                crop_through_mutex.unlock();
            }
        }
#pragma endregion SHOW_SEETROUGH
    }
}


// 
void GetDepthMapParallelRun(int ID_DEPTH, int depth_img_width, int depth_img_height, int depth_img_channel, int depth_color_img_channel)
{
    const static int DEPTH_ELEM_LENGTH = ViveSR::Depth::OutputMask::OUTPUT_MASK_MAX;
    static std::vector<ViveSR::MemoryElement> depth_element(DEPTH_ELEM_LENGTH);
    auto left_frame = std::make_unique<char[]>(depth_img_height*depth_img_width*depth_color_img_channel);
    auto depth_map = std::make_unique<float[]>(depth_img_height*depth_img_width*depth_img_channel);
    unsigned int frame_seq;
    unsigned int time_stp;
    float pose[16];
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
    for (int i = 0; i < ViveSR::Depth::OutputMask::OUTPUT_MASK_MAX; ++i)
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
    const static auto MIN_FRAME_INTERVAL = 33ms;
    while (g_get_depth_map_threading)
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
        int res = ViveSR::FAILED;
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            res = ViveSR_GetModuleData(ID_DEPTH, depth_element.data(), depth_count);
        }
        if (res == ViveSR::WORK) {
            if (depthMutex.try_lock()) {
                memcpy(depthData, depth_map.get(), sizeof(float) * DEPTH_W * DEPTH_H);
                for (int i = 0; i < DEPTH_W *DEPTH_H; i++)
                    depthData[i] = (depthData[i] - 15.0f) / (250.0f - 15.0f);
                depthMutex.unlock();
            }
        }
    }
}
void GetRigiReconstructionParallelRun(int ID_RIGID_RECONSTRUCTION) {
#pragma region ALLOCATE_RECONSTRCUTION
    unsigned int frame_seq;	        // sizeof(unsigned int)
    auto posemtx44 = std::make_unique<float[]>(16);               // sizeof(float) * 16
    int num_vertices;	            // sizeof(int)
    num_vert = (char*)&num_vertices;
    int bytepervert;	            // sizeof(int)
    byte_vert = (char*)&bytepervert;
    auto vertices_uptr = std::make_unique<float[]>(8 * 2500000);	            // sizeof(float) * 8 * 2500000
    vertices = (char*)vertices_uptr.get();
    int num_indices;	            // sizeof(int)
    num_idx = (char*)&num_indices;
    auto indices = std::make_unique<int[]>(2500000);	                // sizeof(int) * 2500000
    idx = (char*)indices.get();
    int cldtype;	                // sizeof(int)
    unsigned int collidernum;	    // sizeof(unsigned int)
    auto cld_num_verts = std::make_unique<unsigned int[]>(200);	// sizeof(unsigned int) * 200
    auto cld_numidx = std::make_unique<unsigned int[]>(200);	    // sizeof(unsigned int) * 200
    auto cld_vertices = std::make_unique<float[]>(3 * 50000);	        // sizeof(float) * 3 * 50000
    auto cld_indices = std::make_unique<int[]>(100000);	            // sizeof(int) * 100000
    int sector_num;                 // sizeof(int)
    ptr_sector_num = (char*)&sector_num;
    auto sector_id_list_uptr = std::make_unique<int[]>(1000000);            // sizeof(int) * 1000000
    sector_id_list = (char*)sector_id_list_uptr.get();
    auto sector_vert_num = std::make_unique<int[]>(1000000);           // sizeof(int) * 1000000
    sector_vertices_num = (char*)sector_vert_num.get();
    auto sector_idx_num = std::make_unique<int[]>(1000000);            // sizeof(int) * 1000000
    sector_indices_num = (char*)sector_idx_num.get();
    int model_chunk_num_instance;            // sizeof(int)
    model_chunk_num = (char*)&model_chunk_num_instance;
    int model_chunk_idx_instance;            // sizeof(int)
    model_chunk_idx = (char*)&model_chunk_idx_instance;
    const int rigid_reconstruction_length = ViveSR::RigidReconstruction::OutputMask::MODEL_CHUNK_IDX + 1;
    static std::vector<ViveSR::MemoryElement> rigid_reconstruction_element(rigid_reconstruction_length);
    void* ptrs[rigid_reconstruction_length]{
        &frame_seq,	        // sizeof(unsigned int)
        posemtx44.get(),            // sizeof(float) * 16
        &num_vertices,	    // sizeof(int)
        &bytepervert,	        // sizeof(int)
        vertices_uptr.get(),	            // sizeof(float) * 8 * 2500000
        &num_indices,	        // sizeof(int)
        indices.get(),	            // sizeof(int) * 2500000
        &cldtype,	            // sizeof(int)
        &collidernum,	        // sizeof(unsigned int)
        cld_num_verts.get(),	    // sizeof(unsigned int) * 200
        cld_numidx.get(),	        // sizeof(unsigned int) * 200
        cld_vertices.get(),	        // sizeof(float) * 3 * 50000
        cld_indices.get(),	        // sizeof(int) * 100000
        &sector_num,          // sizeof(int)
        sector_id_list_uptr.get(),       // sizeof(int) * 1000000
        sector_vert_num.get(),      // sizeof(int) * 1000000
        sector_idx_num.get(),       // sizeof(int) * 1000000
        &model_chunk_num_instance,     // sizeof(int)
        &model_chunk_idx_instance,     // sizeof(int)
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
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_GetParameterFloat(ID_RIGID_RECONSTRUCTION, ViveSR::RigidReconstruction::Param::POINTCLOUD_POINTSIZE, &pointRenderSize);
    }
    pSizeScale = (pointRenderSize * 100.f);
    while (g_get_reconstruction_threading)
    {
        static int lastNumVerts = -1;
        int result = ViveSR::Error::FAILED;
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            result = ViveSR_GetModuleData(ID_RIGID_RECONSTRUCTION, rigid_reconstruction_element.data(), nCount);
        }
    }

#pragma endregion ALLOCATE_RECONSTRCUTION
}

void UpdateSemanticProgress(int stage, int perc)
{
	if (stage == (int)ViveSR::RigidReconstruction::ExportStage::SCENE_UNDERSTANDING_PASS_1) semanticExportStage = 0;
	if (stage == (int)ViveSR::RigidReconstruction::ExportStage::SCENE_UNDERSTANDING_PASS_2) semanticExportStage = 1;

	semanticExportProgress = perc;

	const float ProportionStage1 = 0.90f; // hardcoded
	const float ProportionStage2 = 0.10f;
	if (semanticExportStage == 0) semanticExportNetProgress = (int)(ProportionStage1 * semanticExportProgress);
	if (semanticExportStage == 1) semanticExportNetProgress = (int)(ProportionStage1 * 100 + ProportionStage2 * semanticExportProgress);

}
void UpdateExportProgressCallback(int stage, int percentage)
{
	if (stage == ViveSR::RigidReconstruction::ExportStage::EXTRACT_MODEL)
		exportPercentage = (int)((float)percentage * 0.25f);
	else if (stage == ViveSR::RigidReconstruction::ExportStage::COMPACT_TEXTURE)
		exportPercentage = 25 + (int)((float)percentage * 0.25f);
	else if (stage == ViveSR::RigidReconstruction::ExportStage::SAVE_MODEL)
		exportPercentage = 50 + (int)((float)percentage * 0.25f);
	else if (stage == ViveSR::RigidReconstruction::ExportStage::EXTRACT_COLLIDER)
		exportPercentage = 75 + (int)((float)percentage * 0.25f);

	//std::cout << "Exporting Progress: " << exportPercentage << "%" << std::endl;
}

void UpdateExtractProgressCallback(int stage, int percentage)
{
	if (stage == ViveSR::RigidReconstruction::ExportStage::EXTRACT_MODEL)
		renderMeshPercentage = (int)((float)percentage);

	//std::cout << "stage: " << stage << ", percentage:" << percentage << "%" << std::endl;
}

//////////////////////////////////////////////////////////////////////////
void Normalize(float* vec)
{
	float invLen = 1.0f / sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	vec[0] *= invLen;
	vec[1] *= invLen;
	vec[2] *= invLen;
}

float Dot(float* vec1, float* vec2)
{
	return (vec1[0] * vec2[0]) + (vec1[1] * vec2[1]) + (vec1[2] * vec2[2]);
}

//

//////////////////////////////////////////////////////////////////////////
void AppViewer::Initialize(int seeThruMID, int depthMID, int reconsMID)
{
	pangolin::CreateWindowAndBind("SRWorks", VIEWER_W, VIEWER_H);
	std::cout << "gelewInit = " << glewInit() << std::endl;

	//Ethan: can we change it to use 4 ? it's faster (when ReadPixels / DrawPixels)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	// Undistort, Distort, Depth, SeeThru
	_seeThruBGTex.Reinitialise(DEPTH_W, DEPTH_H);
	_distortTex_L.Reinitialise(DISTORTED_W, DISTORTED_H);
	_distortTex_R.Reinitialise(DISTORTED_W, DISTORTED_H);
	_undistortTex_L.Reinitialise(UNDISTORTED_W, UNDISTORTED_H);
	_undistortTex_R.Reinitialise(UNDISTORTED_W, UNDISTORTED_H);
	_depthColorMap = new DepthColorTex(DEPTH_W, DEPTH_H);

	pangolin::CreatePanel("appStateUI").SetBounds(_moduleUITop, 1.0, 0.0, pangolin::Attach::Pix(250));
	_seeThruPanel = &pangolin::CreatePanel("seeThruUI");
	_depthPanel = &pangolin::CreatePanel("depthUI");
	_reconsPanel = &pangolin::CreatePanel("reconsUI");
	_semanticPanel = &pangolin::CreatePanel("semanticUI");
	_emptyPanel = &pangolin::CreatePanel("emptyUI");
	_seeThruPanelHeight = SeeThruMODLabel ? _seeThruPanelMaxH : 0.04f;
	_depthPanelHeight = DepthMODLabel ? _depthPanelMaxH : 0.04f;
	_reconsPanelHeight = ReconsMODLabel ? _reconsPanelMaxH : 0.04f;
	_semanticPanelHeight = semanticLabel ? _semanticPanelMaxH : 0.04f;
	_ArrangePanel();

	pangolin::Display("scene").SetBounds(0, 1.0f, pangolin::Attach::Pix(250), 1.0f, (float)DEPTH_W / DEPTH_H);
	pangolin::Display("DistortLeft").SetAspect((float)DISTORTED_W / DISTORTED_H);
	pangolin::Display("DistortRight").SetAspect((float)DISTORTED_W / DISTORTED_H);
	pangolin::Display("UndistortLeft").SetAspect((float)UNDISTORTED_W / UNDISTORTED_H);
	pangolin::Display("UndistortRight").SetAspect((float)UNDISTORTED_W / UNDISTORTED_H);
	pangolin::Display("Depth").SetAspect((float)DEPTH_W / DEPTH_H);

	pangolin::Display("RenderTargets").SetBounds(0.0, 0.25f, pangolin::Attach::Pix(250), 1.0f)
		.SetLayout(pangolin::LayoutEqualHorizontal)
		.AddDisplay(pangolin::Display("DistortLeft"))
		.AddDisplay(pangolin::Display("DistortRight"))
		.AddDisplay(pangolin::Display("UndistortLeft"))
		.AddDisplay(pangolin::Display("UndistortRight"))
		.AddDisplay(pangolin::Display("Depth"));

	// SRWorks Setup
	SEETHROUGH_MODULE_ID = seeThruMID;
	DEPTH_MODULE_ID = depthMID;
	RECONSTRUCT_MODULE_ID = reconsMID;
	
    

	// info of undistorted/distorted
    int undis_width, undis_height, undis_channel;
	ViveSR_GetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::OUTPUT_UNDISTORTED_WIDTH, &undis_width);
	ViveSR_GetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::OUTPUT_UNDISTORTED_HEIGHT, &undis_height);
	ViveSR_GetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::OUTPUT_UNDISTORTED_CHANNEL, &undis_channel);
	UndistImgInfo = std::to_string(undis_width) + "/" + std::to_string(undis_height) + "/" + std::to_string(undis_channel);
    int dis_width, dis_height, dis_channel;
	ViveSR_GetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::OUTPUT_DISTORTED_WIDTH, &dis_width);
	ViveSR_GetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::OUTPUT_DISTORTED_HEIGHT, &dis_height);
	ViveSR_GetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::OUTPUT_DISTORTED_CHANNEL, &dis_channel);
	DistImgInfo = std::to_string(dis_width) + "/" + std::to_string(dis_height) + "/" + std::to_string(dis_channel);
    int depth_width, depth_height;
	ViveSR_GetParameterInt(DEPTH_MODULE_ID, ViveSR::Depth::OUTPUT_WIDTH, &depth_width);
	ViveSR_GetParameterInt(DEPTH_MODULE_ID, ViveSR::Depth::OUTPUT_HEIGHT, &depth_height);
	ViveSR_SetParameterBool(DEPTH_MODULE_ID, ViveSR::Depth::ENABLE_REFINEMENT, EnableDepthRefine);
	DepthImgInfo = std::to_string(depth_width) + "/" + std::to_string(depth_height);

	// Get Recons Info
	{
		float temp;
		ViveSR_GetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ADAPTIVE_MIN_GRID, &temp);
		AdptMinGrid = temp * 100.0f;
		ViveSR_GetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ADAPTIVE_MAX_GRID, &temp);
		AdptMaxGrid = temp * 100.0f;
		ViveSR_GetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ADAPTIVE_ERROR_THRES, &temp);
		/* set scene understanding target candidate list size
		1. Floor		: 5 units
		2. Wall			: 5 units
		3. Ceiling		: 5 units
		4. Chair		: 5 units
		5. Table		: 5 units
		6. Bed			: 5 units
		7. Monitor		: 5 units
		8. Window		: 5 units
		9. Person		: 5 units
		10.Furniture	: 5 units
		11.Door			: 5 units
		*/
		SceneUnderstandingConfig config = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

		ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config,sizeof(config));
		ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_ENABLE, true);
		ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::SHOW_INFO, false);
	}

	{
		bool temp;
		ViveSR_GetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_MACHINE_VISION, &temp);
		semanticPreview = temp;
		ViveSR_GetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_REFINEMENT, &temp);
		semanticFilter = temp;
	}

	// init reconstruction display mode
	_SetDisplayMode();
	ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, EnableSector);

	// FPS init
	_hundredFrameStartTime_Render = std::chrono::steady_clock::now();
	_hundredFrameStartTime_SR = std::chrono::steady_clock::now();

	_initedAndProcessing = true;

    //Start see through thread
     see_through_thread_ = new std::thread(
         GetSeeThroughTask,
         SEETHROUGH_MODULE_ID,
         dis_width,
         dis_height,
         dis_channel,
         undis_width,
         undis_height,
         undis_channel
     );
     depth_map_thread_ = new std::thread(
         GetDepthMapParallelRun,
         DEPTH_MODULE_ID,
         depth_width,
         depth_height,
         1,
         4
     );
     reconstruction_thread_ = new std::thread(GetRigiReconstructionParallelRun, RECONSTRUCT_MODULE_ID);
}

void AppViewer::SetProjection(float focalLength)
{
#ifdef RENDER_WITH_RIGHT_HANDED
	glClearDepth(1);			// openGl default
	glDepthFunc(GL_LEQUAL);		// openGL default
	_SetPerspectiveRH(focalLength, (float)DEPTH_W / (float)DEPTH_H, 0.001f, 100.0f);

#else
	glClearDepth(0);
	glDepthFunc(GL_GEQUAL);
	_SetPerspectiveLH(focalLength, (float)DEPTH_W / (float)DEPTH_H, 0.001f, 100.0f);
#endif
}

void AppViewer::Process()
{
	while (_initedAndProcessing && !pangolin::ShouldQuit())
	{
		_BeginRender();

		_ProcessGeometries();

		_ProcessImages();

		_Render();

		_EndRender();

		_HandleInput();
	}

	// loop is broken by:
	// 1. closing pangolin window (ShouldQuit) or 
	// 2. closing the app console window (StopProcess is called)
	//    and returning of this function make sure the thread finishes
	_initedAndProcessing = false;
	_Terminate();
}

//////////////////////////////////////////////////////////////////////////
void AppViewer::_UpdateCameraPose(float *poseMtx, bool transpose)
{
	// poseMtx is row major....

	if (transpose)
	{
		_right[0] = poseMtx[0]; _right[1] = poseMtx[4]; _right[2] = poseMtx[8];
		_up[0] = poseMtx[1]; _up[1] = poseMtx[5]; _up[2] = poseMtx[9];
		_fwd[0] = poseMtx[2]; _fwd[1] = poseMtx[6]; _fwd[2] = poseMtx[10];
		_eye[0] = poseMtx[3]; _eye[1] = poseMtx[7]; _eye[2] = poseMtx[11];
	}
	else
	{
		_right[0] = poseMtx[0]; _right[1] = poseMtx[1]; _right[2] = poseMtx[2];
		_up[0] = poseMtx[4]; _up[1] = poseMtx[5]; _up[2] = poseMtx[6];
		_fwd[0] = poseMtx[8]; _fwd[1] = poseMtx[9]; _fwd[2] = poseMtx[10];
		_eye[0] = poseMtx[12]; _eye[1] = poseMtx[13]; _eye[2] = poseMtx[14];
	}

	// view mtx is column major, (and transpose rotate) 
	// --> row major to column major + transpose = canel the effect 
	_viewMtx[0] = _right[0]; _viewMtx[4] = _right[1]; _viewMtx[8] = _right[2];
	_viewMtx[1] = _up[0];    _viewMtx[5] = _up[1];    _viewMtx[9] = _up[2];
	_viewMtx[2] = _fwd[0];   _viewMtx[6] = _fwd[1];   _viewMtx[10] = _fwd[2];
	_viewMtx[3] = 0.0f;     _viewMtx[7] = 0.0f;     _viewMtx[11] = 0.0f;

	Normalize(_right);
	Normalize(_up);
	Normalize(_fwd);

	// put the viewer eye back-ward a little
	//eye[0] -= fwd[0] * 0.0f;
	//eye[1] -= fwd[1] * 0.0f;
	//eye[2] -= fwd[2] * 0.0f;

	// translate = -eye.dot.right / up / fwd
	_viewMtx[12] = -Dot(_eye, _right);
	_viewMtx[13] = -Dot(_eye, _up);
	_viewMtx[14] = -Dot(_eye, _fwd);
	_viewMtx[15] = 1.0f;
}

void AppViewer::_UpdateGeometry(float *in_pointsCloud, int in_numPoints, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index, int in_numIdx, bool enable_sector)
{
	if (enable_sector)
	{
		if (in_numIdx > 0)
		{
			_UpdateGeometry_Sector_WithIdx(in_pointsCloud, vertStride, sector_num, sector_id_list, sector_vert_num, sector_mesh_id_num, in_index);
		}
		else
		{
			_UpdateGeometry_Sector_NoIdx(in_pointsCloud, vertStride, sector_num, sector_id_list, sector_vert_num, sector_mesh_id_num, in_index);
		}
	}
	else
	{
		_UpdateGeometry_SingleCloud(0, in_pointsCloud, in_numPoints, vertStride, in_index, in_numIdx);
	}
}

void AppViewer::_UpdateGeometry_SingleCloud(int cloudId, float *in_pointsCloud, int in_numPoints, int vertStride, int *in_index, int in_numIdx)
{
	if (_allClouds.count(cloudId) == 0)
		_allClouds[cloudId] = new Clouds();

	_allClouds[cloudId]->UpdateData(in_pointsCloud, in_numPoints, vertStride, 3, in_index, in_numIdx);
}

void AppViewer::_UpdateGeometry_Sector_WithIdx(float *in_pointsCloud, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index)
{
	for (int i = 0, vert_id = 0, idx_id = 0; i < sector_num; i++)
	{
		int sector_id = sector_id_list[i];

		std::map<int, Clouds*>::iterator it = _allClouds.find(sector_id);
		if (it != _allClouds.end()) {
			delete it->second;
			_allClouds.erase(it);
		}

		int vert_num = sector_vert_num[i];
		int idx_num = sector_mesh_id_num[i];

		Clouds *cloud = new Clouds();
		cloud->UpdateData(&in_pointsCloud[vert_id * 4], vert_num, vertStride, 3, &in_index[idx_id], idx_num);
		_allClouds[sector_id] = cloud;

		vert_id += vert_num;
		idx_id += idx_num;
	}
}

void AppViewer::_UpdateGeometry_Sector_NoIdx(float *in_pointsCloud, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index)
{
	for (int i = 0, vert_id = 0; i < sector_num; i++)
	{
		int sector_id = sector_id_list[i];

		std::map<int, Clouds*>::iterator it = _allClouds.find(sector_id);
		if (it != _allClouds.end()) {
			delete it->second;
			_allClouds.erase(it);
		}

		int vert_num = sector_vert_num[i];

		Clouds *cloud = new Clouds();
		cloud->UpdateData(&in_pointsCloud[vert_id * 4], vert_num, vertStride, 3, in_index, 0);
		_allClouds[sector_id] = cloud;

		vert_id += vert_num;
	}
}

void AppViewer::_BeginRender()
{
	glClearColor(0.25, 0.25, 0.25, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pangolin::Display("scene").Activate();
}

void AppViewer::_ProcessGeometries()
{
	if (!DrawReconstruct || (!RunRigidReconstruction && !modelPreviewStarted)) return;

	static int lastNumVerts = -1;
	if (vive_api_mutex.try_lock())
	{
		if (vertices)
		{
			if (modelPreviewStarted)
			{
				_ProcessGeometries_ModelPreview();
			}
			else
			{
				_updateVertNum = *((int*)num_vert);
				_updateTriNum = *((int*)num_idx) / 3;
				if (lastNumVerts != _updateVertNum)
				{
					lastNumVerts = _updateVertNum;
					_UpdateGeometry((float*)vertices, _updateVertNum, *(int*)byte_vert, *(int*)ptr_sector_num, (int*)sector_id_list, (int*)sector_vertices_num, (int*)sector_indices_num, (int*)idx, *(int*)num_idx, EnableSector);
				}
			}
		}

		ViveSR_GetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::POINTCLOUD_POINTSIZE, &_pointSize);
		_pointSize = (_pointSize / 0.005f);

        vive_api_mutex.unlock();
	}
}

void AppViewer::_ProcessGeometries_ModelPreview()
{
	static int lastChunkId = -1;
	if (*(int*)model_chunk_num > 0)
	{
		if (lastChunkId == *(int*)model_chunk_idx)
			return;

		lastChunkId = *(int*)model_chunk_idx;
		_UpdateGeometry_SingleCloud(*(int*)model_chunk_idx, (float*)vertices, *(int*)num_vert, sizeof(float) * 4, (int*)idx, *(int*)num_idx);
		
		if (*(int*)model_chunk_num == *(int*)model_chunk_idx + 1) // finish
		{
			ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::MODEL_PREVIEW_FINISH, true);
			modelPreviewStarted = false;
			MeshPreviewPercent = std::to_string(100) + "%";
			lastChunkId = -1;
		}
		else if ((*(int*)model_chunk_num > *(int*)model_chunk_idx + 1)) // move to next chunk
		{
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::MODEL_PREVIEW_NEXT_CHUNK, true);
		}
	}
	else if(lastChunkId != -1)// abnormal finish
	{
		//ViveSR_SetCommandBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::EXTRACT_SCANNED_MODEL_FINISH, true);
		modelPreviewStarted = false;
		MeshPreviewPercent = std::to_string(100) + "%";
		lastChunkId = -1;
	}
}

void AppViewer::_ProcessImages()
{
    if (crop_through_mutex.try_lock()) {
        _UpdateCameraPose(camera_pose, false);
        _seeThruBGTex.Upload(cropSeeThruData, GL_RGBA, GL_UNSIGNED_BYTE);
        crop_through_mutex.unlock();
    }

	if (ShowDistort && see_through_mutex.try_lock())
	{
		_distortTex_L.Upload(distortData_L, GL_RGBA, GL_UNSIGNED_BYTE);
		_distortTex_R.Upload(distortData_R, GL_RGBA, GL_UNSIGNED_BYTE);
		see_through_mutex.unlock();
	}

	if (ShowUndistort && see_through_mutex.try_lock())
	{
		_undistortTex_L.Upload(undistortData_L, GL_RGBA, GL_UNSIGNED_BYTE);
		_undistortTex_R.Upload(undistortData_R, GL_RGBA, GL_UNSIGNED_BYTE);
        see_through_mutex.unlock();
	}

	if (ShowDepth && depthMutex.try_lock())
	{
		_depthColorMap->UploadTex(depthData, DEPTH_W, DEPTH_H);
		depthMutex.unlock();
	}
}

void AppViewer::_Render()
{
	_seeThruBGTex.RenderToViewport(true);

	if (DrawReconstruct)
	{
		if (_allClouds.size() != 0)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDepthMask(GL_TRUE);

			for (auto& cloud : _allClouds)
			{
				if (cloud.second != nullptr)
					cloud.second->DrawPoints(_projMtx, _viewMtx, _pointSize, _useWhiteColor);
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	glDisable(GL_DEPTH_TEST);

	if (ShowUndistort)
	{
		pangolin::Display("UndistortLeft").Activate();
		_undistortTex_L.RenderToViewport(true);

		pangolin::Display("UndistortRight").Activate();
		_undistortTex_R.RenderToViewport(true);
	}
	if (ShowDistort)
	{
		pangolin::Display("DistortLeft").Activate();
		_distortTex_L.RenderToViewport(true);

		pangolin::Display("DistortRight").Activate();
		_distortTex_R.RenderToViewport(true);
	}
	if (ShowDepth)
	{
		pangolin::Display("Depth").Activate();
		_depthColorMap->Draw();
	}

	glEnable(GL_DEPTH_TEST);
}

void AppViewer::_EndRender()
{
	pangolin::FinishFrame();

	// Refresh FPS
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	++_elapseFrameNum_Render;
	_hundredFrameElapseMS_Render = std::chrono::duration_cast<std::chrono::milliseconds>(now - _hundredFrameStartTime_Render).count();
	if (_elapseFrameNum_Render % 100 == 0) _hundredFrameStartTime_Render = now;

	_hundredFrameElapseMS_SR = std::chrono::duration_cast<std::chrono::milliseconds>(now - _hundredFrameStartTime_SR).count();
	_elapseFrameNum_SR = SRFrames - _restartFrame_SR;
	if (_elapseFrameNum_SR > 100)
	{
		_restartFrame_SR = SRFrames;
		_hundredFrameStartTime_SR = now;
	}
}

void AppViewer::_SetDisplayMode()
{
	if (_meshDisplayMode == 0) { _liteRaycast = true;	_liveAdaptive = false;	_livePC = false;	_useWhiteColor = 0.0f; }
	else if (_meshDisplayMode == 1) { _liteRaycast = false;	_liveAdaptive = true;	_livePC = false;	_useWhiteColor = 1.0f; }
	else { _liteRaycast = false;	_liveAdaptive = false;	_livePC = true;		_useWhiteColor = 0.0f; }
    std::lock_guard<std::mutex> lg(vive_api_mutex);
	ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::LITE_POINT_CLOUD_MODE, _liteRaycast);
	ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::LIVE_ADAPTIVE_MODE, _liveAdaptive);
	ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::FULL_POINT_CLOUD_MODE, _livePC);

	EnableSector = !_liteRaycast;
	DispMode = _liteRaycast ? "RaycastView" : (_liveAdaptive ? "AdaptiveMesh" : "PointCloud");
}

void AppViewer::_HandleInput()
{
    ViewerCurrentFPS = std::to_string(1000.0f * ((float)(_elapseFrameNum_Render % 100) / _hundredFrameElapseMS_Render));
    SRWorksFPS = std::to_string(1000.0f * ((float)_elapseFrameNum_SR / _hundredFrameElapseMS_SR));
    if (FPSControl.GuiChanged())
    {
        if (FPSControl == 60) {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(SEETHROUGH_MODULE_ID, ViveSR::ModuleParam::ENABLE_FPSCONTROL, false);
        }
        else
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(SEETHROUGH_MODULE_ID, ViveSR::ModuleParam::ENABLE_FPSCONTROL, true);
            ViveSR_SetParameterInt(SEETHROUGH_MODULE_ID, ViveSR::ModuleParam::SET_FPS, FPSControl);
        }
    }
    // See Thru Module
    if (pangolin::Pushed(SeeThruMODLabel))
    {
        static bool expand = SeeThruMODLabel;
        expand = !expand;
        _seeThruPanelHeight = (expand == true) ? _seeThruPanelMaxH : 0.04f;
        _ArrangePanel();
    }
    if (ShowUndistort.GuiChanged())
    {
        //TODO
        //if (ShowUndistort)	ViveSR_RegisterCallback(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::Callback::BASIC, GetUndistortCallback);
        //else				ViveSR_UnregisterCallback(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::Callback::BASIC, GetUndistortCallback);
    }
    if (ShowDistort.GuiChanged())
    {
        //TODO
        //if (ShowDistort)	ViveSR_RegisterCallback(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::Callback::BASIC, GetDistortCallback);
        //else				ViveSR_UnregisterCallback(SEETHROUGH_MODULE_ID, ViveSR::PassThrough::Callback::BASIC, GetDistortCallback);
    }
    // Depth Module
    if (pangolin::Pushed(DepthMODLabel))
    {
        static bool expand = DepthMODLabel;
        expand = !expand;
        _depthPanelHeight = (expand == true) ? _depthPanelMaxH : 0.04f;
        _ArrangePanel();
    }
    if (EnableDepthEngine.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_LinkModule(SEETHROUGH_MODULE_ID, DEPTH_MODULE_ID, EnableDepthEngine ? ViveSR::LinkType::ACTIVE : ViveSR::LinkType::NONE);
    }
    if (EnableDepthRefine.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterBool(DEPTH_MODULE_ID, ViveSR::Depth::Cmd::ENABLE_REFINEMENT, EnableDepthRefine);
    }
    if (EnableEdgeEnhance.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterBool(DEPTH_MODULE_ID, ViveSR::Depth::Cmd::ENABLE_EDGE_ENHANCE, EnableEdgeEnhance);
    }
    if (ShowDepth.GuiChanged())
    {
        //TODO
        //if (ShowDepth)		ViveSR_RegisterCallback(DEPTH_MODULE_ID, ViveSR::Depth::Callback::BASIC, GetDepthMapCallback);
        //else				ViveSR_UnregisterCallback(DEPTH_MODULE_ID, ViveSR::Depth::Callback::BASIC, GetDepthMapCallback);
    }
    // Reconstruction Module
    if (pangolin::Pushed(ReconsMODLabel))
    {
        static bool expand = ReconsMODLabel;
        expand = !expand;
        _reconsPanelHeight = (expand == true) ? _reconsPanelMaxH : 0.04f;
        _ArrangePanel();
    }
    static bool prevRunRecons = false;
    if (RunRigidReconstruction != prevRunRecons)
    {
        prevRunRecons = RunRigidReconstruction;
        if (RunRigidReconstruction)
        {
            _RemoveAllClouds(); // to clear model preview
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::START, true);	// send start cmd
        }
        else
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::STOP, true);	// send start cmd
            _RemoveAllClouds();
        }
    }
    if (DrawReconstruct.GuiChanged())
    {
        //TODO
        //if (DrawReconstruct)ViveSR_RegisterCallback(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Callback::BASIC, GetRigidReconstructionCallback);
        //else				ViveSR_UnregisterCallback(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Callback::BASIC, GetRigidReconstructionCallback);
    }
    if (pangolin::Pushed(SwitchGeoDisplayMode))
    {
        _meshDisplayMode = (_meshDisplayMode + 1) % 3;
        _SetDisplayMode();
    }


    if (pangolin::Pushed(SaveMeshButton))
    {
        exportPercentage = 0;
        char obj_name[] = "VivePro";
        //TODO
//        ViveSR_RegisterCallback(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Callback::EXPORT_PROGRESS, 0, 0, UpdateExportProgressCallback);
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::EXPORT_ADAPTIVE_MODEL, true);
        ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::EXPORT_MODEL_RIGHT_HAND, true);			// call save model to model name
        RunRigidReconstruction = false;
    }
    if (pangolin::Pushed(MeshPreviewButton))
    {
        if (DrawReconstruct && vertices && !modelPreviewStarted && reconsMutex.try_lock())
        {
            // switch to adaptive mesh mode
            static int last_meshDisplayMode = _meshDisplayMode;
            _meshDisplayMode = 1; // adaptive mesh
            _SetDisplayMode();

            modelPreviewStarted = true;
            renderMeshPercentage = 0;

            //            ViveSR_RegisterCallback(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Callback::EXPORT_PROGRESS, 0, 0, UpdateExtractProgressCallback);
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::EXPORT_ADAPTIVE_MODEL, true);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::MODEL_PREVIEW_START_RIGHT_HAND, true);
            RunRigidReconstruction = false;
            reconsMutex.unlock();
        }
    }
    // update percentage
    if (modelPreviewStarted)
    {
        MeshPreviewPercent = std::to_string(renderMeshPercentage) + "%";
    }
    else
    {
        ExportPercent = std::to_string(exportPercentage) + "%";
        VertexNum = std::to_string(_updateVertNum);
        TriangleNum = std::to_string(_updateTriNum);
    }
    if (AdptMinGrid.GuiChanged())
    {
        if (AdptMinGrid >= 50) AdptMinGrid = 64.0f;
        else if (AdptMinGrid >= 25) AdptMinGrid = 32.0f;
        else if (AdptMinGrid >= 12) AdptMinGrid = 16.0f;
        else if (AdptMinGrid >= 6) AdptMinGrid = 8.0f;
        else if (AdptMinGrid >= 3) AdptMinGrid = 4.0f;
        else AdptMinGrid = 2.0f;

        if (AdptMinGrid >= AdptMaxGrid) AdptMinGrid = AdptMaxGrid;
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ADAPTIVE_MIN_GRID, AdptMinGrid * 0.01f);
    }
    if (AdptMaxGrid.GuiChanged())
    {
        if (AdptMaxGrid >= 50) AdptMaxGrid = 64.0f;
        else if (AdptMaxGrid >= 25) AdptMaxGrid = 32.0f;
        else if (AdptMaxGrid >= 12) AdptMaxGrid = 16.0f;
        else if (AdptMaxGrid >= 6) AdptMaxGrid = 8.0f;
        else if (AdptMaxGrid >= 3) AdptMaxGrid = 4.0f;
        else AdptMaxGrid = 2.0f;

        if (AdptMaxGrid <= AdptMinGrid) AdptMaxGrid = AdptMinGrid;
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ADAPTIVE_MAX_GRID, AdptMaxGrid * 0.01f);

    }
    if (AdptDivideThre.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterFloat(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ADAPTIVE_ERROR_THRES, AdptDivideThre);
    }
    if (pangolin::Pushed(semanticLabel))
    {
        static bool expand = semanticLabel;
        expand = !expand;
        _semanticPanelHeight = (expand == true) ? _semanticPanelMaxH : 0.04f;
        _ArrangePanel();
    }
    if (semanticFilter.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_REFINEMENT, semanticFilter);
    }

    if (pangolin::Pushed(semanticPreview))
    {
        bool semanticVisualization; {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_GetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_MACHINE_VISION, &semanticVisualization);
        }

        if (!semanticVisualization)
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            // scene understanding requires the following properties be set correspondingly
            _useWhiteColor = 0.0f;
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, false);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, false);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::LITE_POINT_CLOUD_MODE, false);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::LIVE_ADAPTIVE_MODE, true);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::FULL_POINT_CLOUD_MODE, false);

            // 			int res = ViveSR_RegisterCallback(
            //                 RECONSTRUCT_MODULE_ID,
            //                 (int)ViveSR::RigidReconstruction::Callback::SCENE_UNDERSTANDING_PROGRESS,
            //                 0,0,
            //                 UpdateSemanticProgress);
        }
        else
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, true);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, true);
            _useWhiteColor = 1.0f; // resume mesh color to be white when semantic preview is off
            semanticProgressUI = "";
        }
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_MACHINE_VISION, !semanticVisualization);
        }
        if (!semanticVisualization)
        {
            semanticProgressUI = "Live";
        }

    }
    if (pangolin::Pushed(semanticExport))
    {
        bool bEnableSectorGrouping, bEnableFrustumCulling;
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_GetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, &bEnableSectorGrouping);
            ViveSR_GetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, &bEnableFrustumCulling);

            // scene understanding is incompatible with these following functionalities
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, false);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, false);
        }

        //         ViveSR_RegisterCallback(
        //             RECONSTRUCT_MODULE_ID,
        //             (int)ViveSR::RigidReconstruction::Callback::SCENE_UNDERSTANDING_PROGRESS,
        //             0, 0,
        //             UpdateSemanticProgress);

        char obj_name[] = "SceneObjList";
        {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Cmd::EXPORT_SCENE_UNDERSTANDING_FOR_UNITY, true);
        }
        // get progress bar from event listener
        std::thread thd([&, bEnableSectorGrouping, bEnableFrustumCulling] {
            semanticExportNetProgress = 0; // reset before querying progress 
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                semanticProgressUI = "Processing: " + std::to_string(semanticExportNetProgress) + " %";

            } while (semanticExportNetProgress < 100);
            semanticProgressUI = "Export complete";
            {
                std::lock_guard<std::mutex> lg(vive_api_mutex);
                ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, bEnableSectorGrouping);
                ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_FRUSTUM_CULLING, bEnableFrustumCulling);
            }
        });
        thd.detach();
    }
    if (semanticChair.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;

        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);

        if (semanticChair) config.nChairMaxInst = 5;
        else config.nChairMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, pConfig, size_of_config);
    }
    if (semanticFloor.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);

        if (semanticFloor) config.nFloorMaxInst = 5;
        else config.nFloorMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticWall.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);

        if (semanticWall) config.nWallMaxInst = 5;
        else config.nWallMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticCeiling.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);

        if (semanticCeiling) config.nCeilingMaxInst = 5;
        else config.nCeilingMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticTable.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);

        if (semanticTable) config.nTableMaxInst = 5;
        else config.nTableMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticBed.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);

        if (semanticBed) config.nBedMaxInst = 5;
        else config.nBedMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticMonitor.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);


        if (semanticMonitor) config.nMonitorMaxInst = 5;
        else config.nMonitorMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticWindow.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);


        if (semanticWindow) config.nWindowMaxInst = 5;
        else config.nWindowMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticPerson.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);


        if (semanticPerson) config.nPersonMaxInst = 5;
        else config.nPersonMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticFurniture.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);


        if (semanticFurniture) config.nFurnitureMaxInst = 5;
        else config.nFurnitureMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }
    if (semanticDoor.GuiChanged())
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        SceneUnderstandingConfig config;
        SceneUnderstandingConfig* pConfig = &config;
        int size_of_config = 0;
        ViveSR_GetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, reinterpret_cast<void **>(&pConfig), &size_of_config);


        if (semanticDoor) config.nDoorMaxInst = 5;
        else config.nDoorMaxInst = 0;

        ViveSR_SetParameterStruct(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::SCENE_UNDERSTANDING_CONFIG, &config, size_of_config);
    }

    // sync state between engine and app
    bool engineEnableSector;
    {
        std::lock_guard<std::mutex> lg(vive_api_mutex);
        ViveSR_GetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, &engineEnableSector);
    }
	if (engineEnableSector != EnableSector)
	{
		if (EnableSector.GuiChanged()) {
            std::lock_guard<std::mutex> lg(vive_api_mutex);
            ViveSR_SetParameterBool(RECONSTRUCT_MODULE_ID, ViveSR::RigidReconstruction::Param::ENABLE_SECTOR_GROUPING, EnableSector); 
        }
		else { EnableSector = engineEnableSector; EnableSector.GuiChanged(); }// the trick here is: for not triggering another GuiChanged() due to assignment, trigger it here to avoid further complications
		_RemoveAllClouds();
	}
}

void AppViewer::_RemoveAllClouds()
{
	for (auto& cloud : _allClouds)
		delete cloud.second;
	_allClouds.clear();
}

void AppViewer::_SetPerspectiveLH(float focalLength, float aspect, float nearPlane, float farPlane)
{
	float fovRad = 2 * atanf((float)DEPTH_H / (2.0f * focalLength));

	float tanHalfFovy = tan(fovRad * 0.5f);
	_projMtx[0] = 1.0f / (aspect * tanHalfFovy);
	_projMtx[5] = 1.0f / (tanHalfFovy);
	_projMtx[10] = farPlane / (farPlane - nearPlane);
	_projMtx[11] = 1.0f;
	_projMtx[14] = -(farPlane * nearPlane) / (farPlane - nearPlane);
	_projMtx[15] = 0.0f;
}

void AppViewer::_SetPerspectiveRH(float focalLength, float aspect, float nearPlane, float farPlane)
{
	float fovRad = 2 * atanf((float)DEPTH_H / (2.0f * focalLength));

	float tanHalfFovy = tan(fovRad * 0.5f);
	_projMtx[0] = 1.0f / (aspect * tanHalfFovy);
	_projMtx[5] = 1.0f / (tanHalfFovy);
	_projMtx[10] = farPlane / (farPlane - nearPlane);
	_projMtx[11] = -1.0f;
	_projMtx[14] = (farPlane * nearPlane) / (farPlane - nearPlane);
	_projMtx[15] = 0.0f;
}

void AppViewer::_ArrangePanel()
{
	float curTOP = _moduleUITop - 0.003f;
	float curBTM = curTOP - _seeThruPanelHeight;
	_seeThruPanel->SetBounds(curBTM, curTOP, 0.0, pangolin::Attach::Pix(250));

	curTOP = curBTM - 0.003f;
	curBTM = curTOP - _depthPanelHeight;
	_depthPanel->SetBounds(curBTM, curTOP, 0.0, pangolin::Attach::Pix(250));

	curTOP = curBTM - 0.003f;
	curBTM = curTOP - _reconsPanelHeight;
	_reconsPanel->SetBounds(std::max(curBTM, 0.0f), curTOP, 0.0, pangolin::Attach::Pix(250));

	curTOP = curBTM - 0.003f;
	curBTM = curTOP - _semanticPanelHeight;
	_semanticPanel->SetBounds(std::max(curBTM, 0.0f), std::max(curTOP, 0.0f), 0.0, pangolin::Attach::Pix(250));

	curTOP = curBTM - 0.003f;
	_emptyPanel->SetBounds(0.0, std::max(curTOP, 0.0f), 0.0, pangolin::Attach::Pix(250));
}

void AppViewer::_Terminate()
{
    _StopAllThread();
	_seeThruPanel = _depthPanel = _reconsPanel = _semanticPanel = _emptyPanel = nullptr;
	_RemoveAllClouds();

	_seeThruBGTex.Delete();
	_distortTex_L.Delete();
	_distortTex_R.Delete();
	_undistortTex_L.Delete();
	_undistortTex_R.Delete();
	delete _depthColorMap;
	_depthColorMap = nullptr;
}

void AppViewer::_StopAllThread()
{
    g_get_seethrough_threading = false;
    see_through_thread_->join();
    delete see_through_thread_;
    g_get_depth_map_threading = false;
    depth_map_thread_->join();
    delete depth_map_thread_;
    g_get_reconstruction_threading = false;
    reconstruction_thread_->join();
    delete reconstruction_thread_;
}

