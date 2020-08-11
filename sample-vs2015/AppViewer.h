#pragma once

class AppViewer
{
public:
	AppViewer() {};
	~AppViewer() {};

	void Initialize(int seeThruMID, int depthMID, int reconsMID);
	void SetProjection(float focalLength);
	void Process();
	void StopProcess() { _initedAndProcessing = false; };

private:
	void _UpdateCameraPose(float *poseMtx, bool transpose);

	void _UpdateGeometry(float *in_pointsCloud, int in_numPoints, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index, int in_numIdx, bool enable_sector);
	void _UpdateGeometry_SingleCloud(int cloudId, float *in_pointsCloud, int in_numPoints, int vertStride, int *in_index, int in_numIdx);
	void _UpdateGeometry_Sector_WithIdx(float *in_pointsCloud, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index);
	void _UpdateGeometry_Sector_NoIdx(float *in_pointsCloud, int vertStride, int sector_num, int *sector_id_list, int *sector_vert_num, int *sector_mesh_id_num, int *in_index);

	void _BeginRender();
	void _ProcessGeometries();
	void _ProcessGeometries_ModelPreview();
	void _ProcessImages();
	void _Render();
	void _EndRender();

	void _SetDisplayMode();
	void _HandleInput();
	void _RemoveAllClouds();

	void _SetPerspectiveLH(float focalLength, float aspect, float nearPlane, float farPlane);
	void _SetPerspectiveRH(float focalLength, float aspect, float nearPlane, float farPlane);	
	void _ArrangePanel();
	void _Terminate();
    void _StopAllThread();
private:
	int SEETHROUGH_MODULE_ID;
	int DEPTH_MODULE_ID;
	int AI_Vision_MODULE_ID;
	int RECONSTRUCT_MODULE_ID;

private:
	bool _initedAndProcessing = false;
	float _eye[3], _right[3], _fwd[3], _up[3];
	float _projMtx[16];
	float _viewMtx[16];

	pangolin::View* _seeThruPanel = nullptr;
	pangolin::View* _depthPanel = nullptr;
	pangolin::View* _reconsPanel = nullptr;	
	pangolin::View* _semanticPanel = nullptr;
	pangolin::View* _emptyPanel = nullptr;
	float _moduleUITop = 0.84f;
	float _seeThruPanelMaxH = 0.2f;
	float _depthPanelMaxH = 0.23f;
	float _reconsPanelMaxH = 0.49f;
	float _semanticPanelMaxH = 0.3f;		// modify

	float _seeThruPanelHeight;
	float _depthPanelHeight;
	float _reconsPanelHeight;
	float _semanticPanelHeight;

	pangolin::GlTexture _seeThruBGTex;
	pangolin::GlTexture _distortTex_L;
	pangolin::GlTexture _distortTex_R;
	pangolin::GlTexture _undistortTex_L;
	pangolin::GlTexture _undistortTex_R;
	DepthColorTex* _depthColorMap = nullptr;

	// renderer fps
	std::chrono::steady_clock::time_point _hundredFrameStartTime_Render;
	long long _hundredFrameElapseMS_Render = 0;
	long long _elapseFrameNum_Render = 0;
	// sr works fps
	std::chrono::steady_clock::time_point _hundredFrameStartTime_SR;
	long long _hundredFrameElapseMS_SR = 0;
	long long _restartFrame_SR = 0;
	long long _elapseFrameNum_SR = 0;

	//
	std::map<int, Clouds*> _allClouds;	
	int _updateVertNum = 0;
	int _updateTriNum = 0;

	int _meshDisplayMode = 1;	// 0: RayCastView, 1: Mesh, 2: PointCloud	
	bool _liteRaycast = false;
	bool _liveAdaptive = false;
	bool _livePC = false;	

	float _pointSize = 8.0f;
	bool _useWhiteColor = false;	

    std::future<void> _semanticExportCbFut;
    std::thread* see_through_thread_ = nullptr;
    std::thread* depth_map_thread_ = nullptr;
    std::thread* reconstruction_thread_ = nullptr;
};