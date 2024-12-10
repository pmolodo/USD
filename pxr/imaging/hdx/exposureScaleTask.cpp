//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/exposureScaleTask.h"
#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((exposureScaleFrag, "ExposureScaleFragment"))
);

HdxExposureScaleTask::HdxExposureScaleTask(
    HdSceneDelegate* delegate,
    SdfPath const& id)
  : HdxTask(id)
{
}

HdxExposureScaleTask::~HdxExposureScaleTask() = default;

void
HdxExposureScaleTask::_Sync(HdSceneDelegate* delegate,
                           HdTaskContext* ctx,
                           HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_compositor) {
        _compositor = std::make_unique<HdxFullscreenShader>(
            _GetHgi(), "ExposureScale");
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxExposureScaleTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _cameraPath = params.cameraPath;
        }
    }

    // Currently, we're querying GetExposureScale() every frame - not
    // sure if there's a better way to detect if this is dirty?
    if (_cameraPath.IsEmpty()) {
        _exposureScale = 1.0f;
    } else {
        HdRenderIndex &renderIndex = delegate->GetRenderIndex();
        const HdCamera *camera = static_cast<const HdCamera *>(
            renderIndex.GetSprim(HdPrimTypeTokens->camera, _cameraPath));
        if (!TF_VERIFY(camera)) {
            return;
        }

        _exposureScale = camera->GetExposureScale();
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxExposureScaleTask::Prepare(HdTaskContext* ctx,
                             HdRenderIndex* renderIndex)
{
}

void
HdxExposureScaleTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HgiTextureHandle aovTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);

    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = _tokens->exposureScaleFrag.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn");
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");

    // The order of the constant parameters has to match the order in the
    // _ParameterBuffer struct
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "screenSize", "vec2");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "exposureScale", "float");

    _compositor->SetProgram(
        HdxPackageExposureScaleShader(),
        _tokens->exposureScaleFrag,
        fragDesc);
    const auto &aovDesc = aovTexture->GetDescriptor();
    if (_UpdateParameterBuffer(
            static_cast<float>(aovDesc.dimensions[0]),
            static_cast<float>(aovDesc.dimensions[1]))) {
        size_t byteSize = sizeof(_ParameterBuffer);
        _compositor->SetShaderConstants(byteSize, &_parameterData);
    }

    _compositor->BindTextures({aovTexture});

    _compositor->Draw(aovTexture, /*no depth*/HgiTextureHandle());
}

bool
HdxExposureScaleTask::_UpdateParameterBuffer(
    float screenSizeX, float screenSizeY)
{
    _ParameterBuffer pb;

    pb.exposureScale = _exposureScale;
    pb.screenSize[0] = screenSizeX;
    pb.screenSize[1] = screenSizeY;

    // All data is still the same, no need to update the storage buffer
    if (pb == _parameterData) {
        return false;
    }

    _parameterData = pb;

    return true;
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(
    std::ostream& out,
    const HdxExposureScaleTaskParams& pv)
{
    out << "ExposureScaleTask Params: (...) "
        << pv.cameraPath << " "
    ;
    return out;
}

bool operator==(const HdxExposureScaleTaskParams& lhs,
                const HdxExposureScaleTaskParams& rhs)
{
    return lhs.cameraPath == rhs.cameraPath;
}

bool operator!=(const HdxExposureScaleTaskParams& lhs,
                const HdxExposureScaleTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
