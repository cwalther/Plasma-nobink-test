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
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  pfGUIMenuItem Header                                                    //
//                                                                          //
//  The type of button that knows how to party.                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef _pfGUIMenuItem_h
#define _pfGUIMenuItem_h

#include "pfGUIButtonMod.h"

class plMessage;

class pfGUISkin;
class pfGUIMenuItem : public pfGUIButtonMod
{
    public:
        enum HowToSkin
        {
            kTop,
            kMiddle,
            kBottom
        };

    protected:

        wchar_t         *fName;
        hsBool          fReportingHover;

        HowToSkin       fHowToSkin;
        hsBool          fSkinBuffersUpdated;

        virtual void    IGrowDTMDimsToDesiredSize( uint16_t &width, uint16_t &height );
        virtual void    IPostSetUpDynTextMap( void );
        virtual void    IUpdate( void );

        void            IUpdateSkinBuffers( void );
        void            IUpdateSingleSkinBuffer( uint16_t y, hsBool sel );

    public:

        pfGUIMenuItem();
        virtual ~pfGUIMenuItem();

        CLASSNAME_REGISTER( pfGUIMenuItem );
        GETINTERFACE_ANY( pfGUIMenuItem, pfGUIButtonMod );

        enum ItemFlags
        {
            kDrawSubMenuArrow = kDerivedFlagsStart,
            kReportHovers
        };

        // Extended event types
        enum ExtendedEvents
        {
            kMouseHover,
            kMouseExit
        };

        virtual hsBool  MsgReceive( plMessage* pMsg );
        
        virtual void Read( hsStream* s, hsResMgr* mgr );
        virtual void Write( hsStream* s, hsResMgr* mgr );

        virtual void    SetInteresting( hsBool i );

        virtual void    HandleMouseDown( hsPoint3 &mousePt, uint8_t modifiers );
        virtual void    HandleMouseUp( hsPoint3 &mousePt, uint8_t modifiers );
        virtual void    HandleMouseDrag( hsPoint3 &mousePt, uint8_t modifiers );
        virtual void    HandleMouseHover( hsPoint3 &mousePt, uint8_t modifiers );

        virtual void    PurgeDynaTextMapImage();


        void        SetName( const char *name );
        void        SetName( const wchar_t *name );
        const wchar_t   *GetName( void ) const { return fName; }
    
        void    GetTextExtents( uint16_t &width, uint16_t &height );

        void    SetSkin( pfGUISkin *skin, HowToSkin s );
};

#endif // _pfGUIMenuItem_h
