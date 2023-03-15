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
#include <ulm_university/sharedtexture/sharedtexturemodule.h>

namespace inviwo {

SharedTextureModule::SharedTextureModule(InviwoApplication* app) : InviwoModule(app, "SharedTexture") {
    // Add a directory to the search path of the Shadermanager
    // ShaderManager::getPtr()->addShaderSearchPath(getPath(ModulePath::GLSL));

    // Register objects that can be shared with the rest of inviwo here:

    // Processors
    registerProcessor<SharedTextureCanvas>();
    // registerProcessor<SharedTextureProcessor>();

    // Properties
    // registerProperty<SharedTextureProperty>();

    // Readers and writes
    // registerDataReader(std::make_unique<SharedTextureReader>());
    // registerDataWriter(std::make_unique<SharedTextureWriter>());

    // Data converters
    // registerRepresentationConverter(std::make_unique<SharedTextureDisk2RAMConverter>());

    // Ports
    // registerPort<SharedTextureOutport>();
    // registerPort<SharedTextureInport>();

    // PropertyWidgets
    // registerPropertyWidget<SharedTexturePropertyWidget, SharedTextureProperty>("Default");

    // Dialogs
    // registerDialog<SharedTextureDialog>(SharedTextureOutport);

    // Other things
    // registerCapabilities(std::make_unique<SharedTextureCapabilities>());
    // registerSettings(std::make_unique<SharedTextureSettings>());
    // registerMetaData(std::make_unique<SharedTextureMetaData>());
    // registerPortInspector("SharedTextureOutport", "path/workspace.inv");
    // registerProcessorWidget(std::string processorClassName, std::unique_ptr<ProcessorWidget> processorWidget); 
    // registerDrawer(util::make_unique_ptr<SharedTextureDrawer>());
}

}  // namespace inviwo
