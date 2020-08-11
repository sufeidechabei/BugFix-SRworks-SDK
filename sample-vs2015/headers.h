#pragma once

#include <opencv.hpp>
#include <Windows.h>
#include <chrono>
#include <mutex>
#include <thread>
#include <future>

#include "ViveSR_Enums.h"
#include "ViveSR_API_Enums.h"
#include "ViveSR_Client.h"
#include "ViveSR_Structs.h"
#include "ViveSR_PassThroughEnums.h"
#include "ViveSR_DepthEnums.h"
#include "ViveSR_RigidReconstructionEnums.h"
#include "Semantic_Utility.h"

#include <GL/glew.h>
#include <pangolin/pangolin.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/gldraw.h>
#include "GraphicElement.h"
#include "AppViewer.h"