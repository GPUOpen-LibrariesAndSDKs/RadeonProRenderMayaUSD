//
// Copyright 2023 Advanced Micro Devices, Inc
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef __RPRUSDOPENSTUDIOSTAGECMD__
#define __RPRUSDOPENSTUDIOSTAGECMD__

#include <pxr/pxr.h>

#include <maya/MArgDatabase.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

// Command arguments.
// required
#define kFilePathFlag     "-f"
#define kFilePathFlagLong "-filePath"

#define kGetRecentFilePathUsedFlag     "-gr"
#define kGetRecentFilePathUsedFlagLong "-getRecentPath"

PXR_NAMESPACE_OPEN_SCOPE

/** Perform a single frame */
class RprUsdOpenStudioStageCmd : public MPxCommand
{
public:
    RprUsdOpenStudioStageCmd();
    // MPxCommand Implementation
    // -----------------------------------------------------------------------------

    MStatus doIt(const MArgList& args) override;

    /** Used by Maya to create the command instance. */
    static void* creator();

    /** Return the command syntax object. */
    static MSyntax newSyntax();

    // Static Methods
    // -----------------------------------------------------------------------------

    /** Clean up before plug-in shutdown. */
    static void cleanUp();

    static void Initialize();

    static void Uninitialize();

public:
    static MString s_commandName;

private:
    static MString s_LastRencetUsedFilePath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //__RPRUSDOPENSTUDIOSTAGECMD__
