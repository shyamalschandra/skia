/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GrGLVertexShaderBuilder.h"
#include "GrGLProgramBuilder.h"
#include "GrGLShaderStringBuilder.h"
#include "../GrGLGpu.h"

#define GL_CALL(X) GR_GL_CALL(fProgramBuilder->gpu()->glInterface(), X)
#define GL_CALL_RET(R, X) GR_GL_CALL_RET(fProgramBuilder->gpu()->glInterface(), R, X)

GrGLVertexBuilder::GrGLVertexBuilder(GrGLProgramBuilder* program)
    : INHERITED(program)
    , fRtAdjustName(NULL) {
}

void GrGLVertexBuilder::addVarying(const char* name, GrGLVarying* v) {
    fOutputs.push_back();
    fOutputs.back().setType(v->fType);
    fOutputs.back().setTypeModifier(GrGLShaderVar::kVaryingOut_TypeModifier);
    fProgramBuilder->nameVariable(fOutputs.back().accessName(), 'v', name);
    v->fVsOut = fOutputs.back().getName().c_str();
}

void GrGLVertexBuilder::emitAttributes(const GrGeometryProcessor& gp) {
    int vaCount = gp.numAttribs();
    for (int i = 0; i < vaCount; i++) {
        this->addAttribute(&gp.getAttrib(i));
    }
    return;
}

void GrGLVertexBuilder::transformToNormalizedDeviceSpace(const GrShaderVar& posVar) {
    SkASSERT(!fRtAdjustName);

    // setup RT Uniform
    fProgramBuilder->fUniformHandles.fRTAdjustmentUni =
            fProgramBuilder->addUniform(GrGLProgramBuilder::kVertex_Visibility,
                                        kVec4f_GrSLType, kDefault_GrSLPrecision,
                                        fProgramBuilder->rtAdjustment(),
                                        &fRtAdjustName);

    // Transform from Skia's device coords to GL's normalized device coords. Note that
    // because we want to "nudge" the device space positions we are converting to 
    // non-homogeneous NDC.
    if (kVec3f_GrSLType == posVar.getType()) {
        this->codeAppendf("gl_Position = vec4(dot(%s.xz, %s.xy)/%s.z, dot(%s.yz, %s.zw)/%s.z, 0, 1);",
                          posVar.c_str(), fRtAdjustName, posVar.c_str(),
                          posVar.c_str(), fRtAdjustName, posVar.c_str());
    } else {
        SkASSERT(kVec2f_GrSLType == posVar.getType());
        this->codeAppendf("gl_Position = vec4(%s.x * %s.x + %s.y, %s.y * %s.z + %s.w, 0, 1);",
                          posVar.c_str(), fRtAdjustName, fRtAdjustName,
                          posVar.c_str(), fRtAdjustName, fRtAdjustName);
    }

    // We could have the GrGeometryProcessor do this, but its just easier to have it performed here.
    // If we ever need to set variable pointsize, then we can reinvestigate
    this->codeAppend("gl_PointSize = 1.0;");
}

void GrGLVertexBuilder::bindVertexAttributes(GrGLuint programID) {
    const GrPrimitiveProcessor& primProc = fProgramBuilder->primitiveProcessor();

    int vaCount = primProc.numAttribs();
    for (int i = 0; i < vaCount; i++) {
        GL_CALL(BindAttribLocation(programID, i, primProc.getAttrib(i).fName));
    }
    return;
}

bool GrGLVertexBuilder::compileAndAttachShaders(GrGLuint programId,
        SkTDArray<GrGLuint>* shaderIds) const {
    GrGLGpu* gpu = fProgramBuilder->gpu();
    const GrGLContext& glCtx = gpu->glContext();
    const GrGLContextInfo& ctxInfo = gpu->ctxInfo();
    SkString vertShaderSrc(GrGetGLSLVersionDecl(ctxInfo));
    fProgramBuilder->appendUniformDecls(GrGLProgramBuilder::kVertex_Visibility, &vertShaderSrc);
    this->appendDecls(fInputs, &vertShaderSrc);
    this->appendDecls(fOutputs, &vertShaderSrc);
    vertShaderSrc.append("void main() {");
    vertShaderSrc.append(fCode);
    vertShaderSrc.append("}\n");
    GrGLuint vertShaderId = GrGLCompileAndAttachShader(glCtx, programId,
                                                       GR_GL_VERTEX_SHADER, vertShaderSrc,
                                                       gpu->gpuStats());
    if (!vertShaderId) {
        return false;
    }
    *shaderIds->append() = vertShaderId;
    return true;
}

bool GrGLVertexBuilder::addAttribute(const GrShaderVar& var) {
    SkASSERT(GrShaderVar::kAttribute_TypeModifier == var.getTypeModifier());
    for (int i = 0; i < fInputs.count(); ++i) {
        const GrGLShaderVar& attr = fInputs[i];
        // if attribute already added, don't add it again
        if (attr.getName().equals(var.getName())) {
            return false;
        }
    }
    fInputs.push_back(var);
    return true;
}
