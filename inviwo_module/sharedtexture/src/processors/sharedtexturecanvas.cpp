/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2023 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <ulm_university/sharedtexture/processors/sharedtexturecanvas.h>

#include <inviwo/core/util/rendercontext.h>
#include <modules/opengl/image/imagegl.h>
#include <modules/opengl/image/layergl.h>
#include <modules/opengl/texture/texture2D.h>

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo SharedTextureCanvas::processorInfo_{
    "de.uni-ulm.SharedTextureCanvas",  // Class identifier
    "Shared Texture Canvas",        // Display name
    "Undefined",                   // Category
    CodeState::Experimental,       // Code state
    Tags::None,                    // Tags
    R"(Share image as a texture with another application.)"_unindentHelp};

const ProcessorInfo SharedTextureCanvas::getProcessorInfo() const { return processorInfo_; }

SharedTextureCanvas::SharedTextureCanvas(InviwoApplication* app)
    : CanvasProcessor(app)
{
}

void SharedTextureCanvas::process() {
    if (!inport_.getData()->hasRepresentation<ImageGL>()) {
        RenderContext::getPtr()->activateDefaultRenderContext();
        inport_.getData()->getRepresentation<ImageGL>();
    }
    CanvasProcessor::process();

    auto image = inport_.getData();
    const ImageGL *imageGL = image->getRepresentation<ImageGL>();
    const LayerGL *layerGL = imageGL->getColorLayerGL();
    std::shared_ptr<const Texture2D> tex = layerGL->getTexture();
    GLuint textureID = tex->getID();
    
}

}  // namespace inviwo
