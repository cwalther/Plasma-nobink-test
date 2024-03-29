/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/
#ifndef plSoundMsg_h
#define plSoundMsg_h

#include "plMessageWithCallbacks.h"
#include "hsTemplates.h"
#include "hsBitVector.h"

class plSoundMsg : public plMessageWithCallbacks
{
private:
    void IInit() { fLoop=false; fPlaying = false; fBegin=fEnd=fTime=fRepeats=0; fSpeed = 0.f; fVolume = 0.f; fIndex = -1; fNameStr = 0; fFadeType = kLinear; }
public:
    plSoundMsg()
        : plMessageWithCallbacks(nil, nil, nil) { IInit(); }
    plSoundMsg(const plKey &s, 
                const plKey &r, 
                const double* t)
        : plMessageWithCallbacks(s, r, t) { IInit(); }
    ~plSoundMsg();

    CLASSNAME_REGISTER( plSoundMsg );
    GETINTERFACE_ANY( plSoundMsg, plMessageWithCallbacks );

    enum ModCmds
    {
        kPlay=0,
        kStop,
        kSetLooping,
        kUnSetLooping,
        kSetBegin,
        kToggleState,
        kAddCallbacks,
        kRemoveCallbacks,
        kGetStatus,
        kGetNumSounds,
        kStatusReply,
        kGoToTime,
        kSetVolume,
        kSetTalkIcon,
        kClearTalkIcon,
        kSetFadeIn,
        kSetFadeOut,
        kIsLocalOnly,       // Not really a command, just a flag
        kSelectFromGroup,
        kNumCmds,
        kFastForwardPlay,
        kFastForwardToggle,
        kSynchedPlay,
    };

    hsBitVector     fCmd;

    hsBool Cmd(int n) const { return fCmd.IsBitSet(n); }
    void SetCmd(int n) { fCmd.SetBit(n); }
    void ClearCmd();

    double   fBegin;
    double   fEnd;
    hsBool   fLoop;
    float fSpeed;
    double   fTime;
    int      fIndex;
    int      fRepeats;
    hsBool   fPlaying;
    uint32_t   fNameStr;  
    float fVolume;   // Range: 0 - silence, 1.f - loudest

    enum FadeType
    {
        kLinear,
        kLogarithmic,
        kExponential
    } fFadeType;

    // IO
    void Read(hsStream* stream, hsResMgr* mgr);
    void Write(hsStream* stream, hsResMgr* mgr);
};


#endif //plWin32Sound_h
