//! @file substance_public.h
//! @brief Substance Unreal Engine 4 API File
//! @author Josh Coyne - Allegorithmic
//! @copyright 2014 Allegorithmic Inc. All rights reserved.
#pragma once

#define WITH_SUBSTANCE				1
#define SUBSTANCE_PLATFORM_BLEND	1

#ifdef __cplusplus
#	define SUBSTANCE_EXTERNC extern "C"
#else
#	define SUBSTANCE_EXTERNC
#endif

#define SUBSTANCE_LIB_STATIC

#if defined (SUBSTANCE_LIB_STATIC)
#	define	SUBSTANCELIB_EXPORT
#else
#	if defined (SUBSTANCE_LIB)
#		define SUBSTANCELIB_EXPORT __declspec(dllexport)
#	else
#		define SUBSTANCELIB_EXPORT __declspec(dllimport)
#	endif
#endif

#define SUBSTANCELIB_API SUBSTANCE_EXTERNC SUBSTANCELIB_EXPORT
#define SUBSTANCELIB_CLASS SUBSTANCELIB_EXPORT

#if defined (SUBSTANCE_LIB)

#include "substance_types.h"
#include "substance_private.h"

#else

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#	define SUBSTANCE_CALLBACK __stdcall
#else
#	define SUBSTANCE_CALLBACK
#endif

typedef PTRINT SubstanceContext;
typedef PTRINT SubstanceHandle;
typedef PTRINT SubstanceLinkerContext;
typedef PTRINT SubstanceLinkerHandle;

typedef PTRINT XMLElementHandle;

#define SUBSTANCE_GPU_COUNT_MAX 4
#define SUBSTANCE_CPU_COUNT_MAX 32
#define SUBSTANCE_SPU_COUNT_MAX 32
#define SUBSTANCE_MEMORYBUDGET_FORCEFLUSH 1

typedef enum
{
	Substance_Resource_Default = 0,      /**< Default value on this platform */
	Substance_Resource_DoNotUse,         /**< The resource must not be used */
	Substance_Resource_Partial1,         /**< Resource partial use: 1/8*/
	Substance_Resource_Partial2,         /**< Partial use 2/8 */
	Substance_Resource_Partial3,         /**< Partial use 3/8 */
	Substance_Resource_Partial4,         /**< Partial use 4/8 */
	Substance_Resource_Partial5,         /**< Partial use 5/8 */
	Substance_Resource_Partial6,         /**< Partial use 6/8 */
	Substance_Resource_Partial7,         /**< Resource partial use: 7/8*/
	Substance_Resource_FullUse,          /**< The resource can be used 100% */

} SubstanceHardSwitch;

typedef enum
{
	Substance_ChanOrder_NC = 0,
	Substance_ChanOrder_BGRA = 0xC6,
} SubstanceChannelsOrder;

typedef enum
{
	Substance_IType_Float = 0x0,
	Substance_IType_Float2 = 0x1,
	Substance_IType_Float3 = 0x2,
	Substance_IType_Float4 = 0x3,
	Substance_IType_Integer = 0x4,
	Substance_IType_Integer2 = 0x8,
	Substance_IType_Integer3 = 0x9,
	Substance_IType_Integer4 = 0xA,
	Substance_IType_Image = 0x5,
	Substance_IType_Mesh = 0x6,
	Substance_IType_String = 0x7
} SubstanceInputType;

typedef enum
{
	Substance_Linker_UIDCollision_Output = 0x0,
	Substance_Linker_UIDCollision_Input = 0x1   
} SubstanceLinkerUIDCollisionType;

typedef enum
{
	Substance_PF_RAW = 0x0,
	Substance_PF_DXT = 0x1,
	Substance_PF_PVRTC = 0x3,
	Substance_PF_ETC = 0x3,
	Substance_PF_Misc = 0x2,
	Substance_PF_RGBA = Substance_PF_RAW | 0x0,
	Substance_PF_RGBx = Substance_PF_RAW | 0x4,
	Substance_PF_RGB = Substance_PF_RAW | 0x8,
	Substance_PF_L = Substance_PF_RAW | 0xC,
	Substance_PF_16b = Substance_PF_RAW | 0x10,
	Substance_PF_DXT1 = Substance_PF_DXT | 0x0,
	Substance_PF_DXT2 = Substance_PF_DXT | 0x4,
	Substance_PF_DXT3 = Substance_PF_DXT | 0x8,
	Substance_PF_DXT4 = Substance_PF_DXT | 0xC,
	Substance_PF_DXT5 = Substance_PF_DXT | 0x10,
	Substance_PF_DXTn = Substance_PF_DXT | 0x14,
	Substance_PF_PVRTC2 = Substance_PF_PVRTC | 0x0,
	Substance_PF_PVRTC4 = Substance_PF_PVRTC | 0x4,
	Substance_PF_ETC1 = Substance_PF_ETC | 0x8,
	Substance_PF_JPEG = Substance_PF_Misc | 0x0,
	Substance_PF_sRGB = 0x20,
} SubstancePixelFormat;

typedef struct SubstanceTexture_
{
	void *buffer;
	unsigned short level0Width;
	unsigned short level0Height;
	unsigned char pixelFormat;
	unsigned char channelsOrder;
	unsigned char mipmapCount;
} SubstanceTexture;

typedef struct SubstanceTextureInput_
{
	SubstanceTexture mTexture;
	unsigned int level0Width;
	unsigned int level0Height;
	unsigned char pixelFormat;
	unsigned char mipmapCount;
} SubstanceTextureInput;

typedef struct SubstanceHardResources_
{
	unsigned char gpusUse[SUBSTANCE_GPU_COUNT_MAX];
	unsigned char cpusUse[SUBSTANCE_CPU_COUNT_MAX];
	unsigned char spusUse[SUBSTANCE_SPU_COUNT_MAX];
	size_t systemMemoryBudget;
	size_t videoMemoryBudget[SUBSTANCE_GPU_COUNT_MAX];
	
} SubstanceHardResources;

typedef struct SubstanceDataDesc_
{
	unsigned int outputsCount;
	unsigned int inputsCount;
	unsigned short formatVersion;
	unsigned char platformId;

} SubstanceDataDesc;

typedef enum
{
	Substance_Sync_Synchronous  = 0x0,
	Substance_Sync_Asynchronous = 0x1,

} SubstanceSyncOption;

typedef struct SubstanceInputDesc_
{
	unsigned int inputId;
	SubstanceInputType inputType;
	union
	{
		const float *typeFloatX;
		const int *typeIntegerX;
		const char **typeString;

	} value;

} SubstanceInputDesc;

typedef enum
{
	Substance_OState_NotComputed,         /**< Output not ready */
	Substance_OState_Ready,               /**< Output computed and ready */
	Substance_OState_Grabbed              /**< Output already grabbed by the user */

} SubstanceOutputState;

typedef struct SubstanceOutputDesc_
{
	unsigned int outputId;
	unsigned int level0Width;
	unsigned int level0Height;
	unsigned char pixelFormat;
	unsigned char mipmapCount;
	SubstanceOutputState state;
} SubstanceOutputDesc;

typedef enum
{
	Substance_State_Started = 0x1,
	Substance_State_Asynchronous = 0x2,
	Substance_State_EndingWaiting = 0x4,
	Substance_State_NoMoreRender = 0x8,
	Substance_State_NoMoreRsc = 0x10,
	Substance_State_PendingCommand = 0x20,

} SubstanceStateFlag;

typedef enum
{
	Substance_Linker_Select_Unselect = 0x0,
	Substance_Linker_Select_Select = 0x1,
	Substance_Linker_Select_UnselectAll = 0x2
} SubstanceLinkerSelect;

#endif

typedef enum
{
	Substance_Linker_Context_Init_Auto,
	Substance_Linker_Context_Init_Mobile,
	Substance_Linker_Context_Init_NonMobile,
} SubstanceLinkerContextInitEnum;

typedef void (*SubstanceInputOutputCallback)(void* userdata, unsigned int* inputIds, size_t inputCount, unsigned int* outputIds, size_t outputCount);

//API
SUBSTANCELIB_API unsigned int SubstanceContextInit(SubstanceContext** context, void* callbackOutputCompleted, void* callbackJobCompleted, void* callbackInputImageLock, void* callbackInputImageUnlock, void* callbackMalloc, void* callbackFree);
SUBSTANCELIB_API unsigned int SubstanceContextMemoryFree(SubstanceContext* context, void* data);
SUBSTANCELIB_API unsigned int SubstanceContextRelease(SubstanceContext* context);
SUBSTANCELIB_API unsigned int SubstanceHandleCompute(SubstanceHandle* handle);
SUBSTANCELIB_API unsigned int SubstanceHandleFlush(SubstanceHandle* handle);
SUBSTANCELIB_API unsigned int SubstanceHandleGetDesc(SubstanceHandle* handle, SubstanceDataDesc* dataDesc);
SUBSTANCELIB_API unsigned int SubstanceHandleGetInputDesc(SubstanceHandle* handle, unsigned int inputIdx, SubstanceInputDesc* substanceInputDesc);
SUBSTANCELIB_API unsigned int SubstanceHandleGetOutputDesc(SubstanceHandle* handle, unsigned int outputIdx, SubstanceOutputDesc* substanceOutputDesc);
SUBSTANCELIB_API unsigned int SubstanceHandleGetState(SubstanceHandle* handle, unsigned int* state);
SUBSTANCELIB_API unsigned int SubstanceHandleGetTexture(SubstanceHandle* handle, unsigned int outputIndex, SubstanceTexture* textureOut, SubstanceContext** contextOut);
SUBSTANCELIB_API unsigned int SubstanceHandleInit(SubstanceHandle** handle, SubstanceContext* context, unsigned char* data, size_t dataSize, SubstanceHardResources& hardrsc, size_t userData);
SUBSTANCELIB_API unsigned int SubstanceHandleInputOutputCallback(SubstanceHandle* handle, SubstanceInputOutputCallback callback, void* userdata);
SUBSTANCELIB_API unsigned int SubstanceHandlePushOutputs(SubstanceHandle* handle, const unsigned int* outputs, unsigned int outputCount, size_t userData);
SUBSTANCELIB_API unsigned int SubstanceHandlePushOutputsHint(SubstanceHandle* handle, const unsigned int* outputs, unsigned int outputCount);
SUBSTANCELIB_API unsigned int SubstanceHandlePushSetInput(SubstanceHandle* handle, unsigned int index, SubstanceInputType type, void* value, size_t userData);
SUBSTANCELIB_API unsigned int SubstanceHandlePushSetInputHint(SubstanceHandle* handle, unsigned int index, SubstanceInputType type);
SUBSTANCELIB_API unsigned int SubstanceHandleRelease(SubstanceHandle* handle);
SUBSTANCELIB_API unsigned int SubstanceHandleStart(SubstanceHandle* handle);
SUBSTANCELIB_API unsigned int SubstanceHandleStop(SubstanceHandle* handle);
SUBSTANCELIB_API unsigned int SubstanceHandleSwitchHard(SubstanceHandle* handle, SubstanceSyncOption syncMode, const SubstanceHardResources* substanceRscNew);
SUBSTANCELIB_API unsigned int SubstanceHandleTransferCache(SubstanceHandle* from, SubstanceHandle* to, const unsigned char* data);
SUBSTANCELIB_API unsigned int SubstanceLinkerConnectOutputToInput(SubstanceLinkerHandle* handle, unsigned int input, unsigned int output);
SUBSTANCELIB_API unsigned int SubstanceLinkerContextInit(SubstanceLinkerContext** context, SubstanceLinkerContextInitEnum initEnum);
SUBSTANCELIB_API unsigned int SubstanceLinkerContextSetXMLCallback(SubstanceLinkerContext* context, void* callbackPtr);
SUBSTANCELIB_API unsigned int SubstanceLinkerContextSetUIDCollisionCallback(SubstanceLinkerContext* context, void* callbackPtr);
SUBSTANCELIB_API unsigned int SubstanceLinkerEnableOutputs(SubstanceLinkerHandle* handle, unsigned int* enabledIds, size_t enabledCount);
SUBSTANCELIB_API unsigned int SubstanceLinkerGetCacheMapping(SubstanceLinkerHandle* handle, const unsigned char** data, const unsigned char* prevData);
SUBSTANCELIB_API unsigned int SubstanceLinkerGetUserData(SubstanceLinkerHandle* handle, size_t* userdata);
SUBSTANCELIB_API unsigned int SubstanceLinkerFuseInputs(SubstanceLinkerHandle* handle, unsigned int input0, unsigned int input1);
SUBSTANCELIB_API unsigned int SubstanceLinkerHandleInit(SubstanceLinkerHandle** handle, SubstanceLinkerContext* context, size_t userData);
SUBSTANCELIB_API unsigned int SubstanceLinkerLink(SubstanceLinkerHandle* handle, const unsigned char** data, size_t* dataSize);
SUBSTANCELIB_API unsigned int SubstanceLinkerPushAssembly(SubstanceLinkerHandle* handle, unsigned int uid, const char* data, size_t dataSize);
SUBSTANCELIB_API unsigned int SubstanceLinkerPushMemory(SubstanceLinkerHandle* handle, const char* data, size_t dataSize);
SUBSTANCELIB_API unsigned int SubstanceLinkerRelease(SubstanceLinkerHandle* handle, SubstanceLinkerContext* context);
SUBSTANCELIB_API unsigned int SubstanceLinkerSelectOutputs(SubstanceLinkerHandle* handle, SubstanceLinkerSelect flag, unsigned int idxOut);
SUBSTANCELIB_API unsigned int SubstanceLinkerSetOutputFormat(SubstanceLinkerHandle* handle, unsigned int uid, unsigned int format, int mips);
SUBSTANCELIB_API unsigned int SubstanceLinkerSetUserData(SubstanceLinkerHandle* handle, size_t userData);

namespace Substance
{
	class XMLNode;
	class XMLFileImpl;

	class SUBSTANCELIB_CLASS XMLFile
	{
	public:
		XMLFile(const char* buffer);
		~XMLFile();

		const XMLNode* GetRootNode() const;

		bool IsValid() const;

		inline XMLFileImpl* GetImpl() const { return mImpl; }
		
	protected:
		XMLFile(const XMLFile&) = delete;
		XMLFile& operator=(const XMLFile&) = delete;

	private:
		XMLFileImpl*			mImpl;
	};

	class SUBSTANCELIB_CLASS XMLNode
	{
	public:
		XMLNode(XMLFile* parent, XMLElementHandle handle);

		const XMLNode* FindChildNode(const char* tag) const;

		const char* GetAttribute(const char* tag) const;
		const XMLNode* GetFirstChildNode() const;
		const XMLNode* GetNextNode() const;

		inline XMLElementHandle GetHandle() const { return mElement; }

	private:
		XMLNode(const XMLNode&) = delete;
		XMLNode& operator=(const XMLNode&) = delete;

	private:
		XMLFile*					mParent;
		XMLElementHandle			mElement;
	};
};
