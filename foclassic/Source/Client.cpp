#include "Core.h"

#include "FL/Fl.H"
#include "FL/x.H"

#include "Access.h"
#include "App.h"
#include "Client.h"
#include "CommandLine.h"
#include "ConfigFile.h"
#include "ConstantsManager.h"
#include "CraftManager.h"
#include "CritterType.h"
#include "Crypt.h"
#include "Defence.h"
#include "Exception.h"
#include "FileSystem.h"
#include "GameOptions.h"
#include "GraphicLoader.h" // CHECK_MULTIPLY_WINDOWS
#include "Ini.h"
#include "ItemManager.h"
#include "Keyboard.h"
#include "Log.h"
#include "MsgStr.h"
#include "NetProtocol.h"
#include "Network.h"
#include "ResourceManager.h"
#include "Script.h"
#include "ScriptBind.h"
#include "ScriptUtils.h"
#include "SinglePlayer.h"
#include "SoundManager.h"
#include "Thread.h"
#include "Timer.h"
#include "Utils.h"
#include "Version.h"
#include "Window.h"

using namespace std;

// Check buffer for error
#define CHECK_IN_BUFF_ERROR                          \
    if( Bin.IsError() )                              \
    {                                                \
        WriteLogF( _FUNC_, " - Wrong MSG data.\n" ); \
        return;                                      \
    }

void* zlib_alloc_( void* opaque, unsigned int items, unsigned int size ) { return calloc( items, size ); }
void  zlib_free_( void* opaque, void* address )                          { free( address ); }

FOClient::IfaceAnim::IfaceAnim( AnyFrames* frm, int res_type ) : Frames( frm ), Flags( 0 ), CurSpr( 0 ), LastTick( Timer::GameTick() ), ResType( res_type )
{}

FOClient*    FOClient::Self = NULL;
bool         FOClient::SpritesCanDraw = false;
static uint* UID4 = NULL;
FOClient::FOClient() : Active( false )
{
    Self = this;

    ComLen = 4096;
    ComBuf = new char[ComLen];
    ZStreamOk = false;
    Sock = INVALID_SOCKET;
    BytesReceive = 0;
    BytesRealReceive = 0;
    BytesSend = 0;
    IsConnected = false;
    InitNetReason = INIT_NET_REASON_NONE;

    Chosen = NULL;
    PingTick = 0;
    PingCallTick = 0;
    IsTurnBased = false;
    TurnBasedTime = 0;
    TurnBasedCurCritterId = 0;
    DaySumRGB = 0;
    CurMode = 0;
    CurModeLast = 0;

    GmapCar.Car = NULL;
    Animations.resize( 10000 );

    #ifndef FO_D3D
    CurVideo = NULL;
    MusicVolumeRestore = -1;
    #endif

    UIDFail = false;
    MoveLastHx = -1;
    MoveLastHy = -1;

    #ifdef FO_D3D
    SaveLoadDraft = NULL;
    #endif
}

void _PreRestore()
{
    FOClient::Self->HexMngr.PreRestore();

    // Save/load surface
    if( Singleplayer )
    {
        #ifdef FO_D3D
        SAFEREL( FOClient::Self->SaveLoadDraft );
        #endif
        FOClient::Self->SaveLoadDraftValid = false;
    }
}

void _PostRestore()
{
    FOClient::Self->HexMngr.PostRestore();
    FOClient::Self->SetDayTime( true );

    // Save/load surface
    if( Singleplayer )
    {
        #ifdef FO_D3D
        SAFEREL( FOClient::Self->SaveLoadDraft );
        if( FAILED( SprMngr.GetDevice()->CreateRenderTarget( SAVE_LOAD_IMAGE_WIDTH, SAVE_LOAD_IMAGE_HEIGHT,
                                                             D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &FOClient::Self->SaveLoadDraft, NULL ) ) )
            WriteLog( "Create save/load draft surface fail.\n" );
        #endif
        FOClient::Self->SaveLoadDraftValid = false;
    }
}

uint* UID1;
bool FOClient::Init()
{
    WriteLog( "Engine initialization...\n" );

	pipboyFont = FONT_TYPE_DEFAULT;
	dialogFont = FONT_TYPE_DEFAULT;
	fixboyFont = FONT_TYPE_DEFAULT;
	messboxFont = FONT_TYPE_DEFAULT;

	fontManager = FontManager();
	fontManager.add(FONT_TYPE_DEFAULT);
	fontManager.add(FONT_TYPE_DEFAULT_125_THIN);
	fontManager.add(FONT_TYPE_DEFAULT_125);

	showTimestamps = false;

	InitMessTabs();

	activeQuestTab = FOClient::ActiveQuestTab::ALL;

    // Check the sizes of base types
    #include "StaticAssert.h"

    GET_UID0( UID0 );
    UID_PREPARE_UID4_0;

    // Another check for already runned window
    #ifndef DEV_VERSION
    if( !Singleplayer )
    {
        # ifdef FO_WINDOWS
        HANDLE h = CreateEvent( NULL, FALSE, FALSE, "_fcsync_" ); // last change 20.09.2018
        if( !h || h == INVALID_HANDLE_VALUE || GetLastError() == ERROR_ALREADY_EXISTS )
            memset( MulWndArray, 1, sizeof(MulWndArray) );
        # else
        // Todo: Linux
        # endif
    }
    #endif

    // Register dll script data
    struct CritterChangeParameter_
    {
        static void CritterChangeParameter( void*, uint ) {}
    };                                                                                    // Dummy
    GameOpt.CritterChangeParameter = &CritterChangeParameter_::CritterChangeParameter;

    GameOpt.CritterTypes = &CritType::GetRealCritType( 0 );

    struct GetDrawingSprites_
    {
        static void* GetDrawingSprites( uint& count )
        {
            Sprites& tree = Self->HexMngr.GetDrawTree();
            count = tree.Size();
            if( !count ) return NULL;
            return &(*tree.Begin() );
        }
    };
    GameOpt.GetDrawingSprites = &GetDrawingSprites_::GetDrawingSprites;

    struct GetSpriteInfo_
    {
        static void* GetSpriteInfo( uint spr_id )
        {
            return SprMngr.GetSpriteInfo( spr_id );
        }
    };
    GameOpt.GetSpriteInfo = &GetSpriteInfo_::GetSpriteInfo;

    struct GetSpriteColor_
    {
        static uint GetSpriteColor( uint spr_id, int x, int y, bool with_zoom )
        {
            return SprMngr.GetPixColor( spr_id, x, y, with_zoom );
        }
    };
    GameOpt.GetSpriteColor = &GetSpriteColor_::GetSpriteColor;

    struct IsSpriteHit_
    {
        static bool IsSpriteHit( void* sprite, int x, int y, bool check_egg )
        {
            Sprite*     sprite_ = (Sprite*)sprite;
            if( !sprite_ || !sprite_->Valid ) return false;
            SpriteInfo* si = SprMngr.GetSpriteInfo( sprite_->PSprId ? *sprite_->PSprId : sprite_->SprId );
            if( !si ) return false;
            if( si->Anim3d ) return si->Anim3d->IsIntersect( x, y );
            int sx = sprite_->ScrX - si->Width / 2 + si->OffsX + GameOpt.ScrOx + (sprite_->OffsX ? *sprite_->OffsX : 0);
            int sy = sprite_->ScrY - si->Height + si->OffsY + GameOpt.ScrOy + (sprite_->OffsY ? *sprite_->OffsY : 0);
            if( !(sprite_ = sprite_->GetIntersected( x - sx, y - sy ) ) ) return false;
            if( check_egg && SprMngr.CompareHexEgg( sprite_->HexX, sprite_->HexY, sprite_->EggType ) && SprMngr.IsEggTransp( x, y ) ) return false;
            return true;
        }
    };
    GameOpt.IsSpriteHit = &IsSpriteHit_::IsSpriteHit;

    struct GetNameByHash_
    {
        static const char* GetNameByHash( uint hash ) { return Str::GetName( hash ); }
    };
    GameOpt.GetNameByHash = &GetNameByHash_::GetNameByHash;
    struct GetHashByName_
    {
        static uint GetHashByName( const char* name ) { return Str::GetHash( name ); }
    };
    GameOpt.GetHashByName = &GetHashByName_::GetHashByName;

    // Input
    Keyb::InitKeyb();

    // Paths
    FileManager::SetDataPath( GameOpt.DataPath.c_str() );
    if( Singleplayer )
        FileManager::CreateDirectoryTree( FileManager::GetFullPath( "", PATH_SAVE ) );

    // Data files
    FileManager::InitDataFiles( ".\\" );

    // Cache
    FileManager::CreateDirectoryTree( FileManager::GetFullPath( "", PATH_CACHE ) );
    if( !Crypt.SetCacheTable( FileManager::GetFullPath( "default.cache", PATH_CACHE ) ) )
    {
        WriteLogF( _FUNC_, " - Can't set default cache.\n" );
        return false;
    }

    char cache_name[MAX_FOPATH] = { "singleplayer" };
    if( !Singleplayer )
        Str::Format( cache_name, "%s.%u", GameOpt.Host.c_str(), GameOpt.Port );

    char cache_fname[MAX_FOPATH];
    FileManager::GetFullPath( cache_name, PATH_CACHE, cache_fname );
    Str::Append( cache_fname, ".cache" );

    bool refresh_cache = (!Singleplayer && !CommandLine->IsOption( "DefCache" ) && !Crypt.IsCacheTable( cache_fname ) );

    if( !Singleplayer && !CommandLine->IsOption( "DefCache" ) && !Crypt.SetCacheTable( cache_fname ) )
    {
        WriteLogF( _FUNC_, " - Can't set cache<%s>.\n", cache_fname );
        return false;
    }

    FileManager::SetCacheName( cache_name );

    UID_PREPARE_UID4_1;

    // Check password in command line
    Password = CommandLine->GetStr( "UserPass" );

    // User and password
    if( !GameOpt.Name.length() && Password.empty() && !Singleplayer )
    {
        bool  fail = false;

        uint  len;
        char* str = (char*)Crypt.GetCache( "__name", len );
        if( str && len <= UTF8_BUF_SIZE( MAX_NAME ) )
            GameOpt.Name = str;
        else
            fail = true;
        delete[] str;

        str = (char*)Crypt.GetCache( "__pass", len );
        if( str && len <= UTF8_BUF_SIZE( MAX_NAME ) )
            Password = str;
        else
            fail = true;
        delete[] str;

        if( fail )
        {
            GameOpt.Name = "login";
            Password = "password";
            Crypt.SetCache( "__name", (uint8*)GameOpt.Name.c_str(), (uint)GameOpt.Name.length() + 1 );
            Crypt.SetCache( "__pass", (uint8*)Password.c_str(), (uint)Password.length() + 1 );
            refresh_cache = true;
        }
    }

    UID_PREPARE_UID4_2;

    // Sprite manager
    SpriteMngrParams params;
    params.PreRestoreFunc = &_PreRestore;
    params.PostRestoreFunc = &_PostRestore;
    if( !SprMngr.Init( params ) )
        return false;
    GET_UID1( UID1 );

    // Fonts
    if( !SprMngr.LoadFontFO( FONT_TYPE_FO, "OldDefault" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_NUM, "Numbers" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_BIG_NUM, "BigNumbers" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_SAND_NUM, "SandNumbers" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_SPECIAL, "Special" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_DEFAULT, "Default" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_THIN, "Thin" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_FAT, "Fat" ) )
        return false;
    if( !SprMngr.LoadFontFO( FONT_TYPE_BIG, "Big" ) )
        return false;
    SprMngr.SetDefaultFont( FONT_TYPE_DEFAULT, COLOR_TEXT );

    // Additional fonts
    if( !SprMngr.LoadFontFO( FONT_TYPE_DIALOG, "Dialog" ) )
    {
        if( !SprMngr.LoadFontFO( FONT_TYPE_DIALOG, "Default" ) )
            return false;
    }

	if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_125, "Default_125")) {
		if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_125, "Default"))
			return false;
	}

	if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_125_THIN, "Default_125_thin")) {
		if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_125_THIN, "Default"))
			return false;
	}

	if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_150, "Default_150")) {
		if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_150, "Default"))
			return false;
	}

	if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_200, "Default_200")) {
		if (!SprMngr.LoadFontFO(FONT_TYPE_DEFAULT_200, "Default"))
			return false;
	}

	// BMF to FOFNT convertation
    if( false )
        SprMngr.LoadFontBMF( 11111, "DefaultExt" );

    // Sound manager
    SndMngr.Init();
    SndMngr.SetSoundVolume( ConfigFile->GetInt( SECTION_CLIENT, "SoundVolume", 100 ) );
    SndMngr.SetMusicVolume( ConfigFile->GetInt( SECTION_CLIENT, "MusicVolume", 100 ) );

    UID_PREPARE_UID4_3;

    // Language Packs
    string lang_name = ConfigFile->GetStr( SECTION_CLIENT, "Language", DEFAULT_LANGUAGE );
    transform( lang_name.begin(), lang_name.end(), lang_name.begin(), ::tolower );
    if( lang_name.length() != 4 )
        lang_name = DEFAULT_LANGUAGE;

    CurLang.Init( lang_name.c_str(), PATH_TEXTS );

    MsgText = &CurLang.Msg[TEXTMSG_TEXT];
    MsgDlg = &CurLang.Msg[TEXTMSG_DLG];
    MsgItem = &CurLang.Msg[TEXTMSG_ITEM];
    MsgGame = &CurLang.Msg[TEXTMSG_GAME];
    MsgGM = &CurLang.Msg[TEXTMSG_GM];
    MsgCombat = &CurLang.Msg[TEXTMSG_COMBAT];
    MsgQuest = &CurLang.Msg[TEXTMSG_QUEST];
    MsgHolo = &CurLang.Msg[TEXTMSG_HOLO];
    MsgCraft = &CurLang.Msg[TEXTMSG_CRAFT];
    MsgInternal = &CurLang.Msg[TEXTMSG_INTERNAL];
    MsgUserHolo = new FOMsg;
    MsgUserHolo->LoadMsgFile( USER_HOLO_TEXTMSG_FILE, PATH_TEXTS );

    // CritterCl types
    CritType::InitFromMsg( MsgInternal );
    GET_UID2( UID2 );

    // Resource manager
    ResMngr.Refresh();

    UID_PREPARE_UID4_4;

    // Wait screen
    ScreenModeMain = CLIENT_MAIN_SCREEN_WAIT;
    CurMode = CURSOR_WAIT;
    WaitPic = ResMngr.GetRandomSplash();
    if( SprMngr.BeginScene( COLOR_XRGB( 0, 0, 0 ) ) )
    {
        WaitDraw();
        SprMngr.EndScene();
    }

    // Constants
    ConstantsManager::Initialize( PATH_DATA );

    // Base ini options
    if( !AppendIfaceIni( NULL ) )
        return false;

    // Scripts
    if( !ReloadScripts( true ) )
        refresh_cache = true;

    // Load interface
    int res = InitIface();
    if( res != 0 )
    {
        WriteLog( "Init interface fail, error<%d>.\n", res );
        return false;
    }

    // Quest Manager
    QuestMngr.Init( MsgQuest );

    // Item manager
    if( !ItemMngr.Init() )
        return false;

    // Item prototypes
    ItemMngr.ClearProtos();
    uint   protos_len;
    uint8* protos = Crypt.GetCache( "item_protos", protos_len );
    if( protos )
    {
        uint8* protos_uc = Crypt.Uncompress( protos, protos_len, 15 );
        delete[] protos;

        uint   protos_count_len;
        uint8* protos_count = Crypt.GetCache( "item_protos_count", protos_count_len );
        uint   count = 0;
        if( protos_count )
        {
            count = *(uint*)protos_count;
            delete[] protos_count;
            if( count != protos_len / sizeof(ProtoItem) )
                count = 0;
        }

        if( protos_uc )
        {
            if( count )
            {
                ProtoItemVec proto_items;
                proto_items.resize( count );
                memcpy( (void*)&proto_items[0], protos_uc, protos_len );
                ItemMngr.ParseProtos( proto_items );
            }

            delete[] protos_uc;
        }
    }

    // MrFixit
    MrFixit.LoadCrafts( *MsgCraft );
    MrFixit.GenerateNames( *MsgGame, *MsgItem );  // After Item manager init

    // Hex manager
    if( !HexMngr.Init() )
        return false;
    GET_UID3( UID3 );

    // Other
    SetGameColor( COLOR_IFACE );
    #ifdef FO_D3D
    if( GameOpt.FullScreen )
        SetCurPos( MODE_WIDTH / 2, MODE_HEIGHT / 2 );
    #endif
    ScreenOffsX = 0;
    ScreenOffsY = 0;
    ScreenOffsXf = 0.0f;
    ScreenOffsYf = 0.0f;
    ScreenOffsStep = 0.0f;
    ScreenOffsNextTick = 0;
    ScreenMirrorTexture = NULL;
    ScreenMirrorEndTick = 0;
    ScreenMirrorStart = false;
    RebuildLookBorders = false;
    DrawLookBorders = false;
    DrawShootBorders = false;

    UID_PREPARE_UID4_5;

    LookBorders.clear();
    ShootBorders.clear();

    UID_PREPARE_UID4_6;

    WriteLog( "Engine initialization complete.\n" );
    Active = true;
    GET_UID4( UID4 );

    // Start game
    // Check cache
    if( refresh_cache )
    {
        InitNetReason = INIT_NET_REASON_CACHE;
    }
    // Begin game
    else if( !Singleplayer && CommandLine->IsOption( "Start" ) )
    {
        LogTryConnect();
    }
    #ifndef FO_D3D
    // Intro
    else if( !CommandLine->IsOption( "SkipIntro" ) )
    {
        if( MsgGame->Count( STR_MUSIC_MAIN_THEME ) )
            MusicAfterVideo = MsgGame->GetStr( STR_MUSIC_MAIN_THEME );
        for( uint i = STR_VIDEO_INTRO_BEGIN; i < STR_VIDEO_INTRO_END; i++ )
            if( MsgGame->Count( i ) )
                AddVideo( MsgGame->GetStr( i, 0 ), true, false );

        if( !IsVideoPlayed() )
        {
            ScreenFadeOut();
            if( MusicAfterVideo != "" )
            {
                SndMngr.PlayMusic( MusicAfterVideo.c_str() );
                MusicAfterVideo = "";
            }
        }
    }
    #else
    ScreenFadeOut();
    if( MsgGame->Count( STR_MUSIC_MAIN_THEME ) )
        SndMngr.PlayMusic( MsgGame->GetStr( STR_MUSIC_MAIN_THEME ) );
    #endif

    // Disable dumps if multiple window detected
    if( MulWndArray[11] )
        CatchExceptions( NULL );

    return true;
}

void FOClient::Finish()
{
    WriteLog( "Engine finish...\n" );

    #ifdef FO_D3D
    SAFEREL( SaveLoadDraft );
    #endif

    Keyb::Finish();
    NetDisconnect();
    ResMngr.Finish();
    HexMngr.Finish();
    SprMngr.Finish();
    SndMngr.Finish();
    QuestMngr.Finish();
    MrFixit.Finish();
    Script::Finish();

    SAFEDELA( ComBuf );

    for( auto it = IntellectWords.begin(), end = IntellectWords.end(); it != end; ++it )
    {
        delete[] (*it).first;
        delete[] (*it).second;
    }
    IntellectWords.clear();
    for( auto it = IntellectSymbols.begin(), end = IntellectSymbols.end(); it != end; ++it )
    {
        delete[] (*it).first;
        delete[] (*it).second;
    }
    IntellectSymbols.clear();
    FileManager::EndOfWork();

    Active = false;
    WriteLog( "Engine finish complete.\n" );
}

void FOClient::ClearCritters()
{
    HexMngr.ClearCritters();
//	CritsNames.clear();
    Chosen = NULL;
    ChosenAction.clear();
    InvContInit.clear();
    BarterCont1oInit = BarterCont2Init = BarterCont2oInit = PupCont2Init = InvContInit;
    PupCont2.clear();
    InvCont = BarterCont1 = PupCont1 = UseCont = BarterCont1o = BarterCont2 = BarterCont2o = PupCont2;
}

void FOClient::AddCritter( CritterCl* cr )
{
    uint       fading = 0;
    CritterCl* cr_ = GetCritter( cr->GetId() );
    if( cr_ )
        fading = cr_->FadingTick;
    EraseCritter( cr->GetId() );
    if( HexMngr.IsMapLoaded() )
    {
        Field& f = HexMngr.GetField( cr->GetHexX(), cr->GetHexY() );
        if( f.Crit && f.Crit->IsFinishing() )
            EraseCritter( f.Crit->GetId() );
    }
    if( cr->IsChosen() )
        Chosen = cr;
    HexMngr.AddCrit( cr );
    cr->FadingTick = Timer::GameTick() + FADING_PERIOD - (fading > Timer::GameTick() ? fading - Timer::GameTick() : 0);
}

void FOClient::EraseCritter( uint remid )
{
    if( Chosen && Chosen->GetId() == remid )
        Chosen = NULL;
    HexMngr.EraseCrit( remid );
}

bool FOClient::IsTurnBasedMyTurn()
{
    return IsTurnBased && Timer::GameTick() < TurnBasedTime && Chosen && Chosen->GetId() == TurnBasedCurCritterId && Chosen->GetAllAp() > 0;
}

void FOClient::LookBordersPrepare()
{
    if( !DrawLookBorders && !DrawShootBorders )
        return;

    LookBorders.clear();
    ShootBorders.clear();
    if( HexMngr.IsMapLoaded() && Chosen )
    {
        uint   dist = Chosen->GetLook();
        uint16 base_hx = Chosen->GetHexX();
        uint16 base_hy = Chosen->GetHexY();
        int    hx = base_hx;
        int    hy = base_hy;
        int    chosen_dir = Chosen->GetDir();
        uint   dist_shoot = Chosen->GetAttackDist();
        uint16 maxhx = HexMngr.GetMaxHexX();
        uint16 maxhy = HexMngr.GetMaxHexY();
        bool   seek_start = true;
        for( int i = 0; i < (GameOpt.MapHexagonal ? 6 : 4); i++ )
        {
            int dir = (GameOpt.MapHexagonal ? (i + 2) % 6 : ( (i + 1) * 2 ) % 8);

            for( uint j = 0, jj = (GameOpt.MapHexagonal ? dist : dist * 2); j < jj; j++ )
            {
                if( seek_start )
                {
                    // Move to start position
                    for( uint l = 0; l < dist; l++ )
                        MoveHexByDirUnsafe( hx, hy, GameOpt.MapHexagonal ? 0 : 7 );
                    seek_start = false;
                    j = -1;
                }
                else
                {
                    // Move to next hex
                    MoveHexByDirUnsafe( hx, hy, dir );
                }

                uint16 hx_ = CLAMP( hx, 0, maxhx - 1 );
                uint16 hy_ = CLAMP( hy, 0, maxhy - 1 );
                if( FLAG( GameOpt.LookChecks, LOOK_CHECK_DIR ) )
                {
                    int dir_ = GetFarDir( base_hx, base_hy, hx_, hy_ );
                    int ii = (chosen_dir > dir_ ? chosen_dir - dir_ : dir_ - chosen_dir);
                    if( ii > DIRS_COUNT / 2 )
                        ii = DIRS_COUNT - ii;
                    uint       dist_ = dist - dist * GameOpt.LookDir[ii] / 100;
                    UInt16Pair block;
                    HexMngr.TraceBullet( base_hx, base_hy, hx_, hy_, dist_, 0.0f, NULL, false, NULL, 0, NULL, &block, NULL, false );
                    hx_ = block.first;
                    hy_ = block.second;
                }

                if( FLAG( GameOpt.LookChecks, LOOK_CHECK_TRACE ) )
                {
                    UInt16Pair block;
                    HexMngr.TraceBullet( base_hx, base_hy, hx_, hy_, 0, 0.0f, NULL, false, NULL, 0, NULL, &block, NULL, true );
                    hx_ = block.first;
                    hy_ = block.second;
                }

                uint16     hx__ = hx_;
                uint16     hy__ = hy_;
                uint       dist_look = DistGame( base_hx, base_hy, hx_, hy_ );
                UInt16Pair block;
                HexMngr.TraceBullet( base_hx, base_hy, hx_, hy_, min( dist_look, dist_shoot ), 0.0f, NULL, false, NULL, 0, NULL, &block, NULL, true );
                hx__ = block.first;
                hy__ = block.second;

                int x, y, x_, y_;
                HexMngr.GetHexCurrentPosition( hx_, hy_, x, y );
                HexMngr.GetHexCurrentPosition( hx__, hy__, x_, y_ );
                LookBorders.push_back( PrepPoint( x + HEX_OX, y + HEX_OY, COLOR_ARGB( 80, 0, 255, 0 ), (short*)&GameOpt.ScrOx, (short*)&GameOpt.ScrOy ) );
                ShootBorders.push_back( PrepPoint( x_ + HEX_OX, y_ + HEX_OY, COLOR_ARGB( 80, 255, 0, 0 ), (short*)&GameOpt.ScrOx, (short*)&GameOpt.ScrOy ) );
            }
        }

        if( LookBorders.size() < 2 )
            LookBorders.clear();
        else
            LookBorders.push_back( *LookBorders.begin() );
        if( ShootBorders.size() < 2 )
            ShootBorders.clear();
        else
            ShootBorders.push_back( *ShootBorders.begin() );
    }
}

void FOClient::LookBordersDraw()
{
    if( RebuildLookBorders )
    {
        LookBordersPrepare();
        RebuildLookBorders = false;
    }
    if( DrawLookBorders )
        SprMngr.DrawPoints( LookBorders, DRAW_PRIMITIVE_LINESTRIP, &GameOpt.SpritesZoom );
    if( DrawShootBorders )
        SprMngr.DrawPoints( ShootBorders, DRAW_PRIMITIVE_LINESTRIP, &GameOpt.SpritesZoom );
}

int FOClient::MainLoop()
{
    // Fixed FPS
    double start_loop = Timer::AccurateTick();

    // FPS counter
    static uint last_call = Timer::FastTick();
    static uint call_counter = 0;
    if( (Timer::FastTick() - last_call) >= 1000 )
    {
        GameOpt.FPS = call_counter;
        call_counter = 0;
        last_call = Timer::FastTick();
    }
    else
    {
        call_counter++;
    }

    // Singleplayer data synchronization
    if( Singleplayer )
    {
        #ifdef FO_WINDOWS
        bool pause = SingleplayerData.Pause;

        if( !pause )
        {
            int main_screen = GetMainScreen();
            if( (main_screen != CLIENT_MAIN_SCREEN_GAME && main_screen != CLIENT_MAIN_SCREEN_WORLDMAP && main_screen != CLIENT_MAIN_SCREEN_WAIT) ||
                IsScreenPresent( CLIENT_SCREEN_MENU ) )
                pause = true;
        }

        SingleplayerData.Lock();         // Read data
        if( pause != SingleplayerData.Pause )
        {
            SingleplayerData.Pause = pause;
            Timer::SetGamePause( pause );
        }
        SingleplayerData.Unlock();         // Write data
        #endif
    }

    //CHECK_MULTIPLY_WINDOWS0;

    // Network
    // Init Net
    if( InitNetReason != INIT_NET_REASON_NONE )
    {
        int reason = InitNetReason;
        InitNetReason = INIT_NET_REASON_NONE;

        // Wait screen
        ShowMainScreen( CLIENT_MAIN_SCREEN_WAIT );

        // Connect to server
        if( !InitNet() )
        {
            ShowMainScreen( CLIENT_MAIN_SCREEN_LOGIN );
            AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_CONN_FAIL ) );
            return 1;
        }

        // After connect things
        if( reason == INIT_NET_REASON_CACHE )
            Net_SendLogIn( NULL, NULL );
        else if( reason == INIT_NET_REASON_LOGIN )
            Net_SendLogIn( GameOpt.Name.c_str(), Password.c_str() );
        else if( reason == INIT_NET_REASON_REG )
            Net_SendCreatePlayer( RegNewCr );
        else if( reason == INIT_NET_REASON_LOAD )
            Net_SendSaveLoad( false, SaveLoadFileName.c_str(), NULL );
		else {
			WriteLog("DC: After connect things");
			NetDisconnect();
		}
    }

    // Parse Net
    if( IsConnected )
        ParseSocket();

    // Exit in Login screen if net disconnect
    if( !IsConnected && !IsMainScreen( CLIENT_MAIN_SCREEN_LOGIN ) && !IsMainScreen( CLIENT_MAIN_SCREEN_REGISTRATION ) && !IsMainScreen( CLIENT_MAIN_SCREEN_CREDITS ) )
        ShowMainScreen( CLIENT_MAIN_SCREEN_LOGIN );

    // Input
    ConsoleProcess();
    IboxProcess();
    ParseKeyboard();
    ParseMouse();

    //CHECK_MULTIPLY_WINDOWS1;

    // Process
    SoundProcess();
    AnimProcess();

    // Game time
    uint16 full_second = GameOpt.FullSecond;
    Timer::ProcessGameTime();
    if( full_second != GameOpt.FullSecond )
        SetDayTime( false );

    if( IsMainScreen( CLIENT_MAIN_SCREEN_WORLDMAP ) )
    {
        CrittersProcess();
        WorldmapProcess();
        LMenuTryCreate();
    }
    else if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && HexMngr.IsMapLoaded() )
    {
        int screen = GetActiveScreen();
        if( screen == CLIENT_SCREEN_NONE || screen == CLIENT_SCREEN_WM_TOWNVIEW || HexMngr.AutoScroll.Active )
        {
            if( HexMngr.Scroll() )
            {
                LMenuSet( LMENU_OFF );
                LookBordersPrepare();
            }
        }
        CrittersProcess();
        HexMngr.ProcessItems();
        HexMngr.ProcessRain();
        LMenuTryCreate();
    }
    else if( IsMainScreen( CLIENT_MAIN_SCREEN_CREDITS ) )
    {
        // CreditsDraw();
    }

    if( IsScreenPresent( CLIENT_SCREEN_ELEVATOR ) )
        ElevatorProcess();

    //CHECK_MULTIPLY_WINDOWS2;

    // Script loop
    static uint next_call = 0;
    if( Timer::FastTick() >= next_call )
    {
        uint wait_tick = 1000;
        if( Script::PrepareContext( ClientFunctions.Loop, _FUNC_, "Game" ) && Script::RunPrepared() )
            wait_tick = Script::GetReturnedUInt();
        next_call = Timer::FastTick() + wait_tick;
    }
    Script::ScriptGarbager();

    #ifndef FO_D3D
    // Video
    if( IsVideoPlayed() )
    {
        # ifdef FO_D3D
        LONGLONG cur, stop;
        if( !MediaSeeking || FAILED( MediaSeeking->GetPositions( &cur, &stop ) ) || cur >= stop )
            NextVideo();
        # else
        RenderVideo();
        # endif
        return 1;
    }
    #endif

    //CHECK_MULTIPLY_WINDOWS3;

    // Render
    if( !SprMngr.BeginScene( COLOR_XRGB( 0, 0, 0 ) ) )
        return 0;

    ProcessScreenEffectQuake();

    DrawIfaceLayer( 1 );

    if( GetMainScreen() == CLIENT_MAIN_SCREEN_GAME && HexMngr.IsMapLoaded() )
    {
        GameDraw();
        if( SaveLoadProcessDraft )
            SaveLoadFillDraft();
        ProcessScreenEffectMirror();
    }

    DrawIfaceLayer( 2 );

    if( SaveLoadProcessDraft && GetMainScreen() == CLIENT_MAIN_SCREEN_WORLDMAP )
        SaveLoadFillDraft();
    ConsoleDraw();
    MessBoxDraw();

    //CHECK_MULTIPLY_WINDOWS4;

    DrawIfaceLayer( 3 );

    LMenuDraw();
    CurDraw();

    DrawIfaceLayer( 4 );

    SprMngr.Flush();
    ProcessScreenEffectFading();

    //CHECK_MULTIPLY_WINDOWS5;

    SprMngr.EndScene();

    // Fixed FPS
    if( !GameOpt.VSync && GameOpt.FixedFPS )
    {
        if( GameOpt.FixedFPS > 0 )
        {
            static double balance = 0.0;
            double        elapsed = Timer::AccurateTick() - start_loop;
            double        need_elapsed = 1000.0 / (double)GameOpt.FixedFPS;
            if( need_elapsed > elapsed )
            {
                double sleep = need_elapsed - elapsed + balance;
                balance = fmod( sleep, 1.0 );
                Thread::Sleep( (uint)floor( sleep ) );
            }
        }
        else
        {
            Thread::Sleep( -GameOpt.FixedFPS );
        }
    }

    return 1;
}

void FOClient::ScreenFade( uint time, uint from_color, uint to_color, bool push_back )
{
    if( !push_back || ScreenEffects.empty() )
    {
        ScreenEffects.push_back( ScreenEffect( Timer::FastTick(), time, from_color, to_color ) );
    }
    else
    {
        uint last_tick = 0;
        for( auto it = ScreenEffects.begin(); it != ScreenEffects.end(); ++it )
        {
            ScreenEffect& e = (*it);
            if( e.BeginTick + e.Time > last_tick )
                last_tick = e.BeginTick + e.Time;
        }
        ScreenEffects.push_back( ScreenEffect( last_tick, time, from_color, to_color ) );
    }
}

void FOClient::ScreenQuake( int noise, uint time )
{
    GameOpt.ScrOx -= ScreenOffsX;
    GameOpt.ScrOy -= ScreenOffsY;
    ScreenOffsX = Random( 0, 1 ) ? noise : -noise;
    ScreenOffsY = Random( 0, 1 ) ? noise : -noise;
    ScreenOffsXf = (float)ScreenOffsX;
    ScreenOffsYf = (float)ScreenOffsY;
    ScreenOffsStep = fabs( ScreenOffsXf ) / (time / 30);
    GameOpt.ScrOx += ScreenOffsX;
    GameOpt.ScrOy += ScreenOffsY;
    ScreenOffsNextTick = Timer::GameTick() + 30;
}

void FOClient::ProcessScreenEffectFading()
{
    static PointVec six_points;
    if( six_points.empty() )
        SprMngr.PrepareSquare( six_points, Rect( 0, 0, MODE_WIDTH, MODE_HEIGHT ), 0 );

    for( auto it = ScreenEffects.begin(); it != ScreenEffects.end();)
    {
        ScreenEffect& e = (*it);
        if( Timer::FastTick() >= e.BeginTick + e.Time )
        {
            it = ScreenEffects.erase( it );
            continue;
        }
        else if( Timer::FastTick() >= e.BeginTick )
        {
            int proc = Procent( e.Time, Timer::FastTick() - e.BeginTick ) + 1;
            int res[4];

            for( int i = 0; i < 4; i++ )
            {
                int sc = ( (uint8*)&e.StartColor )[i];
                int ec = ( (uint8*)&e.EndColor )[i];
                int dc = ec - sc;
                res[i] = sc + dc * proc / 100;
            }

            uint color = COLOR_ARGB( res[3], res[2], res[1], res[0] );
            for( int i = 0; i < 6; i++ )
                six_points[i].PointColor = color;

            SprMngr.DrawPoints( six_points, DRAW_PRIMITIVE_TRIANGLELIST );
        }
        it++;
    }
}

void FOClient::ProcessScreenEffectQuake()
{
    if( (ScreenOffsX || ScreenOffsY) && Timer::GameTick() >= ScreenOffsNextTick )
    {
        GameOpt.ScrOx -= ScreenOffsX;
        GameOpt.ScrOy -= ScreenOffsY;
        if( ScreenOffsXf < 0.0f )
            ScreenOffsXf += ScreenOffsStep;
        else if( ScreenOffsXf > 0.0f )
            ScreenOffsXf -= ScreenOffsStep;
        if( ScreenOffsYf < 0.0f )
            ScreenOffsYf += ScreenOffsStep;
        else if( ScreenOffsYf > 0.0f )
            ScreenOffsYf -= ScreenOffsStep;
        ScreenOffsXf = -ScreenOffsXf;
        ScreenOffsYf = -ScreenOffsYf;
        ScreenOffsX = (int)ScreenOffsXf;
        ScreenOffsY = (int)ScreenOffsYf;
        GameOpt.ScrOx += ScreenOffsX;
        GameOpt.ScrOy += ScreenOffsY;
        ScreenOffsNextTick = Timer::GameTick() + 30;
    }
}

void FOClient::ProcessScreenEffectMirror()
{
/*
        if(ScreenMirrorStart)
        {
                ScreenQuake(10,1000);
                ScreenMirrorX=0;
                ScreenMirrorY=0;
                SAFEREL(ScreenMirrorTexture);
                ScreenMirrorEndTick=Timer::FastTick()+1000;
                ScreenMirrorStart=false;

                if(FAILED(SprMngr.GetDevice()->CreateTexture(MODE_WIDTH,MODE_HEIGHT,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&ScreenMirrorTexture))) return;
                LPDIRECT3DSURFACE8 mirror=NULL;
                LPDIRECT3DSURFACE8 back_buf=NULL;
                if(SUCCEEDED(ScreenMirrorTexture->GetSurfaceLevel(0,&mirror))
                && SUCCEEDED(SprMngr.GetDevice()->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&back_buf)))
                        D3DXLoadSurfaceFromSurface(mirror,NULL,NULL,back_buf,NULL,NULL,D3DX_DEFAULT,0);
                SAFEREL(back_buf);
                SAFEREL(mirror);
        }
        else if(ScreenMirrorEndTick)
        {
                if(Timer::FastTick()>=ScreenMirrorEndTick)
                {
                        SAFEREL(ScreenMirrorTexture);
                        ScreenMirrorEndTick=0;
                }
                else
                {
                        MYVERTEX vb_[6];
                        vb_[0].x=0+ScreenMirrorX-0.5f;
                        vb_[0].y=MODE_HEIGHT+ScreenMirrorY-0.5f;
                        vb_[0].tu=0.0f;
                        vb_[0].tv=1.0f;
                        vb_[0].Diffuse=0x7F7F7F7F;

                        vb_[1].x=0+ScreenMirrorX-0.5f;
                        vb_[1].y=0+ScreenMirrorY-0.5f;
                        vb_[1].tu=0.0f;
                        vb_[1].tv=0.0f;
                        vb_[1].Diffuse=0x7F7F7F7F;
                        vb_[3].x=0+ScreenMirrorX-0.5f;
                        vb_[3].y=0+ScreenMirrorY-0.5f;
                        vb_[3].tu=0.0f;
                        vb_[3].tv=0.0f;
                        vb_[3].Diffuse=0x7F7F7F7F;

                        vb_[2].x=MODE_WIDTH+ScreenMirrorX-0.5f;
                        vb_[2].y=MODE_HEIGHT+ScreenMirrorY-0.5f;
                        vb_[2].tu=1.0f;
                        vb_[2].tv=1.0f;
                        vb_[2].Diffuse=0x7F7F7F7F;
                        vb_[5].x=MODE_WIDTH+ScreenMirrorX-0.5f;
                        vb_[5].y=MODE_HEIGHT+ScreenMirrorY-0.5f;
                        vb_[5].tu=1.0f;
                        vb_[5].tv=1.0f;
                        vb_[5].Diffuse=0x7F7F7F7F;

                        vb_[4].x=MODE_WIDTH+ScreenMirrorX-0.5f;
                        vb_[4].y=0+ScreenMirrorY-0.5f;
                        vb_[4].tu=1.0f;
                        vb_[4].tv=0.0f;
                        vb_[4].Diffuse=0x7F7F7F7F;

                        SprMngr.GetDevice()->SetTexture(0,ScreenMirrorTexture);
                        LPDIRECT3DVERTEXBUFFER vb;
                        SprMngr.GetDevice()->CreateVertexBuffer(6*sizeof(MYVERTEX),D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,D3DFVF_MYVERTEX,D3DPOOL_DEFAULT,&vb);
                        void* vertices;
                        vb->Lock(0,6*sizeof(MYVERTEX),(uint8**)&vertices,D3DLOCK_DISCARD);
                        memcpy(vertices,vb_,6*sizeof(MYVERTEX));
                        vb->Unlock();
                        SprMngr.GetDevice()->SetStreamSource(0,vb,sizeof(MYVERTEX));
                        SprMngr.GetDevice()->SetVertexShader(D3DFVF_MYVERTEX);
                        SprMngr.GetDevice()->DrawPrimitive(DRAW_PRIMITIVE_TRIANGLELIST,0,2);
                        SAFEREL(vb);
                        SprMngr.GetDevice()->SetStreamSource(0,SprMngr.GetVB(),sizeof(MYVERTEX));
                        SprMngr.GetDevice()->SetVertexShader(D3DFVF_MYVERTEX);
                }
        }
 */
}

void FOClient::ParseKeyboard()
{
    // Stop processing if window not active
    if( !MainWindow->Focused )
    {
        MainWindow->KeyboardEvents.clear();
        MainWindow->KeyboardEventsText.clear();
        Keyb::Lost();
        Timer::StartAccelerator( ACCELERATE_NONE );
        if( Script::PrepareContext( ClientFunctions.InputLost, _FUNC_, "Game" ) )
            Script::RunPrepared();
        return;
    }

    // Accelerators
    if( !IsCurMode( CURSOR_WAIT ) )
    {
        if( Timer::ProcessAccelerator( ACCELERATE_PAGE_UP ) )
            ProcessMouseWheel( 1 );
        if( Timer::ProcessAccelerator( ACCELERATE_PAGE_DOWN ) )
            ProcessMouseWheel( -1 );
    }

    // Get buffered data
    if( MainWindow->KeyboardEvents.empty() )
        return;
    IntVec events = MainWindow->KeyboardEvents;
    StrVec events_text = MainWindow->KeyboardEventsText;
    MainWindow->KeyboardEvents.clear();
    MainWindow->KeyboardEventsText.clear();

    // Process events
    for( uint i = 0; i < events.size(); i += 2 )
    {
        // Event data
        int         event = events[i];
        int         event_key = events[i + 1];
        const char* event_text = events_text[i / 2].c_str();

        // Keys codes mapping
        uint8 dikdw = 0;
        uint8 dikup = 0;
        if( event == FL_KEYDOWN )
            dikdw = Keyb::MapKey( event_key );
        else if( event == FL_KEYUP )
            dikup = Keyb::MapKey( event_key );
        if( !dikdw  && !dikup )
            continue;

        // Avoid repeating
        if( dikdw && Keyb::KeyPressed[dikdw] )
            continue;
        if( dikup && !Keyb::KeyPressed[dikup] )
            continue;

        // Keyboard states, to know outside function
        Keyb::KeyPressed[dikup] = false;
        Keyb::KeyPressed[dikdw] = true;

        // Video
        #ifndef FO_D3D
        if( IsVideoPlayed() )
        {
            if( IsCanStopVideo() && (dikdw == DIK_ESCAPE || dikdw == DIK_SPACE || dikdw == DIK_RETURN || dikdw == DIK_NUMPADENTER) )
            {
                NextVideo();
                return;
            }
            continue;
        }
        #endif

        // Key script event
        bool script_result = false;
        if( dikdw && Script::PrepareContext( ClientFunctions.KeyDown, _FUNC_, "Game" ) )
        {
            ScriptString* event_text_script = new ScriptString( event_text );
            Script::SetArgUChar( dikdw );
            Script::SetArgObject( event_text_script );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
            event_text_script->Release();
        }

        if( dikup && Script::PrepareContext( ClientFunctions.KeyUp, _FUNC_, "Game" ) )
        {
            ScriptString* event_text_script = new ScriptString( event_text );
            Script::SetArgUChar( dikup );
            Script::SetArgObject( event_text_script );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
            event_text_script->Release();
        }

        // Disable keyboard events
        if( script_result || GameOpt.DisableKeyboardEvents )
        {
            if( dikdw == DIK_ESCAPE && Keyb::ShiftDwn )
                GameOpt.Quit = true;
            continue;
        }

        // Control keys
        if( dikdw == DIK_RCONTROL || dikdw == DIK_LCONTROL )
            Keyb::CtrlDwn = true;
        else if( dikdw == DIK_LMENU || dikdw == DIK_RMENU )
            Keyb::AltDwn = true;
        else if( dikdw == DIK_LSHIFT || dikdw == DIK_RSHIFT )
            Keyb::ShiftDwn = true;
        if( dikup == DIK_RCONTROL || dikup == DIK_LCONTROL )
            Keyb::CtrlDwn = false;
        else if( dikup == DIK_LMENU || dikup == DIK_RMENU )
            Keyb::AltDwn = false;
        else if( dikup == DIK_LSHIFT || dikup == DIK_RSHIFT )
            Keyb::ShiftDwn = false;

        // Hotkeys
        if( !Keyb::ShiftDwn && !Keyb::AltDwn && !Keyb::CtrlDwn )
        {
            // F Buttons
            switch( dikdw )
            {
                case DIK_F1:
                    GameOpt.HelpInfo = !GameOpt.HelpInfo;
                    break;
                case DIK_F2:
                    if( SaveLogFile() )
                        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_LOG_SAVED ) );
                    else
                        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_LOG_NOT_SAVED ) );
                    break;
                case DIK_F3:
                    if( SaveScreenshot() )
                        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_SCREENSHOT_SAVED ) );
                    else
                        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_SCREENSHOT_NOT_SAVED ) );
                    break;
                case DIK_F4:
                    IntVisible = !IntVisible;
                    MessBoxGenerate();
                    break;
                case DIK_F5:
                    IntAddMess = !IntAddMess;
                    MessBoxGenerate();
                    break;
                case DIK_F6:
                    GameOpt.ShowPlayerNames = !GameOpt.ShowPlayerNames;
                    break;

                case DIK_F7:
                    if( GameOpt.DebugInfo )
                        GameOpt.ShowNpcNames = !GameOpt.ShowNpcNames;
                    break;

                case DIK_F8:
                    GameOpt.MouseScroll = !GameOpt.MouseScroll;
                    GameOpt.ScrollMouseRight = false;
                    GameOpt.ScrollMouseLeft = false;
                    GameOpt.ScrollMouseDown = false;
                    GameOpt.ScrollMouseUp = false;
                    break;

                case DIK_F9:
                    if( GameOpt.DebugInfo )
                        HexMngr.SwitchShowTrack();
                    break;
                case DIK_F10:
                    if( GameOpt.DebugInfo )
                        HexMngr.SwitchShowHex();
                    break;

                // Fullscreen
                case DIK_F11:
                    if( !GameOpt.FullScreen )
                    {
                        MainWindow->size_range( MODE_WIDTH, MODE_HEIGHT );
                        MainWindow->fullscreen();
                        GameOpt.FullScreen = true;
                    }
                    else
                    {
                        MainWindow->fullscreen_off();
                        MainWindow->size_range( MODE_WIDTH, MODE_HEIGHT, MODE_WIDTH, MODE_HEIGHT );
                        GameOpt.FullScreen = false;
                    }
                    #ifndef FO_D3D
                    SprMngr.RefreshViewPort();
                    #endif
                    continue;
                // Minimize
                case DIK_F12:
                    MainWindow->iconize();
                    continue;

                // Exit buttons
                case DIK_TAB:
                    if( GetActiveScreen() == CLIENT_SCREEN_MINIMAP )
                    {
                        TryExit();
                        continue;
                    }
                    break;
                // Mouse wheel emulate
                case DIK_PRIOR:
                    ProcessMouseWheel( 1 );
                    Timer::StartAccelerator( ACCELERATE_PAGE_UP );
                    break;
                case DIK_NEXT:
                    ProcessMouseWheel( -1 );
                    Timer::StartAccelerator( ACCELERATE_PAGE_DOWN );
                    break;

                case DIK_ESCAPE:
					if (ConsoleActive) {
						ConsoleActive = false;
					}
					else {
						TryExit();
					}
                    continue;
                default:
                    break;
            }

            if( !ConsoleActive )
            {
                switch( dikdw )
                {
                    case DIK_C:
                        if( GetActiveScreen() == CLIENT_SCREEN_CHARACTER )
                        {
                            TryExit();
                            continue;
                        }
                        break;
                    case DIK_P:
                        if( GetActiveScreen() == CLIENT_SCREEN_PIPBOY )
                        {
                            if( PipMode == PIP__NONE )
                                PipMode = PIP__STATUS;
                            else
                                TryExit();
                            continue;
                        }
                    case DIK_U:
                        if( GetActiveScreen() == CLIENT_SCREEN_FIXBOY )
                        {
                            TryExit();
                            continue;
                        }
                        break;
                    case DIK_I:
                        if( GetActiveScreen() == CLIENT_SCREEN_INVENTORY )
                        {
                            TryExit();
                            continue;
                        }
                        break;
					case DIK_W:
						if (GetActiveScreen() == CLIENT_SCREEN_INVENTORY || GetActiveScreen() == CLIENT_SCREEN_BARTER
							|| GetActiveScreen() == CLIENT_SCREEN_PICKUP || GetActiveScreen() == CLIENT_SCREEN_USE) {
							HexMngr.ShowItemDurability = !HexMngr.ShowItemDurability;
						} else {
							HexMngr.ItemHighlightActive = !HexMngr.ItemHighlightActive;
							HexMngr.RefreshMap();
						}
						break;
                    case DIK_LBRACKET:
                        if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && GetActiveScreen() == CLIENT_SCREEN_NONE )
                        {
                            DrawLookBorders = !DrawLookBorders;
                            RebuildLookBorders = true;
                        }
                        break;
                    case DIK_RBRACKET:
                        if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && GetActiveScreen() == CLIENT_SCREEN_NONE )
                        {
                            DrawShootBorders = !DrawShootBorders;
                            RebuildLookBorders = true;
                        }
                        break;
                    case DIK_SPACE:
                        if( Singleplayer )
                            SingleplayerData.Pause = !SingleplayerData.Pause;
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            switch( dikdw )
            {
				case DIK_F1:
					//	We have a bug here, for some reason this gets double triggered without the Ctrl in effect,
					//	so it will also toggle normal F1 behaviour. TODO: ROTATORS PLS FIX
					if (Keyb::CtrlDwn)
						LegacyMouseCursor = !LegacyMouseCursor;
					break;
                case DIK_F6:
                    if( GameOpt.DebugInfo && Keyb::CtrlDwn )
                        GameOpt.ShowCritId = !GameOpt.ShowCritId;
                    break;
                case DIK_F7:
                    if( GameOpt.DebugInfo && Keyb::CtrlDwn )
                        GameOpt.ShowCritId = !GameOpt.ShowCritId;
                    break;
                case DIK_F11:
                    if( GameOpt.DebugInfo && Keyb::ShiftDwn )
                        SprMngr.SaveSufaces();
                    break;

                // Num Pad
                case DIK_EQUALS:
                case DIK_ADD:
                    if( ConsoleActive )
                        break;
                    if( Keyb::CtrlDwn )
                        SndMngr.SetSoundVolume( SndMngr.GetSoundVolume() + 2 );
                    else if( Keyb::ShiftDwn )
                        SndMngr.SetMusicVolume( SndMngr.GetMusicVolume() + 2 );
                    else if( Keyb::AltDwn && GameOpt.FixedFPS < 10000 )
                        GameOpt.FixedFPS++;
                    break;
                case DIK_MINUS:
                case DIK_SUBTRACT:
                    if( ConsoleActive )
                        break;
                    if( Keyb::CtrlDwn )
                        SndMngr.SetSoundVolume( SndMngr.GetSoundVolume() - 2 );
                    else if( Keyb::ShiftDwn )
                        SndMngr.SetMusicVolume( SndMngr.GetMusicVolume() - 2 );
                    else if( Keyb::AltDwn && GameOpt.FixedFPS > -10000 )
                        GameOpt.FixedFPS--;
                    break;
                // Escape
                case DIK_ESCAPE:
                    if( Keyb::ShiftDwn )
                        GameOpt.Quit = true;
                    break;
                default:
                    break;
            }
        }

        // Wait mode
        if( IsCurMode( CURSOR_WAIT ) )
        {
            ConsoleKeyDown( dikdw, event_text );
            continue;
        }

        // Cursor steps
        if( HexMngr.IsMapLoaded() && (dikdw == DIK_RCONTROL || dikdw == DIK_LCONTROL || dikup == DIK_RCONTROL || dikup == DIK_LCONTROL) )
			//    To check if smart cursor is activated, if so, may not redraw to keep move-hex outline on clicked position.
			if (!LegacyMouseCursor && !SmartMouseLastClick)
				HexMngr.SetCursorPos( GameOpt.MouseX, GameOpt.MouseY, Keyb::CtrlDwn, true );

        // Other by screens
        // Key down
        if( dikdw )
        {
            int active = GetActiveScreen();
            if( active != CLIENT_SCREEN_NONE )
            {
                switch( active )
                {
                    case CLIENT_SCREEN_SPLIT:
                        SplitKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_SCREEN_TIMER:
                        TimerKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_SCREEN_CHAR_NAME:
                        ChaNameKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_SCREEN_SAY:
                        SayKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_SCREEN_INPUTBOX:
                        IboxKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_SCREEN_DIALOG:
                        DlgKeyDown( true, dikdw, event_text );
                        ConsoleKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_SCREEN_BARTER:
                        DlgKeyDown( false, dikdw, event_text );
                        ConsoleKeyDown( dikdw, event_text );
                        break;
                    default:
                        ConsoleKeyDown( dikdw, event_text );
                        break;
                }
            }
            else
            {
                switch( GetMainScreen() )
                {
                    case CLIENT_MAIN_SCREEN_LOGIN:
                        LogKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_MAIN_SCREEN_CREDITS:
                        TryExit();
                        break;
                    case CLIENT_MAIN_SCREEN_GAME:
                        GameKeyDown( dikdw, event_text );
                        break;
                    case CLIENT_MAIN_SCREEN_WORLDMAP:
                        GmapKeyDown( dikdw, event_text );
                        break;
                    default:
                        break;
                }
                ConsoleKeyDown( dikdw, event_text );
            }

            ProcessKeybScroll( true, dikdw );
        }

        // Key up
        if( dikup )
        {
            if( GetActiveScreen() == CLIENT_SCREEN_INPUTBOX )
                IboxKeyUp( dikup );
            ConsoleKeyUp( dikup );
            ProcessKeybScroll( false, dikup );
            Timer::StartAccelerator( ACCELERATE_NONE );
        }
    }
}

void FOClient::ParseMouse()
{
    // Mouse position
    int mx = 0, my = 0;
    Fl::get_mouse( mx, my );
    #ifdef FO_D3D
    GameOpt.MouseX = mx - (!GameOpt.FullScreen ? MainWindow->x() : 0);
    GameOpt.MouseY = my - (!GameOpt.FullScreen ? MainWindow->y() : 0);
    #else
    GameOpt.MouseX = mx - MainWindow->x();
    GameOpt.MouseY = my - MainWindow->y();
    #endif
    GameOpt.MouseX = GameOpt.MouseX * MODE_WIDTH / MainWindow->w();
    GameOpt.MouseY = GameOpt.MouseY * MODE_HEIGHT / MainWindow->h();
    GameOpt.MouseX = CLAMP( GameOpt.MouseX, 0, MODE_WIDTH - 1 );
    GameOpt.MouseY = CLAMP( GameOpt.MouseY, 0, MODE_HEIGHT - 1 );

    // Stop processing if window not active
    if( !MainWindow->Focused )
    {
        MainWindow->MouseEvents.clear();
        IfaceHold = IFACE_NONE;
        Timer::StartAccelerator( ACCELERATE_NONE );
        if( Script::PrepareContext( ClientFunctions.InputLost, _FUNC_, "Game" ) )
            Script::RunPrepared();
        return;
    }

    // Accelerators
    if( Timer::GetAcceleratorNum() != ACCELERATE_NONE && !IsCurMode( CURSOR_WAIT ) )
    {
        int iface_hold = IfaceHold;
        if( Timer::ProcessAccelerator( ACCELERATE_MESSBOX ) )
            MessBoxLMouseDown();
        else if( Timer::ProcessAccelerator( ACCELERATE_SPLIT_UP ) )
            SplitLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_SPLIT_DOWN ) )
            SplitLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_TIMER_UP ) )
            TimerLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_TIMER_DOWN ) )
            TimerLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_USE_SCRUP ) )
            UseLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_USE_SCRDOWN ) )
            UseLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_INV_SCRUP ) )
            InvLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_INV_SCRDOWN ) )
            InvLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_PUP_SCRUP1 ) )
            PupLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_PUP_SCRDOWN1 ) )
            PupLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_PUP_SCRUP2 ) )
            PupLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_PUP_SCRDOWN2 ) )
            PupLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_CHA_SW_SCRUP ) )
            ChaLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_CHA_SW_SCRDOWN ) )
            ChaLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_CHA_PLUS ) )
            ChaLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_CHA_MINUS ) )
            ChaLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_CHA_AGE_UP ) )
            ChaAgeLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_CHA_AGE_DOWN ) )
            ChaAgeLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT1SU ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT1SD ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT2SU ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT2SD ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT1OSU ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT1OSD ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT2OSU ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_BARTER_CONT2OSD ) )
            DlgLMouseUp( false );
        else if( Timer::ProcessAccelerator( ACCELERATE_PERK_SCRUP ) )
            PerkLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_PERK_SCRDOWN ) )
            PerkLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_DLG_TEXT_UP ) )
            DlgLMouseUp( true );
        else if( Timer::ProcessAccelerator( ACCELERATE_DLG_TEXT_DOWN ) )
            DlgLMouseUp( true );
        else if( Timer::ProcessAccelerator( ACCELERATE_SAVE_LOAD_SCR_UP ) )
            SaveLoadLMouseUp();
        else if( Timer::ProcessAccelerator( ACCELERATE_SAVE_LOAD_SCR_DN ) )
            SaveLoadLMouseUp();
        IfaceHold = iface_hold;
    }

    // Mouse move
    static int old_cur_x = GameOpt.MouseX;
    static int old_cur_y = GameOpt.MouseY;
    if( old_cur_x != GameOpt.MouseX || old_cur_y != GameOpt.MouseY )
    {
        old_cur_x = GameOpt.MouseX;
        old_cur_y = GameOpt.MouseY;

        if( GetActiveScreen() )
        {
            switch( GetActiveScreen() )
            {
                case CLIENT_SCREEN_SPLIT:
                    SplitMouseMove();
                    break;
                case CLIENT_SCREEN_TIMER:
                    TimerMouseMove();
                    break;
                case CLIENT_SCREEN_DIALOGBOX:
                    DlgboxMouseMove();
                    break;
                case CLIENT_SCREEN_ELEVATOR:
                    ElevatorMouseMove();
                    break;
                case CLIENT_SCREEN_SAY:
                    SayMouseMove();
                    break;
                case CLIENT_SCREEN_INPUTBOX:
                    IboxMouseMove();
                    break;
                case CLIENT_SCREEN_SKILLBOX:
                    SboxMouseMove();
                    break;
                case CLIENT_SCREEN_USE:
                    UseMouseMove();
                    break;
                case CLIENT_SCREEN_PERK:
                    PerkMouseMove();
                    break;
                case CLIENT_SCREEN_WM_TOWNVIEW:
                    TViewMouseMove();
                    break;
                case CLIENT_SCREEN_DIALOG:
                    DlgMouseMove( true );
                    break;
                case CLIENT_SCREEN_BARTER:
                    DlgMouseMove( false );
                    break;
                case CLIENT_SCREEN_INVENTORY:
                    InvMouseMove();
                    break;
                case CLIENT_SCREEN_PICKUP:
                    PupMouseMove();
                    break;
                case CLIENT_SCREEN_MINIMAP:
                    LmapMouseMove();
                    break;
                case CLIENT_SCREEN_CHARACTER:
                    ChaMouseMove( false );
                    break;
                case CLIENT_SCREEN_PIPBOY:
                    PipMouseMove();
                    break;
                case CLIENT_SCREEN_FIXBOY:
                    FixMouseMove();
                    break;
                case CLIENT_SCREEN_AIM:
                    AimMouseMove();
                    break;
                case CLIENT_SCREEN_LOADSAVE:
                    SaveLoadMouseMove();
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch( GetMainScreen() )
            {
                case CLIENT_MAIN_SCREEN_WORLDMAP:
                    GmapMouseMove();
                    break;
                default:
                    break;
            }
        }

        ProcessMouseScroll();
        LMenuMouseMove();

        if( Script::PrepareContext( ClientFunctions.MouseMove, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( GameOpt.MouseX );
            Script::SetArgUInt( GameOpt.MouseY );
            Script::RunPrepared();
        }
    }

    // Get buffered data
    if( MainWindow->MouseEvents.empty() )
        return;
    IntVec events = MainWindow->MouseEvents;
    MainWindow->MouseEvents.clear();

    // Process events
    for( uint i = 0; i < events.size(); i += 3 )
    {
        int event = events[i];
        int event_button = events[i + 1];
        int event_dy = -events[i + 2];

        #ifndef FO_D3D
        // Stop video
        if( IsVideoPlayed() )
        {
            if( IsCanStopVideo() && (event == FL_PUSH && (event_button == FL_LEFT_MOUSE || event_button == FL_RIGHT_MOUSE) ) )
            {
                NextVideo();
                return;
            }
            continue;
        }
        #endif

        // Scripts
        bool script_result = false;
        if( event == FL_MOUSEWHEEL && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( event_dy > 0 ? MOUSE_CLICK_WHEEL_UP : MOUSE_CLICK_WHEEL_DOWN );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_LEFT_MOUSE && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_LEFT );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_LEFT_MOUSE && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_LEFT );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_RIGHT_MOUSE && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_RIGHT );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_RIGHT_MOUSE && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_RIGHT );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_MIDDLE_MOUSE && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_MIDDLE );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_MIDDLE_MOUSE && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_MIDDLE );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();

            // Normalize zoom
            if( !script_result && !GameOpt.DisableMouseEvents && !ConsoleActive && GameOpt.MapZooming && GameOpt.SpritesZoomMin != GameOpt.SpritesZoomMax )
            {
                int screen = GetActiveScreen();
                if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && (screen == CLIENT_SCREEN_NONE || screen == CLIENT_SCREEN_WM_TOWNVIEW) )
                {
                    HexMngr.ChangeZoom( 0 );
                    RebuildLookBorders = true;
                }
            }
        }
        if( event == FL_PUSH && event_button == FL_BUTTON( 1 ) && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT0 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_BUTTON( 1 ) && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT0 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_BUTTON( 2 ) && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT1 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_BUTTON( 2 ) && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT1 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_BUTTON( 3 ) && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT2 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_BUTTON( 3 ) && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT2 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_BUTTON( 4 ) && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT3 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_BUTTON( 4 ) && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT3 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_PUSH && event_button == FL_BUTTON( 5 ) && Script::PrepareContext( ClientFunctions.MouseDown, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT4 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }
        if( event == FL_RELEASE && event_button == FL_BUTTON( 5 ) && Script::PrepareContext( ClientFunctions.MouseUp, _FUNC_, "Game" ) )
        {
            Script::SetArgUInt( MOUSE_CLICK_EXT4 );
            if( Script::RunPrepared() )
                script_result = Script::GetReturnedBool();
        }

        if( script_result || GameOpt.DisableMouseEvents )
            continue;
        if( IsCurMode( CURSOR_WAIT ) )
            continue;

        // Wheel
        if( event == FL_MOUSEWHEEL )
        {
            ProcessMouseWheel( event_dy );
            continue;
        }

        // Left Button Down
        if( event == FL_PUSH && event_button == FL_LEFT_MOUSE )
        {
            if( GetActiveScreen() )
            {
                switch( GetActiveScreen() )
                {
                    case CLIENT_SCREEN_SPLIT:
                        SplitLMouseDown();
                        break;
                    case CLIENT_SCREEN_TIMER:
                        TimerLMouseDown();
                        break;
                    case CLIENT_SCREEN_DIALOGBOX:
                        DlgboxLMouseDown();
                        break;
                    case CLIENT_SCREEN_ELEVATOR:
                        ElevatorLMouseDown();
                        break;
                    case CLIENT_SCREEN_SAY:
                        SayLMouseDown();
                        break;
                    case CLIENT_SCREEN_CHAR_NAME:
                        ChaNameLMouseDown();
                        break;
                    case CLIENT_SCREEN_CHAR_AGE:
                        ChaAgeLMouseDown();
                        break;
                    case CLIENT_SCREEN_CHAR_SEX:
                        ChaSexLMouseDown();
                        break;
                    case CLIENT_SCREEN_WM_TOWN:
                        GmapLMouseDown();
                        break;
                    case CLIENT_SCREEN_INPUTBOX:
                        IboxLMouseDown();
                        break;
                    case CLIENT_SCREEN_SKILLBOX:
                        SboxLMouseDown();
                        break;
                    case CLIENT_SCREEN_USE:
                        UseLMouseDown();
                        break;
                    case CLIENT_SCREEN_PERK:
                        PerkLMouseDown();
                        break;
                    case CLIENT_SCREEN_WM_TOWNVIEW:
                        TViewLMouseDown();
                        break;
                    case CLIENT_SCREEN_INVENTORY:
                        InvLMouseDown();
                        break;
                    case CLIENT_SCREEN_PICKUP:
                        PupLMouseDown();
                        break;
                    case CLIENT_SCREEN_DIALOG:
                        DlgLMouseDown( true );
                        break;
                    case CLIENT_SCREEN_BARTER:
                        DlgLMouseDown( false );
                        break;
                    case CLIENT_SCREEN_MINIMAP:
                        LmapLMouseDown();
                        break;
                    case CLIENT_SCREEN_MENU:
                        MoptLMouseDown();
                        break;
                    case CLIENT_SCREEN_CHARACTER:
                        ChaLMouseDown( false );
                        break;
                    case CLIENT_SCREEN_PIPBOY:
                        PipLMouseDown();
                        break;
                    case CLIENT_SCREEN_FIXBOY:
                        FixLMouseDown();
                        break;
                    case CLIENT_SCREEN_AIM:
                        AimLMouseDown();
                        break;
                    case CLIENT_SCREEN_LOADSAVE:
                        SaveLoadLMouseDown();
                        break;
                    default:
                        break;
                }
            }
            else
            {
                switch( GetMainScreen() )
                {
                    case CLIENT_MAIN_SCREEN_GAME:
                        if( IntLMouseDown() == IFACE_NONE )
                            GameLMouseDown();
                        break;
                    case CLIENT_MAIN_SCREEN_WORLDMAP:
                        GmapLMouseDown();
                        break;
                    case CLIENT_MAIN_SCREEN_LOGIN:
                        LogLMouseDown();
                        break;
                    case CLIENT_MAIN_SCREEN_REGISTRATION:
                        ChaLMouseDown( true );
                        break;
                    case CLIENT_MAIN_SCREEN_CREDITS:
                        TryExit();
                        break;
                    default:
                        break;
                }
            }

			if (IsCurInRect(IntWMessTabs)) {
				MessBoxTabLMouseDown();
			}
			if( MessBoxLMouseDown() )
                Timer::StartAccelerator( ACCELERATE_MESSBOX );
            continue;
        }

        // Left Button Up
        if( event == FL_RELEASE && event_button == FL_LEFT_MOUSE )
        {
            if( GetActiveScreen() )
            {
                switch( GetActiveScreen() )
                {
                    case CLIENT_SCREEN_SPLIT:
                        SplitLMouseUp();
                        break;
                    case CLIENT_SCREEN_TIMER:
                        TimerLMouseUp();
                        break;
                    case CLIENT_SCREEN_DIALOGBOX:
                        DlgboxLMouseUp();
                        break;
                    case CLIENT_SCREEN_ELEVATOR:
                        ElevatorLMouseUp();
                        break;
                    case CLIENT_SCREEN_SAY:
                        SayLMouseUp();
                        break;
                    case CLIENT_SCREEN_CHAR_AGE:
                        ChaAgeLMouseUp();
                        break;
                    case CLIENT_SCREEN_CHAR_SEX:
                        ChaSexLMouseUp();
                        break;
                    case CLIENT_SCREEN_WM_TOWN:
                        GmapLMouseUp();
                        break;
                    case CLIENT_SCREEN_INPUTBOX:
                        IboxLMouseUp();
                        break;
                    case CLIENT_SCREEN_SKILLBOX:
                        SboxLMouseUp();
                        break;
                    case CLIENT_SCREEN_USE:
                        UseLMouseUp();
                        break;
                    case CLIENT_SCREEN_PERK:
                        PerkLMouseUp();
                        break;
                    case CLIENT_SCREEN_WM_TOWNVIEW:
                        TViewLMouseUp();
                        break;
                    case CLIENT_SCREEN_INVENTORY:
                        InvLMouseUp();
                        break;
                    case CLIENT_SCREEN_PICKUP:
                        PupLMouseUp();
                        break;
                    case CLIENT_SCREEN_DIALOG:
                        DlgLMouseUp( true );
                        break;
                    case CLIENT_SCREEN_BARTER:
                        DlgLMouseUp( false );
                        break;
                    case CLIENT_SCREEN_MINIMAP:
                        LmapLMouseUp();
                        break;
                    case CLIENT_SCREEN_MENU:
                        MoptLMouseUp();
                        break;
                    case CLIENT_SCREEN_CHARACTER:
                        ChaLMouseUp( false );
                        break;
                    case CLIENT_SCREEN_PIPBOY:
                        PipLMouseUp();
                        break;
                    case CLIENT_SCREEN_FIXBOY:
                        FixLMouseUp();
                        break;
                    case CLIENT_SCREEN_AIM:
                        AimLMouseUp();
                        break;
                    case CLIENT_SCREEN_LOADSAVE:
                        SaveLoadLMouseUp();
                        break;
                    default:
                        break;
                }
            }
            else
            {
                switch( GetMainScreen() )
                {
                    case CLIENT_MAIN_SCREEN_GAME:
                        (IfaceHold == IFACE_NONE) ? GameLMouseUp() : IntLMouseUp();
                        break;
                    case CLIENT_MAIN_SCREEN_WORLDMAP:
                        GmapLMouseUp();
                        break;
                    case CLIENT_MAIN_SCREEN_LOGIN:
                        LogLMouseUp();
                        break;
                    case CLIENT_MAIN_SCREEN_REGISTRATION:
                        ChaLMouseUp( true );
                        break;
                    default:
                        break;
                }
            }

            LMenuMouseUp();
            Timer::StartAccelerator( ACCELERATE_NONE );
            continue;
        }

        // Right Button Down
        if( event == FL_PUSH && event_button == FL_RIGHT_MOUSE )
        {
            if( GetActiveScreen() )
            {
                switch( GetActiveScreen() )
                {
                    case CLIENT_SCREEN_USE:
                        UseRMouseDown();
                        break;
                    case CLIENT_SCREEN_INVENTORY:
                        InvRMouseDown();
                        break;
                    case CLIENT_SCREEN_DIALOG:
                        DlgRMouseDown( true );
                        break;
                    case CLIENT_SCREEN_BARTER:
                        DlgRMouseDown( false );
                        break;
                    case CLIENT_SCREEN_PICKUP:
                        PupRMouseDown();
                        break;
                    case CLIENT_SCREEN_PIPBOY:
                        PipRMouseDown();
                        break;
					case CLIENT_SCREEN_FIXBOY:
						FixRMouseDown();
						break;
                    default:
                        break;
                }
            }
            else
            {
                switch( GetMainScreen() )
                {
                    case CLIENT_MAIN_SCREEN_GAME:
                        IntRMouseDown();
                        GameRMouseDown();
                        break;
                    case CLIENT_MAIN_SCREEN_WORLDMAP:
                        GmapRMouseDown();
                        break;
                    default:
                        break;
                }
            }
            continue;
        }

        // Right Button Up
        if( event == FL_RELEASE && event_button == FL_RIGHT_MOUSE )
        {
			if (GetActiveScreen())
			{
				switch (GetActiveScreen())
				{
					case CLIENT_SCREEN_FIXBOY:
						FixRMouseUp();
						break;
					default:
						break;
				}
			} else {
                switch( GetMainScreen() )
                {
                    case CLIENT_MAIN_SCREEN_GAME:
                        GameRMouseUp();
                        break;
                    case CLIENT_MAIN_SCREEN_WORLDMAP:
                        GmapRMouseUp();
                        break;
                    default:
                        break;
                }
            }
            continue;
        }
    }
}

void ContainerWheelScroll( int items_count, int cont_height, int item_height, int& cont_scroll, int wheel_data )
{
    int height_items = cont_height / item_height;
    int scroll = 1;
    if( Keyb::ShiftDwn )
        scroll = height_items;
    if( wheel_data > 0 )
        scroll = -scroll;
    cont_scroll += scroll;
    if( cont_scroll < 0 )
        cont_scroll = 0;
    else if( cont_scroll > items_count - height_items )
        cont_scroll = max( 0, items_count - height_items );
}

void FOClient::ProcessMouseWheel( int data )
{
    int screen = GetActiveScreen();

/************************************************************************/
/* Split value                                                          */
/************************************************************************/
    if( screen == CLIENT_SCREEN_SPLIT )
    {
        if( IsCurInRect( SplitWValue, SplitX, SplitY ) || IsCurInRect( SplitWItem, SplitX, SplitY ) )
        {
            if( data > 0 && SplitValue < SplitMaxValue )
                SplitValue++;
            else if( data < 0 && SplitValue > SplitMinValue )
                SplitValue--;
        }
    }
/************************************************************************/
/* Timer value                                                          */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_TIMER )
    {
        if( IsCurInRect( TimerWValue, TimerX, TimerY ) || IsCurInRect( TimerWItem, TimerX, TimerY ) )
        {
            if( data > 0 && TimerValue < TIMER_MAX_VALUE )
                TimerValue++;
            else if( data < 0 && TimerValue > TIMER_MIN_VALUE )
                TimerValue--;
        }
    }
/************************************************************************/
/* Use scroll                                                           */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_USE )
    {
        if( IsCurInRect( UseWInv, UseX, UseY ) )
            ContainerWheelScroll( (int)UseCont.size(), UseWInv.H(), UseHeightItem, UseScroll, data );
    }
/************************************************************************/
/* PerkUp scroll                                                        */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_PERK )
    {
        if( data > 0 && IsCurInRect( PerkWPerks, PerkX, PerkY ) && PerkScroll > 0 )
            PerkScroll--;
        else if( data < 0 && IsCurInRect( PerkWPerks, PerkX, PerkY ) && PerkScroll < (int)PerkCollection.size() - 1 )
            PerkScroll++;
    }
/************************************************************************/
/* Local, global map zoom, global tabs scroll, mess box scroll          */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_NONE || screen == CLIENT_SCREEN_WM_TOWNVIEW )
    {
        Rect r = MessBoxCurRectDraw();
		if( !r.IsZero() && IsCurInRect( r ) )
        {
			if (Keyb::CtrlDwn) {
				if (data > 0) {
					messboxFont = fontManager.getPrev(messboxFont);
					MessBoxGenerate();
				}
				if (data < 0) {
					messboxFont = fontManager.getNext(messboxFont);
					MessBoxGenerate();
				}
			}
			else {
				if (data > 0)
				{
					if (GameOpt.MsgboxInvert && MessBoxScroll > 0)
						MessBoxScroll--;
					if (!GameOpt.MsgboxInvert && MessBoxScroll < MessBoxMaxScroll)
						MessBoxScroll++;
					MessBoxGenerate();
				}
				else
				{
					if (GameOpt.MsgboxInvert && MessBoxScroll < MessBoxMaxScroll)
						MessBoxScroll++;
					if (!GameOpt.MsgboxInvert && MessBoxScroll > 0)
						MessBoxScroll--;
					MessBoxGenerate();
				}
			}
        }
        else if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) )
        {
			if( IntVisible && Chosen && IsCurInRect( IntBItem ) )
            {
                bool send = false;
                if( data > 0 )
                    send = Chosen->NextRateItem( true );
                else
                    send = Chosen->NextRateItem( false );
                if( send )
                    Net_SendRateItem();
            }

            if( !ConsoleActive && GameOpt.MapZooming && IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && GameOpt.SpritesZoomMin != GameOpt.SpritesZoomMax )
            {
                HexMngr.ChangeZoom( data > 0 ? -1 : 1 );
                RebuildLookBorders = true;
            }
        }
        else if( IsMainScreen( CLIENT_MAIN_SCREEN_WORLDMAP ) )
        {
			GmapChangeZoom( (float)-data / 20.0f );
            if( IsCurInRect( WorldmapWTabs ) )
            {
                if( data > 0 )
                {
                    if( WorldmapTabNextX )
                        WorldmapTabsScrX -= 26;
                    if( WorldmapTabNextY )
                        WorldmapTabsScrY -= 26;
                    if( WorldmapTabsScrX < 0 )
                        WorldmapTabsScrX = 0;
                    if( WorldmapTabsScrY < 0 )
                        WorldmapTabsScrY = 0;
                }
                else
                {
                    if( WorldmapTabNextX )
                        WorldmapTabsScrX += 26;
                    if( WorldmapTabNextY )
                        WorldmapTabsScrY += 26;

                    int tabs_count = 0;
                    for( uint i = 0, j = (uint)GmapLoc.size(); i < j; i++ )
                    {
                        GmapLocation& loc = GmapLoc[i];
                        if( MsgGM->Count( STR_GM_LABELPIC_( loc.LocPid ) ) && ResMngr.GetIfaceAnim( Str::GetHash( MsgGM->GetStr( STR_GM_LABELPIC_( loc.LocPid ) ) ) ) )
                            tabs_count++;
                    }
                    if( WorldmapTabNextX && WorldmapTabsScrX > WorldmapWTab.W() * tabs_count )
                        WorldmapTabsScrX = WorldmapWTab.W() * tabs_count;
                    if( WorldmapTabNextY && WorldmapTabsScrY > WorldmapWTab.H() * tabs_count )
                        WorldmapTabsScrY = WorldmapWTab.H() * tabs_count;
                }
            }
        }
    }
/************************************************************************/
/* Inventory scroll                                                     */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_INVENTORY )
    {
        if( IsCurInRect( InvWInv, InvX, InvY ) )
            ContainerWheelScroll( (int)InvCont.size(), InvWInv.H(), InvHeightItem, InvScroll, data );
        else if( IsCurInRect( InvWText, InvX, InvY ) )
        {
            if( data > 0 )
            {
                if( InvItemInfoScroll > 0 )
                    InvItemInfoScroll--;
            }
            else
            {
                if( InvItemInfoScroll < InvItemInfoMaxScroll )
                    InvItemInfoScroll++;
            }
        }
    }
/************************************************************************/
/* Dialog texts, answers                                                */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_DIALOG )
    {
		if (Keyb::CtrlDwn) {
			
			if (IsCurInRect(DlgWText, DlgX, DlgY) || IsCurInRect(DlgAnswText, DlgX, DlgY)) {
				//	scrolled up
				if (data > 0) {
					dialogFont = fontManager.getPrev(dialogFont);
					RecalcDlgMainTextLinesRect();
					RecalcDlgMainTextLinesReal();
					RecalcDlgAnswerPositions();
				}
				//	scrolled down
				if (data < 0) {
					dialogFont = fontManager.getNext(dialogFont);
					RecalcDlgMainTextLinesRect();
					RecalcDlgMainTextLinesReal();
					RecalcDlgAnswerPositions();
				}
			}
		} else {
			if (IsCurInRect(DlgWText, DlgX, DlgY))
			{
				if (data > 0 && DlgMainTextCur > 0)
					DlgMainTextCur--;
				else if (data < 0 && DlgMainTextCur < DlgMainTextLinesReal - DlgMainTextLinesRect + 2)
					DlgMainTextCur++;
			}
			else if (IsCurInRect(DlgAnswText, DlgX, DlgY))
			{
				DlgCollectAnswers(data < 0);
			}
		}
    }
/************************************************************************/
/* Barter scroll                                                        */
/************************************************************************/
	else if (screen == CLIENT_SCREEN_BARTER)
	{
		if (Keyb::CtrlDwn) {

			if (IsCurInRect(DlgWText, DlgX, DlgY)) {
				//	scrolled up
				if (data > 0) {
					dialogFont = fontManager.getPrev(dialogFont);
					RecalcDlgMainTextLinesRect();
					RecalcDlgMainTextLinesReal();
				}
				//	scrolled down
				if (data < 0) {
					dialogFont = fontManager.getNext(dialogFont);
					RecalcDlgMainTextLinesRect();
					RecalcDlgMainTextLinesReal();
				}
			}
		}
		else {
			if (IsCurInRect(BarterWCont1, DlgX, DlgY))
				ContainerWheelScroll((int)BarterCont1.size(), BarterWCont1.H(), BarterCont1HeightItem, BarterScroll1, data);
			else if (IsCurInRect(BarterWCont2, DlgX, DlgY))
				ContainerWheelScroll((int)BarterCont2.size(), BarterWCont2.H(), BarterCont2HeightItem, BarterScroll2, data);
			else if (IsCurInRect(BarterWCont1o, DlgX, DlgY))
				ContainerWheelScroll((int)BarterCont1o.size(), BarterWCont1o.H(), BarterCont1oHeightItem, BarterScroll1o, data);
			else if (IsCurInRect(BarterWCont2o, DlgX, DlgY))
				ContainerWheelScroll((int)BarterCont2o.size(), BarterWCont2o.H(), BarterCont2oHeightItem, BarterScroll2o, data);
			else if (IsCurInRect(DlgWText, DlgX, DlgY))
			{
				if (data > 0 && DlgMainTextCur > 0)
					DlgMainTextCur--;
				else if (data < 0 && DlgMainTextCur < DlgMainTextLinesReal - DlgMainTextLinesRect)
					DlgMainTextCur++;
			}
		}
	}
/************************************************************************/
/* PickUp scroll                                                        */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_PICKUP )
    {
        if( IsCurInRect( PupWCont1, PupX, PupY ) )
            ContainerWheelScroll( (int)PupCont1.size(), PupWCont1.H(), PupHeightItem1, PupScroll1, data );
        else if( IsCurInRect( PupWCont2, PupX, PupY ) )
            ContainerWheelScroll( (int)PupCont2.size(), PupWCont2.H(), PupHeightItem2, PupScroll2, data );
    }
/************************************************************************/
/* Character switch scroll                                              */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_CHARACTER )
    {
        if( data > 0 )
        {
            if( IsCurInRect( ChaTSwitch, ChaX, ChaY ) && ChaSwitchScroll[ChaCurSwitch] > 0 )
                ChaSwitchScroll[ChaCurSwitch]--;
        }
        else
        {
            if( IsCurInRect( ChaTSwitch, ChaX, ChaY ) )
            {
                int               max_lines = ChaTSwitch.H() / 11;
                SwitchElementVec& text = ChaSwitchText[ChaCurSwitch];
                int&              scroll = ChaSwitchScroll[ChaCurSwitch];
                if( scroll + max_lines < (int)text.size() )
                    scroll++;
            }
        }

    }
/************************************************************************/
/* Minimap zoom                                                         */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_MINIMAP )
    {
        if( IsCurInRect( LmapWMap, LmapX, LmapY ) )
        {
            if( data > 0 )
                LmapZoom++;
            else
                LmapZoom--;
            LmapZoom = CLAMP( LmapZoom, 2, 13 );
            LmapPrepareMap();
        }
    }
/************************************************************************/
/* PipBoy scroll                                                        */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_PIPBOY )
    {
        if( IsCurInRect( PipWMonitor, PipX, PipY ) )
        {
            if( PipMode != PIP__AUTOMAPS_MAP )
            {
				if (Keyb::CtrlDwn) {

					//	scrolled up
					if (data > 0) {
						pipboyFont = fontManager.getPrev(pipboyFont);
					}
					//	scrolled down
					if (data < 0) {
						pipboyFont = fontManager.getNext(pipboyFont);
					}
					return;
				}
				int scroll = 1;
				if( Keyb::ShiftDwn )
                    scroll = SprMngr.GetLinesCount( 0, PipWMonitor.H(), NULL, pipboyFont);
                if( data > 0 )
                    scroll = -scroll;
                PipScroll[PipMode] += scroll;
                if( PipScroll[PipMode] < 0 )
                    PipScroll[PipMode] = 0;
                #pragma MESSAGE("Check maximums in PipBoy scrolling.")
            }
            else
            {
                float scr_x = ( (float)(PipWMonitor.CX() + PipX) - AutomapScrX ) * AutomapZoom;
                float scr_y = ( (float)(PipWMonitor.CY() + PipY) - AutomapScrY ) * AutomapZoom;
                if( data > 0 )
                    AutomapZoom -= 0.1f;
                else
                    AutomapZoom += 0.1f;
                AutomapZoom = CLAMP( AutomapZoom, 0.1f, 10.0f );
                AutomapScrX = (float)(PipWMonitor.CX() + PipX) - scr_x / AutomapZoom;
                AutomapScrY = (float)(PipWMonitor.CY() + PipY) - scr_y / AutomapZoom;
            }
        }
    }
	/************************************************************************/
	/* FixBoy scroll                                                        */
	/************************************************************************/
	else if (screen == CLIENT_SCREEN_FIXBOY)
	{
		if (IsCurInRect(FixWWin, FixX, FixY))
		{
			//	font increase/decrease
			if (Keyb::CtrlDwn) {
				//	scrolled up
				if (data > 0) {
					fixboyFont = fontManager.getPrev(fixboyFont);
					if (FixMode == FIX_MODE_LIST)
						FixGenerate(FIX_MODE_LIST);
				}
				//	scrolled down
				if (data < 0) {
					fixboyFont = fontManager.getNext(fixboyFont);
					if (FixMode == FIX_MODE_LIST)
						FixGenerate(FIX_MODE_LIST);
				}
				return;
			} else {
				if (FixMode == FIX_MODE_LIST)
				{
					if (data < 0) {
						//	scroll down
						if (FixScrollLst < (int)FixCraftLst.size() - 1)
							FixScrollLst++;
					} else {
						//	scroll up	
						if (FixScrollLst > 0)
							FixScrollLst--;
					}
				}
			}
		}
	}
/************************************************************************/
/* Save/Load                                                            */
/************************************************************************/
    else if( screen == CLIENT_SCREEN_LOADSAVE )
    {
        int ox = (SaveLoadLoginScreen ? SaveLoadCX : SaveLoadX);
        int oy = (SaveLoadLoginScreen ? SaveLoadCY : SaveLoadY);

        if( IsCurInRect( SaveLoadSlots, ox, oy ) )
        {
            if( data > 0 )
            {
                if( SaveLoadSlotScroll > 0 )
                    SaveLoadSlotScroll--;
            }
            else
            {
                int max = (int)SaveLoadDataSlots.size() - SaveLoadSlotsMax + (SaveLoadSave ? 1 : 0);
                if( SaveLoadSlotScroll < max )
                    SaveLoadSlotScroll++;
            }
        }
    }
/************************************************************************/
/*                                                                      */
/************************************************************************/
}

bool FOClient::InitNet()
{
    WriteLog( "Network init...\n" );
    IsConnected = false;

    #ifdef FO_WINDOWS
    WSADATA wsa;
    if( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) )
    {
        WriteLog( "WSAStartup error<%s>.\n", GetLastSocketError() );
        return false;
    }
    #endif

    if( !Singleplayer )
    {
        if( !FillSockAddr( SockAddr, GameOpt.Host.c_str(), GameOpt.Port ) )
            return false;
        if( GameOpt.ProxyType && !FillSockAddr( ProxyAddr, GameOpt.ProxyHost.c_str(), GameOpt.ProxyPort ) )
            return false;
    }
    else
    {
        #ifdef FO_WINDOWS
        for( int i = 0; i < 60; i++ )     // Wait 1 minute, then abort
        {
            if( !SingleplayerData.Refresh() )
                return false;
            if( SingleplayerData.NetPort )
                break;
            Thread::Sleep( 1000 );
        }
        if( !SingleplayerData.NetPort )
            return false;
        SockAddr.sin_family = AF_INET;
        SockAddr.sin_port = SingleplayerData.NetPort;
        SockAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
        #endif
    }

    if( !NetConnect() )
        return false;
    IsConnected = true;
    WriteLog( "Network init successful.\n" );
    return true;
}

bool FOClient::FillSockAddr( sockaddr_in& saddr, const char* host, uint16 port )
{
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons( port );
    if( (saddr.sin_addr.s_addr = inet_addr( host ) ) == uint( -1 ) )
    {
        hostent* h = gethostbyname( host );
        if( !h )
        {
            WriteLogF( _FUNC_, " - Can't resolve remote host<%s>, error<%s>.", host, GetLastSocketError() );
            return false;
        }

        memcpy( &saddr.sin_addr, h->h_addr, sizeof(in_addr) );
    }
    return true;
}

bool FOClient::NetConnect()
{
    if( !Singleplayer )
        WriteLog( "Connecting to server<%s:%d>.\n", GameOpt.Host.c_str(), GameOpt.Port );
    else
        WriteLog( "Connecting to server.\n" );

    Bin.SetEncryptKey( 0 );
    Bout.SetEncryptKey( 0 );

    #ifdef FO_WINDOWS
    if( (Sock = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0 ) ) == INVALID_SOCKET )
    #else
    if( (Sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == INVALID_SOCKET )
    #endif
    {
        WriteLog( "Create socket error<%s>.\n", GetLastSocketError() );
        return false;
    }

    // Nagle
    #ifdef FO_WINDOWS
    if( GameOpt.DisableTcpNagle )
    {
        int optval = 1;
        if( setsockopt( Sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval) ) )
            WriteLog( "Can't set TCP_NODELAY (disable Nagle) to socket, error<%s>.\n", WSAGetLastError() );
    }
    #endif

    // Direct connect
    if( !GameOpt.ProxyType || Singleplayer )
    {
        if( connect( Sock, (sockaddr*)&SockAddr, sizeof(sockaddr_in) ) )
        {
            WriteLog( "Can't connect to game server, error<%s>.\n", GetLastSocketError() );
            return false;
        }
    }
    // Proxy connect
    else
    {
        if( connect( Sock, (sockaddr*)&ProxyAddr, sizeof(sockaddr_in) ) )
        {
            WriteLog( "Can't connect to proxy server, error<%s>.\n", GetLastSocketError() );
            return false;
        }

// ==========================================
        #define SEND_RECV                                      \
            do {                                               \
                if( !NetOutput() )                             \
                {                                              \
                    WriteLog( "Net output error.\n" );         \
                    return false;                              \
                }                                              \
                uint tick = Timer::FastTick();                 \
                while( true )                                  \
                {                                              \
                    int receive = NetInput( false );           \
                    if( receive > 0 )                          \
                        break;                                 \
                    if( receive < 0 )                          \
                    {                                          \
                        WriteLog( "Net input error.\n" );      \
                        return false;                          \
                    }                                          \
                    if( Timer::FastTick() - tick > 10000 )     \
                    {                                          \
                        WriteLog( "Proxy answer timeout.\n" ); \
                        return false;                          \
                    }                                          \
                    Thread::Sleep( 1 );                        \
                }                                              \
            }                                                  \
            while( 0 )
// ==========================================

        uint8 b1, b2;
        Bin.Reset();
        Bout.Reset();
        // Authentication
        if( GameOpt.ProxyType == PROXY_SOCKS4 )
        {
            // Connect
            Bout << uint8( 4 );           // Socks version
            Bout << uint8( 1 );           // Connect command
            Bout << uint16( SockAddr.sin_port );
            Bout << uint( SockAddr.sin_addr.s_addr );
            Bout << uint8( 0 );
            SEND_RECV;
            Bin >> b1;             // Null byte
            Bin >> b2;             // Answer code
            if( b2 != 0x5A )
            {
                switch( b2 )
                {
                    case 0x5B:
                        WriteLog( "Proxy connection error, request rejected or failed.\n" );
                        break;
                    case 0x5C:
                        WriteLog( "Proxy connection error, request failed because client is not running identd (or not reachable from the server).\n" );
                        break;
                    case 0x5D:
                        WriteLog( "Proxy connection error, request failed because client's identd could not confirm the user ID string in the request.\n" );
                        break;
                    default:
                        WriteLog( "Proxy connection error, Unknown error<%u>.\n", b2 );
                        break;
                }
                return false;
            }
        }
        else if( GameOpt.ProxyType == PROXY_SOCKS5 )
        {
            Bout << uint8( 5 );                                                                // Socks version
            Bout << uint8( 1 );                                                                // Count methods
            Bout << uint8( 2 );                                                                // Method
            SEND_RECV;
            Bin >> b1;                                                                         // Socks version
            Bin >> b2;                                                                         // Method
            if( b2 == 2 )                                                                      // User/Password
            {
                Bout << uint8( 1 );                                                            // Subnegotiation version
                Bout << uint8( GameOpt.ProxyUser.length() );                                   // Name length
                Bout.Push( GameOpt.ProxyUser.c_str(), (uint)GameOpt.ProxyUser.length() );      // Name
                Bout << uint8( GameOpt.ProxyPass.length() );                                   // Pass length
                Bout.Push( GameOpt.ProxyPass.c_str(), (uint)GameOpt.ProxyPass.length() );      // Pass
                SEND_RECV;
                Bin >> b1;                                                                     // Subnegotiation version
                Bin >> b2;                                                                     // Status
                if( b2 != 0 )
                {
                    WriteLog( "Invalid proxy user or password.\n" );
                    return false;
                }
            }
            else if( b2 != 0 )         // Other authorization
            {
                WriteLog( "Socks server connect fail.\n" );
                return false;
            }
            // Connect
            Bout << uint8( 5 );           // Socks version
            Bout << uint8( 1 );           // Connect command
            Bout << uint8( 0 );           // Reserved
            Bout << uint8( 1 );           // IP v4 address
            Bout << uint( SockAddr.sin_addr.s_addr );
            Bout << uint16( SockAddr.sin_port );
            SEND_RECV;
            Bin >> b1;             // Socks version
            Bin >> b2;             // Answer code
            if( b2 != 0 )
            {
                switch( b2 )
                {
                    case 1:
                        WriteLog( "Proxy connection error, SOCKS-server error.\n" );
                        break;
                    case 2:
                        WriteLog( "Proxy connection error, connections fail by proxy rules.\n" );
                        break;
                    case 3:
                        WriteLog( "Proxy connection error, network is not aviable.\n" );
                        break;
                    case 4:
                        WriteLog( "Proxy connection error, host is not aviable.\n" );
                        break;
                    case 5:
                        WriteLog( "Proxy connection error, connection denied.\n" );
                        break;
                    case 6:
                        WriteLog( "Proxy connection error, TTL expired.\n" );
                        break;
                    case 7:
                        WriteLog( "Proxy connection error, command not supported.\n" );
                        break;
                    case 8:
                        WriteLog( "Proxy connection error, address type not supported.\n" );
                        break;
                    default:
                        WriteLog( "Proxy connection error, unknown error<%u>.\n", b2 );
                        break;
                }
                return false;
            }
        }
        else if( GameOpt.ProxyType == PROXY_HTTP )
        {
            char* buf = (char*)Str::FormatBuf( "CONNECT %s:%d HTTP/1.0\r\n\r\n", inet_ntoa( SockAddr.sin_addr ), GameOpt.Port );
            Bout.Push( buf, Str::Length( buf ) );
            SEND_RECV;
            buf = Bin.GetCurData();
            /*if(strstr(buf," 407 "))
               {
                    char* buf=(char*)Str::FormatBuf("CONNECT %s:%d HTTP/1.0\r\nProxy-authorization: basic \r\n\r\n",inet_ntoa(SockAddr.sin_addr),OptPort);
                    Bout.Push(buf,strlen(buf));
                    SEND_RECV;
                    buf=Bin.GetCurData();
               }*/
            if( !Str::Substring( buf, " 200 " ) )
            {
                WriteLog( "Proxy connection error, receive message<%s>.\n", buf );
                return false;
            }
        }
        else
        {
            WriteLog( "Unknown proxy type<%u>.\n", GameOpt.ProxyType );
            return false;
        }

        Bin.Reset();
        Bout.Reset();
    }

    WriteLog( "Connecting successful.\n" );

    ZStream.zalloc = zlib_alloc_;
    ZStream.zfree = zlib_free_;
    ZStream.opaque = NULL;
    if( inflateInit( &ZStream ) != Z_OK )
    {
        WriteLog( "ZStream InflateInit error.\n" );
        return false;
    }
    ZStreamOk = true;

    return true;
}

void FOClient::NetDisconnect()
{
    WriteLog( "Disconnect.\n" );

    if( ZStreamOk )
        inflateEnd( &ZStream );
    ZStreamOk = false;
    if( Sock != INVALID_SOCKET )
        closesocket( Sock );
    Sock = INVALID_SOCKET;
    IsConnected = false;

    WriteLog( "Traffic: send<%u>, receive<%u>, whole<%u>, receive real<%u>.\n",
              BytesSend, BytesReceive, BytesReceive + BytesSend, BytesRealReceive );

    SetCurMode( CURSOR_DEFAULT );
    HexMngr.UnloadMap();
    ClearCritters();
    QuestMngr.Finish();
    Bin.Reset();
    Bout.Reset();
    Bin.SetEncryptKey( 0 );
    Bout.SetEncryptKey( 0 );
    WriteLog( "Disconnect success.\n" );
}

void FOClient::ParseSocket()
{
    if( Sock == INVALID_SOCKET )
        return;

    if( NetInput( true ) < 0 )
    {
        IsConnected = false;
    }
    else
    {
        NetProcess();

        if( GameOpt.HelpInfo && Bout.IsEmpty() && !PingTick && GameOpt.PingPeriod && Timer::FastTick() >= PingCallTick )
        {
            Net_SendPing( PING_PING );
            PingTick = Timer::FastTick();
        }

        NetOutput();
    }

	if (!IsConnected) {
		WriteLog("DC: FOClient::ParseSocket()");
		NetDisconnect();
	}
}

bool FOClient::NetOutput()
{
    if( Bout.IsEmpty() )
        return true;

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO( &SockSet );
    FD_ZERO( &SockSetErr );
    FD_SET( Sock, &SockSet );
    FD_SET( Sock, &SockSetErr );
    if( select( Sock + 1, NULL, &SockSet, &SockSetErr, &tv ) == SOCKET_ERROR )
        WriteLogF( _FUNC_, " - Select error<%s>.\n", GetLastSocketError() );
    if( FD_ISSET( Sock, &SockSetErr ) )
        WriteLogF( _FUNC_, " - Socket error.\n" );
    if( !FD_ISSET( Sock, &SockSet ) )
        return true;

    int tosend = Bout.GetEndPos();
    int sendpos = 0;
    while( sendpos < tosend )
    {
        #ifdef FO_WINDOWS
        DWORD  len;
        WSABUF buf;
        buf.buf = Bout.GetData() + sendpos;
        buf.len = tosend - sendpos;
        if( WSASend( Sock, &buf, 1, &len, 0, NULL, NULL ) == SOCKET_ERROR || len == 0 )
        #else
        int len = send( Sock, Bout.GetData() + sendpos, tosend - sendpos, 0 );
        if( len <= 0 )
        #endif
        {
            WriteLogF( _FUNC_, " - Socket error while send to server, error<%s>.\n", GetLastSocketError() );
            IsConnected = false;
            return false;
        }
        sendpos += len;
        BytesSend += len;
    }

    Bout.Reset();
    return true;
}

int FOClient::NetInput( bool unpack )
{
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO( &SockSet );
    FD_ZERO( &SockSetErr );
    FD_SET( Sock, &SockSet );
    FD_SET( Sock, &SockSetErr );
    if( select( Sock + 1, &SockSet, NULL, &SockSetErr, &tv ) == SOCKET_ERROR )
        WriteLogF( _FUNC_, " - Select error<%s>.\n", GetLastSocketError() );
    if( FD_ISSET( Sock, &SockSetErr ) )
        WriteLogF( _FUNC_, " - Socket error.\n" );
    if( !FD_ISSET( Sock, &SockSet ) )
        return 0;

    #ifdef FO_WINDOWS
    DWORD  len;
    DWORD  flags = 0;
    WSABUF buf;
    buf.buf = ComBuf;
    buf.len = ComLen;
    if( WSARecv( Sock, &buf, 1, &len, &flags, NULL, NULL ) == SOCKET_ERROR )
    #else
    int len = recv( Sock, ComBuf, ComLen, 0 );
    if( len < 0 )
    #endif
    {
        WriteLogF( _FUNC_, " - Socket error while receive from server, error<%s>.\n", GetLastSocketError() );
        return -1;
    }
    if( len == 0 )
    {
        WriteLogF( _FUNC_, " - Socket is closed.\n" );
        return -2;
    }

    uint pos = len;
    while( pos == ComLen )
    {
        uint  newcomlen = (ComLen << 1);
        char* combuf = new char[newcomlen];
        memcpy( combuf, ComBuf, ComLen );
        SAFEDELA( ComBuf );
        ComBuf = combuf;
        ComLen = newcomlen;

        #ifdef FO_WINDOWS
        flags = 0;
        buf.buf = ComBuf + pos;
        buf.len = ComLen - pos;
        if( WSARecv( Sock, &buf, 1, &len, &flags, NULL, NULL ) == SOCKET_ERROR )
        #else
        int len = recv( Sock, ComBuf + pos, ComLen - pos, 0 );
        if( len < 0 )
        #endif
        {
            WriteLogF( _FUNC_, " - Socket error (2) while receive from server, error<%s>.\n", GetLastSocketError() );
            return -1;
        }
        if( len == 0 )
        {
            WriteLogF( _FUNC_, " - Socket is closed (2).\n" );
            return -2;
        }

        pos += len;
    }

    Bin.Refresh();
    uint old_pos = Bin.GetEndPos();     // Fix position

    if( unpack && !GameOpt.DisableZlibCompression )
    {
        ZStream.next_in = (uint8*)ComBuf;
        ZStream.avail_in = pos;
        ZStream.next_out = (uint8*)Bin.GetData() + Bin.GetEndPos();
        ZStream.avail_out = Bin.GetLen() - Bin.GetEndPos();

        if( inflate( &ZStream, Z_SYNC_FLUSH ) != Z_OK )
        {
            WriteLogF( _FUNC_, " - ZStream Inflate error.\n" );
            return -3;
        }

        Bin.SetEndPos( (uint)( (size_t)ZStream.next_out - (size_t)Bin.GetData() ) );

        while( ZStream.avail_in )
        {
            Bin.GrowBuf( 2048 );

            ZStream.next_out = (uint8*)Bin.GetData() + Bin.GetEndPos();
            ZStream.avail_out = Bin.GetLen() - Bin.GetEndPos();

            if( inflate( &ZStream, Z_SYNC_FLUSH ) != Z_OK )
            {
                WriteLogF( _FUNC_, " - ZStream Inflate continue error.\n" );
                return -4;
            }

            Bin.SetEndPos( (uint)( (size_t)ZStream.next_out - (size_t)Bin.GetData() ) );
        }
    }
    else
    {
        Bin.Push( ComBuf, pos, true );
    }

    BytesReceive += pos;
    BytesRealReceive += Bin.GetEndPos() - old_pos;
    return Bin.GetEndPos() - old_pos;
}

void FOClient::NetProcess()
{
    if( Bin.NeedProcessRaw() )
    {
        Bin.SetEncryptKey( 0 );
        Bout.SetEncryptKey( 0 );
        Bin.MoveReadPos( 4 );

        uint data = 0;
        Bin >> data;

        switch( data )
        {
            case NETRAW_INVALID_VERSION:
            {
                WriteLog( "Invalid version.\n" );

                const char* msg = NULL;
                if( CurLang.Msg[TEXTMSG_GAME].Count( STR_NET_WRONG_NETPROTO ) )
                    msg = CurLang.Msg[TEXTMSG_GAME].GetStr( STR_NET_WRONG_NETPROTO );

                if( msg && Str::Length( msg ) && !Str::Compare( msg, "error" ) )
                    AddMess( MSGBOX_GAME, msg );
                else
                    ShowMessage( "Please update the client.\nCurrently used version is no longer supported." );

                IsConnected = false;
                break;
            }
            case NETRAW_DISCONNECT:
            {
                IsConnected = false;
                break;
            }
            default:
                WriteLog( "Unknown raw data<0x%X>\n", data );
                break;
        }

        return;
    }

    while( Bin.NeedProcess() )
    {
        uint msg = 0;
        Bin >> msg;

        if( GameOpt.DebugNet )
        {
            #pragma TODO("DebugNet")
            static uint count = 0;
            AddMess( MSGBOX_GAME, Str::FormatBuf( "%04u) Input net message<%u>.", count, (msg >> 8) & 0xFF ) );
            WriteLog( "%04u) Input net message<%u>.\n", count, (msg >> 8) & 0xFF );
            count++;
        }

        switch( msg )
        {
            case NETMSG_LOGIN_SUCCESS:
				//	SASA: TODO - put script callback here for event then client logs in
                Net_OnLoginSuccess();
				HexMngr.ItemHighlightActive = false;
				HexMngr.ShowItemDurability = false;
                break;
            case NETMSG_REGISTER_SUCCESS:
                if( !Singleplayer )
                {
                    WriteLog( "Registration success.\n" );
                    if( RegNewCr )
                    {
                        GameOpt.Name = RegNewCr->Name;
                        Password = RegNewCr->Pass;
                        SAFEDEL( RegNewCr );
                    }
                }
                else
                {
                    WriteLog( "World loaded, enter to it.\n" );
                    LogTryConnect();
                }
                break;

            case NETMSG_PING:
                Net_OnPing();
                break;
            case NETMSG_CHECK_UID0:
                Net_OnCheckUID0();
                break;

            case NETMSG_ADD_PLAYER:
                Net_OnAddCritter( false );
                break;
            case NETMSG_ADD_NPC:
                Net_OnAddCritter( true );
                break;
            case NETMSG_REMOVE_CRITTER:
                Net_OnRemoveCritter();
                break;
            case NETMSG_SOME_ITEM:
                Net_OnSomeItem();
                break;
            case NETMSG_CRITTER_ACTION:
                Net_OnCritterAction();
                break;
            case NETMSG_CRITTER_KNOCKOUT:
                Net_OnCritterKnockout();
                break;
            case NETMSG_CRITTER_MOVE_ITEM:
                Net_OnCritterMoveItem();
                break;
            case NETMSG_CRITTER_ITEM_DATA:
                Net_OnCritterItemData();
                break;
            case NETMSG_CRITTER_ANIMATE:
                Net_OnCritterAnimate();
                break;
            case NETMSG_CRITTER_SET_ANIMS:
                Net_OnCritterSetAnims();
                break;
            case NETMSG_CRITTER_PARAM:
                Net_OnCritterParam();
                break;
            case NETMSG_CRITTER_MOVE:
                Net_OnCritterMove();
                break;
            case NETMSG_CRITTER_DIR:
                Net_OnCritterDir();
                break;
            case NETMSG_CRITTER_XY:
                Net_OnCritterXY();
                break;
            case NETMSG_CHECK_UID1:
                Net_OnCheckUID1();
                break;

            case NETMSG_CRITTER_TEXT:
                Net_OnText();
                break;
            case NETMSG_MSG:
                Net_OnTextMsg( false );
                break;
            case NETMSG_MSG_LEX:
                Net_OnTextMsg( true );
                break;
            case NETMSG_MAP_TEXT:
                Net_OnMapText();
                break;
            case NETMSG_MAP_TEXT_MSG:
                Net_OnMapTextMsg();
                break;
            case NETMSG_MAP_TEXT_MSG_LEX:
                Net_OnMapTextMsgLex();
                break;
            case NETMSG_CHECK_UID2:
                Net_OnCheckUID2();
                break;

            case NETMSG_ALL_PARAMS:
                Net_OnChosenParams();
                break;
            case NETMSG_PARAM:
                Net_OnChosenParam();
                break;
            case NETMSG_CRAFT_ASK:
                Net_OnCraftAsk();
                break;
            case NETMSG_CRAFT_RESULT:
                Net_OnCraftResult();
                break;
            case NETMSG_CLEAR_ITEMS:
                Net_OnChosenClearItems();
                break;
            case NETMSG_ADD_ITEM:
                Net_OnChosenAddItem();
                break;
            case NETMSG_REMOVE_ITEM:
                Net_OnChosenEraseItem();
                break;
            case NETMSG_CHECK_UID3:
                Net_OnCheckUID3();
                break;

            case NETMSG_TALK_NPC:
                Net_OnChosenTalk();
                break;

            case NETMSG_GAME_INFO:
                Net_OnGameInfo();
                break;

            case NETMSG_QUEST:
                Net_OnQuest( false );
                break;
            case NETMSG_QUESTS:
                Net_OnQuest( true );
                break;
            case NETMSG_HOLO_INFO:
                Net_OnHoloInfo();
                break;
            case NETMSG_SCORES:
                Net_OnScores();
                break;
            case NETMSG_USER_HOLO_STR:
                Net_OnUserHoloStr();
                break;
            case NETMSG_AUTOMAPS_INFO:
                Net_OnAutomapsInfo();
                break;
            case NETMSG_CHECK_UID4:
                Net_OnCheckUID4();
                break;
            case NETMSG_VIEW_MAP:
                Net_OnViewMap();
                break;

            case NETMSG_LOADMAP:
                Net_OnLoadMap();
                break;
            case NETMSG_MAP:
                Net_OnMap();
                break;
            case NETMSG_GLOBAL_INFO:
                Net_OnGlobalInfo();
                break;
            case NETMSG_GLOBAL_ENTRANCES:
                Net_OnGlobalEntrances();
                break;
            case NETMSG_CONTAINER_INFO:
                Net_OnContainerInfo();
                break;
            case NETMSG_FOLLOW:
                Net_OnFollow();
                break;
            case NETMSG_PLAYERS_BARTER:
                Net_OnPlayersBarter();
                break;
            case NETMSG_PLAYERS_BARTER_SET_HIDE:
                Net_OnPlayersBarterSetHide();
                break;
            case NETMSG_SHOW_SCREEN:
                Net_OnShowScreen();
                break;
            case NETMSG_RUN_CLIENT_SCRIPT:
                Net_OnRunClientScript();
                break;
            case NETMSG_DROP_TIMERS:
                Net_OnDropTimers();
                break;
            case NETMSG_CRITTER_LEXEMS:
                Net_OnCritterLexems();
                break;
            case NETMSG_ITEM_LEXEMS:
                Net_OnItemLexems();
                break;

            case NETMSG_ADD_ITEM_ON_MAP:
                Net_OnAddItemOnMap();
                break;
            case NETMSG_CHANGE_ITEM_ON_MAP:
                Net_OnChangeItemOnMap();
                break;
            case NETMSG_ERASE_ITEM_FROM_MAP:
                Net_OnEraseItemFromMap();
                break;
            case NETMSG_ANIMATE_ITEM:
                Net_OnAnimateItem();
                break;
            case NETMSG_COMBAT_RESULTS:
                Net_OnCombatResult();
                break;
            case NETMSG_EFFECT:
                Net_OnEffect();
                break;
            case NETMSG_FLY_EFFECT:
                Net_OnFlyEffect();
                break;
            case NETMSG_PLAY_SOUND:
                Net_OnPlaySound( false );
                break;
            case NETMSG_PLAY_SOUND_TYPE:
                Net_OnPlaySound( true );
                break;

            case NETMSG_MSG_DATA:
                Net_OnMsgData();
                break;
            case NETMSG_ITEM_PROTOS:
                Net_OnProtoItemData();
                break;

            default:
                if( GameOpt.DebugNet )
                    AddMess( MSGBOX_GAME, Str::FormatBuf( "Invalid msg<%u>. Seek valid.", (msg >> 8) & 0xFF ) );
                WriteLog( "Invalid msg<%u>. Seek valid.\n", msg );
                Bin.MoveReadPos( sizeof(uint) );
                Bin.SeekValidMsg();
                return;
        }

        if( !IsConnected )
            return;
    }
}

void FOClient::Net_SendLogIn( const char* name, const char* pass )
{
    char name_[UTF8_BUF_SIZE( MAX_NAME )] = { 0 };
    char pass_[UTF8_BUF_SIZE( MAX_NAME )] = { 0 };
    if( name )
        Str::Copy( name_, name );
    if( pass )
        Str::Copy( pass_, pass );

    uint uid1 = *UID1;
    Bout << NETMSG_LOGIN;
    Bout << (uint16)FOCLASSIC_STAGE;
    Bout << (uint16)FOCLASSIC_VERSION;
    uint uid4 = *UID4;
    Bout << uid4;
    uid4 = uid1;                                                                                                                                                        // UID4

    // Begin data encrypting
    Bout.SetEncryptKey( *UID4 + NETSALT_LOGIN );
    Bin.SetEncryptKey( *UID4 - NETSALT_LOGIN );

    Bout.Push( name_, sizeof(name_) );
    Bout << uid1;
    uid4 ^= uid1 * Random( 0, 432157 ) + *UID3;                                                                                                 // UID1
    char pass_hash[PASS_HASH_SIZE];
    Crypt.ClientPassHash( name_, pass_, pass_hash );
    Bout.Push( pass_hash, PASS_HASH_SIZE );
    uint uid2 = *UID2;
    Bout << CurLang.Name;
    for( int i = 0; i < TEXTMSG_MAX; i++ )
        Bout << CurLang.Msg[i].GetHash();
    Bout << UIDXOR;                                                                                                                                                             // UID xor
    uint uid3 = *UID3;
    Bout << uid3;
    uid3 ^= uid1;
    uid1 |= uid3 / Random( 2, 53 );                                                                                                             // UID3
    Bout << uid2;
    uid3 ^= uid2;
    uid1 |= uid4 + 222 - *UID2;                                                                                                                 // UID2
    Bout << UIDOR;                                                                                                                              // UID or
    for( int i = 0; i < ITEM_TYPE_MAX; i++ )
        Bout << ItemMngr.GetProtosHash( i );
    Bout << UIDCALC;                                                                                                                            // UID uidcalc
    Bout << GameOpt.DefaultCombatMode;
    uint uid0 = *UID0;
    Bout << uid0;
    uid3 ^= uid1 + Random( 0, 53245 );
    uid1 |= uid3 + *UID0;
    uid3 ^= uid1 + uid3;
    uid1 |= uid3;                                                                                       // UID0

    char dummy[100];
    Bout.Push( dummy, 100 );

    if( !Singleplayer && name )
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_CONN_SUCCESS ) );
}

void FOClient::Net_SendCreatePlayer( CritterCl* newcr )
{
    WriteLog( "Player registration..." );

    if( !newcr )
    {
        WriteLog( "internal error.\n" );
        return;
    }

    uint16 count = 0;
    for( int i = 0; i < MAX_PARAMS; i++ )
        if( newcr->ParamsReg[i] && CritterCl::ParamsRegEnabled[i] )
            count++;

    uint msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(uint16) + UTF8_BUF_SIZE( MAX_NAME ) + PASS_HASH_SIZE + sizeof(count) + (sizeof(uint16) + sizeof(int) ) * count;

    Bout << NETMSG_REGISTER;
    Bout << msg_len;

    Bout << (uint16)FOCLASSIC_STAGE;
    Bout << (uint16)FOCLASSIC_VERSION;

    // Begin data encrypting
    Bout.SetEncryptKey( 892018 + NETSALT_REGISTER );
    Bin.SetEncryptKey( 892018 - NETSALT_REGISTER );

    Bout.Push( Str::FormatBuf( "%s", newcr->GetName() ), UTF8_BUF_SIZE( MAX_NAME ) );
    char pass_hash[PASS_HASH_SIZE];
    Crypt.ClientPassHash( newcr->GetName(), newcr->GetPass(), pass_hash );
    Bout.Push( pass_hash, PASS_HASH_SIZE );

    Bout << count;
    for( uint16 i = 0; i < MAX_PARAMS; i++ )
    {
        int val = newcr->ParamsReg[i];
        if( val && CritterCl::ParamsRegEnabled[i] )
        {
            Bout << i;
            Bout << val;
        }
    }

    WriteLog( "complete.\n" );
}

void FOClient::Net_SendSaveLoad( bool save, const char* fname, UInt8Vec* pic_data )
{
    if( save )
        WriteLog( "Request save to file<%s>...", fname );
    else
        WriteLog( "Request load from file<%s>...", fname );

    uint   msg = NETMSG_SINGLEPLAYER_SAVE_LOAD;
    uint16 fname_len = Str::Length( fname );
    uint   msg_len = sizeof(msg) + sizeof(save) + sizeof(fname_len) + fname_len;
    if( save )
        msg_len += sizeof(uint) + (uint)pic_data->size();

    Bout << msg;
    Bout << msg_len;
    Bout << save;
    Bout << fname_len;
    Bout.Push( fname, fname_len );
    if( save )
    {
        Bout << (uint)pic_data->size();
        if( pic_data->size() )
            Bout.Push( (char*)&(*pic_data)[0], (uint)pic_data->size() );
    }

    WriteLog( "complete.\n" );
}

void FOClient::Net_SendText( const char* send_str, uint8 how_say )
{
    if( !send_str || !send_str[0] )
        return;

    char  str_buf[MAX_FOTEXT];
    char* str = str_buf;
    Str::Copy( str_buf, send_str );

    bool result = false;
    if( Script::PrepareContext( ClientFunctions.OutMessage, _FUNC_, "Game" ) )
    {
        int           say_type = how_say;
        ScriptString* sstr = new ScriptString( str );
        Script::SetArgObject( sstr );
        Script::SetArgAddress( &say_type );
        if( Script::RunPrepared() )
            result = Script::GetReturnedBool();
        Str::Copy( str, MAX_FOTEXT, sstr->c_str() );
        sstr->Release();
        how_say = say_type;
	}

    if( !result || !str[0] )
        return;

    if( str[0] == '~' )
    {
        str++;
        Net_SendCommand( str );
        return;
    }

    uint16 len = Str::Length( str );
    uint   msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(how_say) + sizeof(len) + len;

    Bout << NETMSG_SEND_TEXT;
    Bout << msg_len;
    Bout << how_say;
    Bout << len;
    Bout.Push( str, len );
}

void FOClient::Net_SendCommand( char* str )
{
    struct LogCB
    {
        static void Message( const char* str )
        {
            Self->AddMess( MSGBOX_GAME, str );
        }
    };
    PackCommand( str, Bout, LogCB::Message, Chosen->Name.c_str() );
}

void FOClient::Net_SendDir()
{
    if( !Chosen )
        return;

    Bout << NETMSG_DIR;
    Bout << (uint8)Chosen->GetDir();
}

void FOClient::Net_SendMove( UInt8Vec steps )
{
    if( !Chosen )
        return;

    uint move_params = 0;
    for( uint i = 0; i < MOVE_PARAM_STEP_COUNT; i++ )
    {
        if( i + 1 >= steps.size() )
            break;                           // Send next steps

        SETFLAG( move_params, steps[i + 1] << (i * MOVE_PARAM_STEP_BITS) );
        SETFLAG( move_params, MOVE_PARAM_STEP_ALLOW << (i * MOVE_PARAM_STEP_BITS) );
    }

    if( Chosen->IsRunning )
        SETFLAG( move_params, MOVE_PARAM_RUN );
    if( steps.empty() )
        SETFLAG( move_params, MOVE_PARAM_STEP_DISALLOW );                // Inform about stopping

    Bout << (Chosen->IsRunning ? NETMSG_SEND_MOVE_RUN : NETMSG_SEND_MOVE_WALK);
    Bout << move_params;
    Bout << Chosen->HexX;
    Bout << Chosen->HexY;
}

void FOClient::Net_SendUseSkill( uint16 skill, CritterCl* cr )
{
    Bout << NETMSG_SEND_USE_SKILL;
    Bout << skill;
    Bout << (uint8)TARGET_CRITTER;
    Bout << cr->GetId();
    Bout << (uint16)0;
}

void FOClient::Net_SendUseSkill( uint16 skill, ItemHex* item )
{
    Bout << NETMSG_SEND_USE_SKILL;
    Bout << skill;

    if( item->IsScenOrGrid() )
    {
        uint hex = (item->GetHexX() << 16) | item->GetHexY();

        Bout << (uint8)TARGET_SCENERY;
        Bout << hex;
        Bout << item->GetProtoId();
    }
    else
    {
        Bout << (uint8)TARGET_ITEM;
        Bout << item->GetId();
        Bout << (uint16)0;
    }
}

void FOClient::Net_SendUseSkill( uint16 skill, Item* item )
{
    Bout << NETMSG_SEND_USE_SKILL;
    Bout << skill;
    Bout << (uint8)TARGET_SELF_ITEM;
    Bout << item->GetId();
    Bout << (uint16)0;
}

void FOClient::Net_SendUseItem( uint8 ap, uint item_id, uint16 item_pid, uint8 rate, uint8 target_type, uint target_id, uint16 target_pid, uint param )
{
    Bout << NETMSG_SEND_USE_ITEM;
    Bout << ap;
    Bout << item_id;
    Bout << item_pid;
    Bout << rate;
    Bout << target_type;
    Bout << target_id;
    Bout << target_pid;
    Bout << param;
}

void FOClient::Net_SendPickItem( uint16 targ_x, uint16 targ_y, uint16 pid )
{
    Bout << NETMSG_SEND_PICK_ITEM;
    Bout << targ_x;
    Bout << targ_y;
    Bout << pid;
}

void FOClient::Net_SendPickCritter( uint crid, uint8 pick_type )
{
    Bout << NETMSG_SEND_PICK_CRITTER;
    Bout << crid;
    Bout << pick_type;
}

void FOClient::Net_SendChangeItem( uint8 ap, uint item_id, uint8 from_slot, uint8 to_slot, uint count )
{
    Bout << NETMSG_SEND_CHANGE_ITEM;
    Bout << ap;
    Bout << item_id;
    Bout << from_slot;
    Bout << to_slot;
    Bout << count;

    CollectContItems();
}

void FOClient::Net_SendItemCont( uint8 transfer_type, uint cont_id, uint item_id, uint count, uint8 take_flags )
{
    Bout << NETMSG_SEND_ITEM_CONT;
    Bout << transfer_type;
    Bout << cont_id;
    Bout << item_id;
    Bout << count;
    Bout << take_flags;

    CollectContItems();
}

void FOClient::Net_SendRateItem()
{
    if( !Chosen )
        return;
    uint rate = Chosen->ItemSlotMain->Data.Mode;
    if( !Chosen->ItemSlotMain->GetId() )
        rate |= Chosen->ItemSlotMain->GetProtoId() << 16;

    Bout << NETMSG_SEND_RATE_ITEM;
    Bout << rate;
}

void FOClient::Net_SendSortValueItem( Item* item )
{
    Bout << NETMSG_SEND_SORT_VALUE_ITEM;
    Bout << item->GetId();
    Bout << item->GetSortValue();

    CollectContItems();
}

void FOClient::Net_SendTalk( uint8 is_npc, uint id_to_talk, uint8 answer )
{
    Bout << NETMSG_SEND_TALK_NPC;
    Bout << is_npc;
    Bout << id_to_talk;
    Bout << answer;
}

void FOClient::Net_SendSayNpc( uint8 is_npc, uint id_to_talk, const char* str )
{
    Bout << NETMSG_SEND_SAY_NPC;
    Bout << is_npc;
    Bout << id_to_talk;
    Bout.Push( str, MAX_SAY_NPC_TEXT );
}

void FOClient::Net_SendBarter( uint npc_id, ItemVec& cont_sale, ItemVec& cont_buy )
{
    uint16 sale_count = (uint16)cont_sale.size();
    uint16 buy_count = (uint16)cont_buy.size();
    uint   msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(sale_count) + sizeof(buy_count) +
                     (sizeof(uint) + sizeof(uint) ) * sale_count + (sizeof(uint) + sizeof(uint) ) * buy_count;

    Bout << NETMSG_SEND_BARTER;
    Bout << msg_len;
    Bout << npc_id;

    Bout << sale_count;
    for( int i = 0; i < sale_count; ++i )
    {
        Bout << cont_sale[i].GetId();
        Bout << cont_sale[i].GetCount();
    }

    Bout << buy_count;
    for( int i = 0; i < buy_count; ++i )
    {
        Bout << cont_buy[i].GetId();
        Bout << cont_buy[i].GetCount();
    }
}

void FOClient::Net_SendGetGameInfo()
{
    Bout << NETMSG_SEND_GET_INFO;
}

void FOClient::Net_SendGiveMap( bool automap, uint16 map_pid, uint loc_id, uint tiles_hash, uint walls_hash, uint scen_hash )
{
    Bout << NETMSG_SEND_GIVE_MAP;
    Bout << automap;
    Bout << map_pid;
    Bout << loc_id;
    Bout << tiles_hash;
    Bout << walls_hash;
    Bout << scen_hash;
}

void FOClient::Net_SendLoadMapOk()
{
    Bout << NETMSG_SEND_LOAD_MAP_OK;
}

void FOClient::Net_SendGiveGlobalInfo( uint8 info_flags )
{
    Bout << NETMSG_SEND_GIVE_GLOBAL_INFO;
    Bout << info_flags;
}

void FOClient::Net_SendRuleGlobal( uint8 command, uint param1, uint param2 )
{
    Bout << NETMSG_SEND_RULE_GLOBAL;
    Bout << command;
    Bout << param1;
    Bout << param2;

    WaitPing();
}

void FOClient::Net_SendLevelUp( uint16 perk_up )
{
    if( !Chosen )
        return;

    uint16 count = 0;
    for( uint i = 0; i < SKILL_COUNT; i++ )
        if( ChaSkillUp[i] )
            count++;
    uint msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(count) + sizeof(uint16) * 2 * count + sizeof(perk_up);

    Bout << NETMSG_SEND_LEVELUP;
    Bout << msg_len;

    // Skills
    Bout << count;
    for( uint i = 0; i < SKILL_COUNT; i++ )
    {
        if( ChaSkillUp[i] )
        {
            uint16 skill_up = ChaSkillUp[i];
            if( Chosen->IsTagSkill( i + SKILL_BEGIN ) )
                skill_up /= 2;

            Bout << uint16( i + SKILL_BEGIN );
            Bout << skill_up;
        }
    }

    // Perks
    Bout << perk_up;
}

void FOClient::Net_SendCraftAsk( UIntVec numbers )
{
    uint16 count = (uint16)numbers.size();
    uint   msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(count) + sizeof(uint) * count;
    Bout << NETMSG_CRAFT_ASK;
    Bout << msg_len;
    Bout << count;
    for( int i = 0; i < count; i++ )
        Bout << numbers[i];
    FixNextShowCraftTick = Timer::FastTick() + CRAFT_SEND_TIME;
}

void FOClient::Net_SendCraft( uint craft_num )
{
    Bout << NETMSG_SEND_CRAFT;
    Bout << craft_num;
}

void FOClient::Net_SendPing( uint8 ping )
{
    Bout << NETMSG_PING;
    Bout << ping;
}

void FOClient::Net_SendPlayersBarter( uint8 barter, uint param, uint param_ext )
{
    Bout << NETMSG_PLAYERS_BARTER;
    Bout << barter;
    Bout << param;
    Bout << param_ext;
}

void FOClient::Net_SendScreenAnswer( uint answer_i, const char* answer_s )
{
    char answer_s_buf[MAX_SAY_NPC_TEXT + 1];
    memzero( answer_s_buf, sizeof(answer_s_buf) );
    Str::Copy( answer_s_buf, MAX_SAY_NPC_TEXT + 1, answer_s );

    Bout << NETMSG_SEND_SCREEN_ANSWER;
    Bout << answer_i;
    Bout.Push( answer_s_buf, MAX_SAY_NPC_TEXT );
}

void FOClient::Net_SendGetScores()
{
    Bout << NETMSG_SEND_GET_SCORES;
}

void FOClient::Net_SendSetUserHoloStr( Item* holodisk, const char* title, const char* text )
{
    if( !holodisk || !title || !text )
    {
        WriteLogF( _FUNC_, " - Null pointers arguments.\n" );
        return;
    }

    uint16 title_len = (uint16)Str::Length( title );
    uint16 text_len = (uint16)Str::Length( text );
    if( !title_len || !text_len || title_len > USER_HOLO_MAX_TITLE_LEN || text_len > USER_HOLO_MAX_LEN )
    {
        WriteLogF( _FUNC_, " - Length of texts is greather of maximum or zero, title cur<%u>, title max<%u>, text cur<%u>, text max<%u>.\n", title_len, USER_HOLO_MAX_TITLE_LEN, text_len, USER_HOLO_MAX_LEN );
        return;
    }

    uint msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(title_len) + sizeof(text_len) + title_len + text_len;
    Bout << NETMSG_SEND_SET_USER_HOLO_STR;
    Bout << msg_len;
    Bout << holodisk->GetId();
    Bout << title_len;
    Bout << text_len;
    Bout.Push( title, title_len );
    Bout.Push( text, text_len );
}

void FOClient::Net_SendGetUserHoloStr( uint str_num )
{
    static UIntSet already_send;
    if( already_send.count( str_num ) )
        return;
    already_send.insert( str_num );

    Bout << NETMSG_SEND_GET_USER_HOLO_STR;
    Bout << str_num;
}

void FOClient::Net_SendCombat( uint8 type, int val )
{
    Bout << NETMSG_SEND_COMBAT;
    Bout << type;
    Bout << val;
}

void FOClient::Net_SendRunScript( bool unsafe, const char* func_name, int p0, int p1, int p2, const char* p3, UIntVec& p4 )
{
    uint16 func_name_len = (uint16)Str::Length( func_name );
    uint16 p3len = (p3 ? (uint16)Str::Length( p3 ) : 0);
    uint16 p4size = (uint16)p4.size();
    uint   msg_len = sizeof(uint) + sizeof(msg_len) + sizeof(unsafe) + sizeof(func_name_len) + func_name_len +
                     sizeof(p0) + sizeof(p1) + sizeof(p2) + sizeof(p3len) + p3len + sizeof(p4size) + p4size * sizeof(uint);

    Bout << NETMSG_SEND_RUN_SERVER_SCRIPT;
    Bout << msg_len;
    Bout << unsafe;
    Bout << func_name_len;
    Bout.Push( func_name, func_name_len );
    Bout << p0;
    Bout << p1;
    Bout << p2;
    Bout << p3len;
    if( p3len )
        Bout.Push( p3, p3len );
    Bout << p4size;
    if( p4size )
        Bout.Push( (char*)&p4[0], p4size * sizeof(uint) );
}

void FOClient::Net_SendKarmaVoting( uint crid, bool val_up )
{
    Bout << NETMSG_SEND_KARMA_VOTING;
    Bout << crid;
    Bout << val_up;
}

void FOClient::Net_SendRefereshMe()
{
    Bout << NETMSG_SEND_REFRESH_ME;

    WaitPing();
}

void FOClient::Net_OnLoginSuccess()
{
    if( !Singleplayer )
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_LOGINOK ) );
    WriteLog( "Authentication success.\n" );

    GmapFreeResources();
    ResMngr.FreeResources( RES_ITEMS );
    ResMngr.FreeResources( RES_CRITTERS );

    // Server bin/bout == client bout/bin
    uint bin_seed, bout_seed;
    Bin >> bin_seed;
    Bin >> bout_seed;
    Bout.SetEncryptKey( bin_seed + NETSALT_BIN );
    Bin.SetEncryptKey( bout_seed + NETSALT_BOUT );
}

void FOClient::Net_OnAddCritter( bool is_npc )
{
    uint msg_len;
    Bin >> msg_len;

    uint crid;
    uint base_type;
    Bin >> crid;
    Bin >> base_type;

    uint16 hx, hy;
    uint8  dir;
    Bin >> hx;
    Bin >> hy;
    Bin >> dir;

    uint8 cond;
    uint  anim1life, anim1ko, anim1dead;
    uint  anim2life, anim2ko, anim2dead;
    uint  flags;
    short multihex;
    Bin >> cond;
    Bin >> anim1life;
    Bin >> anim1ko;
    Bin >> anim1dead;
    Bin >> anim2life;
    Bin >> anim2ko;
    Bin >> anim2dead;
    Bin >> flags;
    Bin >> multihex;

    // Npc
    uint16 npc_pid;
    uint   npc_dialog_id;
    if( is_npc )
    {
        Bin >> npc_pid;
        Bin >> npc_dialog_id;
    }

    // Player
    char cl_name[UTF8_BUF_SIZE( MAX_NAME )];
    if( !is_npc )
    {
        Bin.Pop( cl_name, sizeof(cl_name) );
        cl_name[sizeof(cl_name) - 1] = 0;
    }

    // Parameters
    int    params[MAX_PARAMS];
    memzero( params, sizeof(params) );
    uint16 count, index;
    Bin >> count;
    for( uint i = 0, j = min( count, uint16( MAX_PARAMS ) ); i < j; i++ )
    {
        Bin >> index;
        Bin >> params[index < MAX_PARAMS ? index : 0];
    }

    CHECK_IN_BUFF_ERROR;

    if( !CritType::IsEnabled( base_type ) )
        base_type = DEFAULT_CRTYPE;

    if( !crid )
        WriteLogF( _FUNC_, " - CritterCl id is zero.\n" );
    else if( HexMngr.IsMapLoaded() && (hx >= HexMngr.GetMaxHexX() || hy >= HexMngr.GetMaxHexY() || dir >= DIRS_COUNT) )
        WriteLogF( _FUNC_, " - Invalid positions hx<%u>, hy<%u>, dir<%u>.\n", hx, hy, dir );
    else
    {
        CritterCl* cr = new CritterCl();
        cr->Id = crid;
        cr->HexX = hx;
        cr->HexY = hy;
        cr->CrDir = dir;
        cr->Cond = cond;
        cr->Anim1Life = anim1life;
        cr->Anim1Knockout = anim1ko;
        cr->Anim1Dead = anim1dead;
        cr->Anim2Life = anim2life;
        cr->Anim2Knockout = anim2ko;
        cr->Anim2Dead = anim2dead;
        cr->Flags = flags;
        cr->Multihex = multihex;
        memcpy( cr->Params, params, sizeof(params) );

        if( is_npc )
        {
            cr->Pid = npc_pid;
            cr->Params[ST_DIALOG_ID] = npc_dialog_id;
            cr->Name = MsgDlg->GetStr( STR_NPC_NAME_( cr->Params[ST_DIALOG_ID], cr->Pid ) );
            if( MsgDlg->Count( STR_NPC_AVATAR_( cr->Params[ST_DIALOG_ID] ) ) )
                cr->Avatar = MsgDlg->GetStr( STR_NPC_AVATAR_( cr->Params[ST_DIALOG_ID] ) );
        }
        else
        {
            cr->Name = cl_name;
        }

        cr->SetBaseType( base_type );
        cr->Init();

        if( FLAG( cr->Flags, CRITTER_FLAG_CHOSEN ) )
            SetAction( CHOSEN_NONE );

        AddCritter( cr );

        if( cr->IsChosen() && !IsMainScreen( CLIENT_MAIN_SCREEN_WORLDMAP ) )
        {
            MoveDirs.clear();
            cr->MoveSteps.clear();
            HexMngr.FindSetCenter( cr->HexX, cr->HexY );
            cr->AnimateStay();
            SndMngr.PlayAmbient( MsgGM->GetStr( STR_MAP_AMBIENT_( HexMngr.GetCurPidMap() ) ) );
            ShowMainScreen( CLIENT_MAIN_SCREEN_GAME );
            ScreenFadeOut();
            HexMngr.RebuildLight();
        }

        const char* look = FmtCritLook( cr, CRITTER_LOOK_NAME );
        if( look )
            cr->Name = look;
        if( Script::PrepareContext( ClientFunctions.CritterIn, _FUNC_, "Game" ) )
        {
            Script::SetArgObject( cr );
            Script::RunPrepared();
        }

        for( int i = 0; i < MAX_PARAMS; i++ )
            cr->ChangeParam( i );
        cr->ProcessChangedParams();
    }
}

void FOClient::Net_OnRemoveCritter()
{
    uint       remid;
    Bin >> remid;
    CritterCl* cr = GetCritter( remid );
    if( cr )
        cr->Finish();

    if( Script::PrepareContext( ClientFunctions.CritterOut, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( cr );
        Script::RunPrepared();
    }
}

void FOClient::Net_OnText()
{
    uint   msg_len;
    uint   crid;
    uint8  how_say;
    uint16 intellect;
    bool   unsafe_text;
    uint16 len;
    char   str[MAX_FOTEXT + 1];

    Bin >> msg_len;
    Bin >> crid;
    Bin >> how_say;
    Bin >> intellect;
    Bin >> unsafe_text;

    Bin >> len;
    Bin.Pop( str, min( len, uint16( MAX_FOTEXT ) ) );
    if( len > MAX_FOTEXT )
        Bin.Pop( Str::GetBigBuf(), len - MAX_FOTEXT );
    str[min( len, uint16( MAX_FOTEXT ) )] = 0;

    CHECK_IN_BUFF_ERROR;

    if( how_say == SAY_FLASH_WINDOW )
    {
        FlashGameWindow();
        return;
    }

    if( unsafe_text )
        Keyb::EraseInvalidChars( str, KIF_NO_SPEC_SYMBOLS );

    OnText( str, crid, how_say, intellect );
}

void FOClient::Net_OnTextMsg( bool with_lexems )
{
    uint   msg_len;
    uint   crid;
    uint8  how_say;
    uint16 msg_num;
    uint   num_str;

    if( with_lexems )
        Bin >> msg_len;
    Bin >> crid;
    Bin >> how_say;
    Bin >> msg_num;
    Bin >> num_str;

    char lexems[MAX_DLG_LEXEMS_TEXT + 1] = { 0 };
    if( with_lexems )
    {
        uint16 lexems_len;
        Bin >> lexems_len;
        if( lexems_len && lexems_len <= MAX_DLG_LEXEMS_TEXT )
        {
            Bin.Pop( lexems, lexems_len );
            lexems[lexems_len] = '\0';
        }
    }

    CHECK_IN_BUFF_ERROR;

    if( how_say == SAY_FLASH_WINDOW )
    {
        FlashGameWindow();
        return;
    }

    if( msg_num >= TEXTMSG_MAX )
    {
        WriteLogF( _FUNC_, " - Msg num invalid value<%u>.\n", msg_num );
        return;
    }

    FOMsg& msg = CurLang.Msg[msg_num];
    if( msg_num == TEXTMSG_HOLO )
    {
        char str[MAX_FOTEXT];
        Str::Copy( str, GetHoloText( num_str ) );
        OnText( str, crid, how_say, 10 );
    }
    else if( msg.Count( num_str ) )
    {
        char str[MAX_FOTEXT];
        Str::Copy( str, msg.GetStr( num_str ) );
        FormatTags( str, MAX_FOTEXT, Chosen, GetCritter( crid ), lexems );
        OnText( str, crid, how_say, 10 );
    }
}

void FOClient::OnText( const char* str, uint crid, int how_say, uint16 intellect )
{
    Str::Copy( fstr, str );
    uint len = Str::Length( str );
    if( !len )
        return;

    uint text_delay = GameOpt.TextDelay + len * 100;
    if( Script::PrepareContext( ClientFunctions.InMessage, _FUNC_, "Game" ) )
    {
        ScriptString* sstr = new ScriptString( fstr );
        Script::SetArgObject( sstr );
        Script::SetArgAddress( &how_say );
        Script::SetArgAddress( &crid );
        Script::SetArgAddress( &text_delay );
        Script::RunPrepared();
        Str::Copy( fstr, sstr->c_str() );
        sstr->Release();

        if( !Script::GetReturnedBool() )
            return;

        len = Str::Length( fstr );
        if( !len )
            return;
    }

    // Intellect format
    if( how_say >= SAY_NORM && how_say <= SAY_RADIO )
        FmtTextIntellect( fstr, intellect );

    // Type stream
    uint fstr_cr = 0;
    uint fstr_mb = 0;
    int  mess_type = MSGBOX_TALK;

    switch( how_say )
    {
        case SAY_NORM:
            fstr_mb = STR_MBNORM;
        case SAY_NORM_ON_HEAD:
            fstr_cr = STR_CRNORM;
            break;
        case SAY_SHOUT:
            fstr_mb = STR_MBSHOUT;
        case SAY_SHOUT_ON_HEAD:
            fstr_cr = STR_CRSHOUT;
            Str::UpperUTF8( fstr );
            break;
        case SAY_EMOTE:
            fstr_mb = STR_MBEMOTE;
        case SAY_EMOTE_ON_HEAD:
            fstr_cr = STR_CREMOTE;
            break;
        case SAY_WHISP:
            fstr_mb = STR_MBWHISP;
        case SAY_WHISP_ON_HEAD:
            fstr_cr = STR_CRWHISP;
            Str::LowerUTF8( fstr );
            break;
        case SAY_SOCIAL:
            fstr_cr = STR_CRSOCIAL;
            fstr_mb = STR_MBSOCIAL;
            break;
		case SAY_FACTION:
			mess_type = MSGBOX_SOCIAL;
			fstr_mb = STR_MBFACTION;
			break;
		case SAY_SQUAD:
			mess_type = MSGBOX_SOCIAL;
			fstr_mb = STR_MBSQUAD;
			break;
		case SAY_RADIO:
            fstr_mb = STR_MBRADIO;
            break;
		case SAY_RADIO_0:
			fstr_mb = STR_MBRADIO + 1;
			break;
		case SAY_RADIO_2:
			fstr_mb = STR_MBRADIO + 1;
			break;
		case SAY_NETMSG:
            mess_type = MSGBOX_GAME;
            fstr_mb = STR_MBNET;
            break;
        default:
            break;
    }

    CritterCl* cr = NULL;
    if( how_say != SAY_RADIO )
        cr = GetCritter( crid );

    string crit_name = (cr ? cr->GetName() : "?");

    // CritterCl on head text
    if( fstr_cr && cr )
        cr->SetText( FmtGameText( fstr_cr, fstr ), COLOR_TEXT, text_delay );

    // Message box text
    if( fstr_mb )
    {
		if (how_say == SAY_FACTION) {
			AddMess(mess_type, FmtGameText(fstr_mb, fstr));
		}
		else if (how_say == SAY_SQUAD)
		{
			AddMess(mess_type, FmtGameText(fstr_mb, fstr));
		}
		else if( how_say == SAY_NETMSG )
        {
            AddMess( mess_type, FmtGameText( fstr_mb, fstr ) );
        }
        else if( how_say == SAY_RADIO )
        {
            uint16 channel = 0;
            if( Chosen )
            {
                Item* radio = Chosen->GetItem( crid );
                if( radio )
                    channel = radio->Data.RadioChannel;
            }
			if (channel % 2 == 0) {
				string str = fstr;
				std::size_t pos = str.find(":");
				if (pos > 0) {
					string nameStr = str.substr(0, pos);
					string messageStr = str.substr(pos + 1);
					AddMess(MSGBOX_RADIO, FmtGameText(fstr_mb + 1, channel, nameStr.c_str(), messageStr.c_str()));
				}
				else {
					WriteLog("Radio 0 bad format, missing name or separator ':'\n");
				}
			} else {
				AddMess(MSGBOX_RADIO, FmtGameText(fstr_mb, channel, fstr));
			}
		}
		else if (how_say == SAY_RADIO_0) {
			string str = fstr;
			std::size_t pos = str.find(":");
			if (pos > 0) {
				string nameStr = str.substr(0, pos);
				string messageStr = str.substr(pos + 1);
				int radioChannel = 0;
				AddMess(MSGBOX_RADIO, FmtGameText(STR_MBRADIO + 1, radioChannel, nameStr.c_str(), messageStr.c_str()));
			}
			else {
				WriteLog("Radio 0 bad format, missing name or separator ':'\n");
			}
		}
		else if (how_say == SAY_RADIO_2) {
			string str = fstr;
			std::size_t pos = str.find(":");
			if (pos > 0) {
				string nameStr = str.substr(0, pos);
				string messageStr = str.substr(pos + 1);
				int radioChannel = 2;
				AddMess(MSGBOX_RADIO_HIDDEN, FmtGameText(STR_MBRADIO + 1, radioChannel, nameStr.c_str(), messageStr.c_str()));
			}
			else {
				WriteLog("Radio 2 bad format, missing name or separator ':'\n");
			}
		}
		else
        {
            AddMess(mess_type, FmtGameText( fstr_mb, crit_name.c_str(), fstr ) );
        }

        if( IsScreenPlayersBarter() && Chosen && (crid == BarterOpponentId || crid == Chosen->GetId() ) )
        {
            if( how_say == SAY_NETMSG || how_say == SAY_RADIO )
                BarterText += FmtGameText( fstr_mb, fstr );
            else
                BarterText += FmtGameText( fstr_mb, crit_name.c_str(), fstr );
            BarterText += "\n";
            BarterText += Str::FormatBuf( "|%u ", COLOR_TEXT );
            DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, BarterText.c_str(), dialogFont );
        }
    }

    // Dialog text
    bool is_dialog = IsScreenPresent( CLIENT_SCREEN_DIALOG );
    bool is_barter = IsScreenPresent( CLIENT_SCREEN_BARTER );
    if( (how_say == SAY_DIALOG || how_say == SAY_APPEND) && (is_dialog || is_barter) )
    {
        CritterCl* npc = GetCritter( DlgNpcId );
        FormatTags( fstr, MAX_FOTEXT, Chosen, npc, "" );

        if( is_dialog )
        {
            if( how_say == SAY_APPEND )
                DlgMainText += fstr;
            else
                DlgMainText = fstr;
            DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, DlgMainText.c_str(), dialogFont );
        }
        else if( is_barter )
        {
            if( how_say == SAY_APPEND )
                BarterText += fstr;
            else
                BarterText = fstr;
            DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, BarterText.c_str(), dialogFont);
        }
        if( how_say != SAY_APPEND )
            DlgMainTextCur = 0;
    }

    // Encounter question
    if( how_say >= SAY_ENCOUNTER_ANY && how_say <= SAY_ENCOUNTER_TB && IsMainScreen( CLIENT_MAIN_SCREEN_WORLDMAP ) )
    {
        ShowScreen( CLIENT_SCREEN_DIALOGBOX );
        if( how_say == SAY_ENCOUNTER_ANY )
        {
            DlgboxType = DIALOGBOX_ENCOUNTER_ANY;
            DlgboxButtonText[0] = MsgGame->GetStr( STR_DIALOGBOX_ENCOUNTER_RT );
            DlgboxButtonText[1] = MsgGame->GetStr( STR_DIALOGBOX_ENCOUNTER_TB );
            DlgboxButtonText[2] = MsgGame->GetStr( STR_DIALOGBOX_CANCEL );
            DlgboxButtonsCount = 3;
        }
        else if( how_say == SAY_ENCOUNTER_RT )
        {
            DlgboxType = DIALOGBOX_ENCOUNTER_RT;
            DlgboxButtonText[0] = MsgGame->GetStr( STR_DIALOGBOX_ENCOUNTER_RT );
            DlgboxButtonText[1] = MsgGame->GetStr( STR_DIALOGBOX_CANCEL );
            DlgboxButtonsCount = 2;
        }
        else
        {
            DlgboxType = DIALOGBOX_ENCOUNTER_TB;
            DlgboxButtonText[0] = MsgGame->GetStr( STR_DIALOGBOX_ENCOUNTER_TB );
            DlgboxButtonText[1] = MsgGame->GetStr( STR_DIALOGBOX_CANCEL );
            DlgboxButtonsCount = 2;
        }
        DlgboxWait = Timer::GameTick() + GM_ANSWER_WAIT_TIME;
        Str::Copy( DlgboxText, fstr );
    }

    // FixBoy result
    if( how_say == SAY_FIX_RESULT )
        FixResultStr = fstr;

    // Dialogbox
    if( how_say == SAY_DIALOGBOX_TEXT )
        Str::Copy( DlgboxText, fstr );
    else if( how_say >= SAY_DIALOGBOX_BUTTON( 0 ) && how_say < SAY_DIALOGBOX_BUTTON( MAX_DLGBOX_BUTTONS ) )
        DlgboxButtonText[how_say - SAY_DIALOGBOX_BUTTON( 0 )] = fstr;

    // Say box
    if( how_say == SAY_SAY_TITLE )
        SayTitle = fstr;
    else if( how_say == SAY_SAY_TEXT )
        SayText = fstr;

    FlashGameWindow();
}

void FOClient::OnMapText( const char* str, uint16 hx, uint16 hy, uint color, uint16 intellect )
{
    uint          len = Str::Length( str );
    uint          text_delay = GameOpt.TextDelay + len * 100;
    ScriptString* sstr = new ScriptString( str );
    if( Script::PrepareContext( ClientFunctions.MapMessage, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( sstr );
        Script::SetArgAddress( &hx );
        Script::SetArgAddress( &hy );
        Script::SetArgAddress( &color );
        Script::SetArgAddress( &text_delay );
        Script::RunPrepared();
        if( !Script::GetReturnedBool() )
        {
            sstr->Release();
            return;
        }
    }

    char fstr[MAX_FOTEXT];
    Str::Copy( fstr, sstr->c_str() );
    sstr->Release();

    FmtTextIntellect( fstr, intellect );

    MapText t;
    t.HexX = hx;
    t.HexY = hy;
    t.Color = (color ? color : COLOR_TEXT);
    t.Fade = false;
    t.StartTick = Timer::GameTick();
    t.Tick = text_delay;
    t.Text = fstr;
    t.Pos = HexMngr.GetRectForText( hx, hy );
    t.EndPos = t.Pos;
    auto it = std::find( GameMapTexts.begin(), GameMapTexts.end(), t );
    if( it != GameMapTexts.end() )
        GameMapTexts.erase( it );
    GameMapTexts.push_back( t );

    FlashGameWindow();
}

void FOClient::Net_OnMapText()
{
    uint   msg_len;
    uint16 hx, hy;
    uint   color;
    uint16 len;
    char   str[MAX_FOTEXT + 1];
    uint16 intellect;
    bool   unsafe_text;

    Bin >> msg_len;
    Bin >> hx;
    Bin >> hy;
    Bin >> color;

    Bin >> len;
    Bin.Pop( str, min( len, uint16( MAX_FOTEXT ) ) );
    if( len > MAX_FOTEXT )
        Bin.Pop( Str::GetBigBuf(), len - MAX_FOTEXT );
    str[min( len, uint16( MAX_FOTEXT ) )] = 0;

    Bin >> intellect;
    Bin >> unsafe_text;

    CHECK_IN_BUFF_ERROR;

    if( hx >= HexMngr.GetMaxHexX() || hy >= HexMngr.GetMaxHexY() )
    {
        WriteLogF( _FUNC_, " - Invalid coords, hx<%u>, hy<%u>, text<%s>.\n", hx, hy, str );
        return;
    }

    if( unsafe_text )
        Keyb::EraseInvalidChars( str, KIF_NO_SPEC_SYMBOLS );

    OnMapText( str, hx, hy, color, intellect );
}

void FOClient::Net_OnMapTextMsg()
{
    uint16 hx, hy;
    uint   color;
    uint16 msg_num;
    uint   num_str;

    Bin >> hx;
    Bin >> hy;
    Bin >> color;
    Bin >> msg_num;
    Bin >> num_str;

    if( msg_num >= TEXTMSG_MAX )
    {
        WriteLogF( _FUNC_, " - Msg num invalid value, num<%u>.\n", msg_num );
        return;
    }

    static char str[MAX_FOTEXT];
    Str::Copy( str, CurLang.Msg[msg_num].GetStr( num_str ) );
    FormatTags( str, MAX_FOTEXT, Chosen, NULL, "" );

    OnMapText( str, hx, hy, color, 0 );
}

void FOClient::Net_OnMapTextMsgLex()
{
    uint   msg_len;
    uint16 hx, hy;
    uint   color;
    uint16 msg_num;
    uint   num_str;
    uint16 lexems_len;

    Bin >> msg_len;
    Bin >> hx;
    Bin >> hy;
    Bin >> color;
    Bin >> msg_num;
    Bin >> num_str;
    Bin >> lexems_len;

    char lexems[MAX_DLG_LEXEMS_TEXT + 1] = { 0 };
    if( lexems_len && lexems_len <= MAX_DLG_LEXEMS_TEXT )
    {
        Bin.Pop( lexems, lexems_len );
        lexems[lexems_len] = '\0';
    }

    CHECK_IN_BUFF_ERROR;

    if( msg_num >= TEXTMSG_MAX )
    {
        WriteLogF( _FUNC_, " - Msg num invalid value, num<%u>.\n", msg_num );
        return;
    }

    char str[MAX_FOTEXT];
    Str::Copy( str, CurLang.Msg[msg_num].GetStr( num_str ) );
    FormatTags( str, MAX_FOTEXT, Chosen, NULL, lexems );

    OnMapText( str, hx, hy, color, 0 );
}

void FOClient::Net_OnCritterDir()
{
    uint  crid;
    uint8 dir;
    Bin >> crid;
    Bin >> dir;

    if( dir >= DIRS_COUNT )
    {
        WriteLogF( _FUNC_, " - Invalid dir<%u>.\n", dir );
        dir = 0;
    }

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;
    cr->SetDir( dir );
}

void FOClient::Net_OnCritterMove()
{
    uint   crid;
    uint   move_params;
    uint16 new_hx;
    uint16 new_hy;
    Bin >> crid;
    Bin >> move_params;
    Bin >> new_hx;
    Bin >> new_hy;

    if( new_hx >= HexMngr.GetMaxHexX() || new_hy >= HexMngr.GetMaxHexY() )
        return;

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    uint16 last_hx = cr->GetHexX();
    uint16 last_hy = cr->GetHexY();
    cr->IsRunning = FLAG( move_params, MOVE_PARAM_RUN );

    if( cr != Chosen )
    {
        cr->MoveSteps.resize( cr->CurMoveStep > 0 ? cr->CurMoveStep : 0 );
        if( cr->CurMoveStep >= 0 )
            cr->MoveSteps.push_back( PAIR( new_hx, new_hy ) );
        for( int i = 0, j = cr->CurMoveStep + 1; i < MOVE_PARAM_STEP_COUNT; i++, j++ )
        {
            int  step = (move_params >> (i * MOVE_PARAM_STEP_BITS) ) & (MOVE_PARAM_STEP_DIR | MOVE_PARAM_STEP_ALLOW | MOVE_PARAM_STEP_DISALLOW);
            int  dir = step & MOVE_PARAM_STEP_DIR;
            bool disallow = (!FLAG( step, MOVE_PARAM_STEP_ALLOW ) || FLAG( step, MOVE_PARAM_STEP_DISALLOW ) );
            if( disallow )
            {
                if( j <= 0 && (cr->GetHexX() != new_hx || cr->GetHexY() != new_hy) )
                {
                    HexMngr.TransitCritter( cr, new_hx, new_hy, true, true );
                    cr->CurMoveStep = -1;
                }
                break;
            }
            MoveHexByDir( new_hx, new_hy, dir, HexMngr.GetMaxHexX(), HexMngr.GetMaxHexY() );
            if( j < 0 )
                continue;
            cr->MoveSteps.push_back( PAIR( new_hx, new_hy ) );
        }
        cr->CurMoveStep++;

        /*if(HexMngr.IsShowTrack())
           {
                HexMngr.ClearHexTrack();
                for(int i=4;i<cr->MoveDirs.size();i++)
                {
                        UInt16Pair& step=cr->MoveDirs[i];
                        HexMngr.HexTrack[step.second][step.first]=1;
                }
                HexMngr.HexTrack[last_hy][last_hx]=2;
                HexMngr.RefreshMap();
           }*/
    }
    else
    {
        if( IsAction( CRITTER_ACTION_MOVE ) )
            EraseFrontAction();
        MoveDirs.clear();
        HexMngr.TransitCritter( cr, new_hx, new_hy, true, true );
    }
}

void FOClient::Net_OnSomeItem()
{
    uint           item_id;
    uint16         item_pid;
    uint8          slot;
    Item::ItemData data;
    Bin >> item_id;
    Bin >> item_pid;
    Bin >> slot;
    Bin.Pop( (char*)&data, sizeof(data) );

    ProtoItem* proto_item = ItemMngr.GetProtoItem( item_pid );
    if( !proto_item )
    {
        WriteLogF( _FUNC_, " - Proto item<%u> not found.\n", item_pid );
        return;
    }

    SomeItem.Id = item_id;
    SomeItem.AccCritter.Slot = slot;
    SomeItem.Init( proto_item );
    SomeItem.Data = data;
}

void FOClient::Net_OnCritterAction()
{
    uint crid;
    int  action;
    int  action_ext;
    bool is_item;
    Bin >> crid;
    Bin >> action;
    Bin >> action_ext;
    Bin >> is_item;

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;
    cr->Action( action, action_ext, is_item ? &SomeItem : NULL, false );
}

void FOClient::Net_OnCritterKnockout()
{
    uint   crid;
    uint   anim2begin;
    uint   anim2idle;
    uint16 knock_hx;
    uint16 knock_hy;
    Bin >> crid;
    Bin >> anim2begin;
    Bin >> anim2idle;
    Bin >> knock_hx;
    Bin >> knock_hy;

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    cr->Action( CRITTER_ACTION_KNOCKOUT, anim2begin, NULL, false );
    cr->Anim2Knockout = anim2idle;

    if( cr->GetHexX() != knock_hx || cr->GetHexY() != knock_hy )
    {
        // TODO: offsets
        HexMngr.TransitCritter( cr, knock_hx, knock_hy, false, true );
    }
}

void FOClient::Net_OnCritterMoveItem()
{
    uint  msg_len;
    uint  crid;
    uint8 action;
    uint8 prev_slot;
    bool  is_item;
    Bin >> msg_len;
    Bin >> crid;
    Bin >> action;
    Bin >> prev_slot;
    Bin >> is_item;

    // Slot items
    UInt8Vec               slots_data_slot;
    UIntVec                slots_data_id;
    UInt16Vec              slots_data_pid;
    vector<Item::ItemData> slots_data_data;
    uint8                  slots_data_count;
    Bin >> slots_data_count;
    for( uint8 i = 0; i < slots_data_count; i++ )
    {
        uint8          slot;
        uint           id;
        uint16         pid;
        Item::ItemData data;
        Bin >> slot;
        Bin >> id;
        Bin >> pid;
        Bin.Pop( (char*)&data, sizeof(data) );
        slots_data_slot.push_back( slot );
        slots_data_id.push_back( id );
        slots_data_pid.push_back( pid );
        slots_data_data.push_back( data );
    }

    CHECK_IN_BUFF_ERROR;

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    if( cr != Chosen )
    {
        int64 prev_hash_sum = 0;
        for( auto it = cr->InvItems.begin(), end = cr->InvItems.end(); it != end; ++it )
        {
            Item* item = *it;
            prev_hash_sum += item->LightGetHash();
        }

        cr->EraseAllItems();
        cr->DefItemSlotHand->Init( ItemMngr.GetProtoItem( ITEM_DEF_SLOT ) );
        cr->DefItemSlotArmor->Init( ItemMngr.GetProtoItem( ITEM_DEF_ARMOR ) );

        for( uint8 i = 0; i < slots_data_count; i++ )
        {
            ProtoItem* proto_item = ItemMngr.GetProtoItem( slots_data_pid[i] );
            if( proto_item )
            {
                Item* item = new Item();
                item->Id = slots_data_id[i];
                item->Init( proto_item );
                item->Data = slots_data_data[i];
                item->AccCritter.Slot = slots_data_slot[i];
                cr->AddItem( item );
            }
        }

        int64 hash_sum = 0;
        for( auto it = cr->InvItems.begin(), end = cr->InvItems.end(); it != end; ++it )
        {
            Item* item = *it;
            hash_sum += item->LightGetHash();
        }
        if( hash_sum != prev_hash_sum )
            HexMngr.RebuildLight();
    }

    cr->Action( action, prev_slot, is_item ? &SomeItem : NULL, false );
}

void FOClient::Net_OnCritterItemData()
{
    uint           crid;
    uint8          slot;
    Item::ItemData data;
    memzero( &data, sizeof(data) );
    Bin >> crid;
    Bin >> slot;
    Bin.Pop( (char*)&data, sizeof(data) );

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    Item* item = cr->GetItemSlot( slot );
    if( item )
    {
        uint light_hash = item->LightGetHash();
        item->Data = data;
        if( item->LightGetHash() != light_hash )
            HexMngr.RebuildLight();
    }
}

void FOClient::Net_OnCritterAnimate()
{
    uint crid;
    uint anim1;
    uint anim2;
    bool is_item;
    bool clear_sequence;
    bool delay_play;
    Bin >> crid;
    Bin >> anim1;
    Bin >> anim2;
    Bin >> is_item;
    Bin >> clear_sequence;
    Bin >> delay_play;

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    if( clear_sequence )
        cr->ClearAnim();
    if( delay_play || !cr->IsAnim() )
        cr->Animate( anim1, anim2, is_item ? &SomeItem : NULL );
}

void FOClient::Net_OnCritterSetAnims()
{
    uint crid;
    int  cond;
    uint anim1;
    uint anim2;
    Bin >> crid;
    Bin >> cond;
    Bin >> anim1;
    Bin >> anim2;

    CritterCl* cr = GetCritter( crid );
    if( cr )
    {
        if( cond == 0 || cond == CRITTER_CONDITION_LIFE )
        {
            cr->Anim1Life = anim1;
            cr->Anim2Life = anim2;
        }
        else if( cond == 0 || cond == CRITTER_CONDITION_KNOCKOUT )
        {
            cr->Anim1Knockout = anim1;
            cr->Anim2Knockout = anim2;
        }
        else if( cond == 0 || cond == CRITTER_CONDITION_DEAD )
        {
            cr->Anim1Dead = anim1;
            cr->Anim2Dead = anim2;
        }
        if( !cr->IsAnim() )
            cr->AnimateStay();
    }
}

void FOClient::Net_OnCheckUID0()
{
    #define CHECKUIDBIN                       \
        uint uid[5];                          \
        uint  uidxor[5];                      \
        uint8 rnd_count, rnd_count2;          \
        uint8 dummy;                          \
        uint  msg_len;                        \
        Bin >> msg_len;                       \
        Bin >> uid[3];                        \
        Bin >> uidxor[0];                     \
        Bin >> rnd_count;                     \
        Bin >> uid[1];                        \
        Bin >> uidxor[2];                     \
        for( int i = 0; i < rnd_count; i++ )  \
            Bin >> dummy;                     \
        Bin >> uid[2];                        \
        Bin >> uidxor[1];                     \
        Bin >> uid[4];                        \
        Bin >> rnd_count2;                    \
        Bin >> uidxor[3];                     \
        Bin >> uidxor[4];                     \
        Bin >> uid[0];                        \
        for( int i = 0; i < 5; i++ )          \
            uid[i] ^= uidxor[i];              \
        for( int i = 0; i < rnd_count2; i++ ) \
            Bin >> dummy;                     \
        CHECK_IN_BUFF_ERROR

        CHECKUIDBIN;
    if( CHECK_UID0( uid ) )
        UIDFail = true;
}

void FOClient::Net_OnCritterParam()
{
    uint   crid;
    uint16 index;
    int    value;
    Bin >> crid;
    Bin >> index;
    Bin >> value;
    if( GameOpt.DebugNet )
        AddMess( MSGBOX_GAME, Str::FormatBuf( " - crid<%u> index<%u> value<%d>.", crid, index, value ) );

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    if( index < MAX_PARAMS )
    {
        cr->ChangeParam( index );
        cr->Params[index] = value;
        cr->ProcessChangedParams();
    }

    if( index >= ST_ANIM3D_LAYER_BEGIN && index <= ST_ANIM3D_LAYER_END )
    {
        if( !cr->IsAnim() )
            cr->Action( CRITTER_ACTION_REFRESH, 0, NULL, false );
    }
    else if( index == OTHER_FLAGS )
    {
        cr->Flags = value;
    }
    else if( index == OTHER_BASE_TYPE )
    {
        if( cr->Multihex < 0 && CritType::GetMultihex( cr->GetCrType() ) != CritType::GetMultihex( value ) )
        {
            HexMngr.SetMultihex( cr->GetHexX(), cr->GetHexY(), CritType::GetMultihex( cr->GetCrType() ), false );
            HexMngr.SetMultihex( cr->GetHexX(), cr->GetHexY(), CritType::GetMultihex( value ), true );
        }

        cr->SetBaseType( value );
        if( !cr->IsAnim() )
            cr->Action( CRITTER_ACTION_REFRESH, 0, NULL, false );
    }
    else if( index == OTHER_MULTIHEX )
    {
        int old_mh = cr->GetMultihex();
        cr->Multihex = value;
        if( old_mh != (int)cr->GetMultihex() )
        {
            HexMngr.SetMultihex( cr->GetHexX(), cr->GetHexY(), old_mh, false );
            HexMngr.SetMultihex( cr->GetHexX(), cr->GetHexY(), cr->GetMultihex(), true );
        }
    }
    else if( index == OTHER_YOU_TURN )
    {
        TurnBasedTime = Timer::GameTick() + value;
        TurnBasedCurCritterId = cr->GetId();
        HexMngr.SetCritterContour( cr->GetId(), CONTOUR_CUSTOM );
    }
}

void FOClient::Net_OnCheckUID1()
{
    CHECKUIDBIN;
    if( CHECK_UID1( uid ) )
        ExitProcess( 0 );
}

void FOClient::Net_OnCritterXY()
{
    uint   crid;
    uint16 hx;
    uint16 hy;
    uint8  dir;
    Bin >> crid;
    Bin >> hx;
    Bin >> hy;
    Bin >> dir;
    if( GameOpt.DebugNet )
        AddMess( MSGBOX_GAME, Str::FormatBuf( " - crid<%u> hx<%u> hy<%u> dir<%u>.", crid, hx, hy, dir ) );

    if( !HexMngr.IsMapLoaded() )
        return;

    CritterCl* cr = GetCritter( crid );
    if( !cr )
        return;

    if( hx >= HexMngr.GetMaxHexX() || hy >= HexMngr.GetMaxHexY() || dir >= DIRS_COUNT )
    {
        WriteLogF( _FUNC_, " - Error data, hx<%d>, hy<%d>, dir<%d>.\n", hx, hy, dir );
        return;
    }

    if( cr->GetDir() != dir )
        cr->SetDir( dir );

    if( cr->GetHexX() != hx || cr->GetHexY() != hy )
    {
        Field& f = HexMngr.GetField( hx, hy );
        if( f.Crit && f.Crit->IsFinishing() )
            EraseCritter( f.Crit->GetId() );

        HexMngr.TransitCritter( cr, hx, hy, false, true );
        cr->AnimateStay();
        if( cr == Chosen )
        {
            // Chosen->TickStart(GameOpt.Breaktime);
            Chosen->TickStart( 200 );
            // SetAction(CHOSEN_NONE);
            MoveDirs.clear();
            Chosen->MoveSteps.clear();
            RebuildLookBorders = true;
        }
    }

    cr->ZeroSteps();
}

void FOClient::Net_OnChosenParams()
{
    WriteLog( "Chosen parameters... " );

    if( !Chosen )
    {
        char buf[NETMSG_ALL_PARAMS_SIZE - sizeof(uint)];
        Bin.Pop( buf, NETMSG_ALL_PARAMS_SIZE - sizeof(uint) );
        WriteLogX( "chosen not created, skip.\n" );
        return;
    }

    // Params
    Bin.Pop( (char*)Chosen->Params, sizeof(Chosen->Params) );

    // Process
    if( Chosen->GetParam( TO_BATTLE ) )
        AnimRun( IntWCombatAnim, ANIMRUN_SET_FRM( 244 ) );
    else
        AnimRun( IntWCombatAnim, ANIMRUN_SET_FRM( 0 ) );
    if( IsScreenPresent( CLIENT_SCREEN_CHARACTER ) )
        ChaPrepareSwitch();

    // Process all changed parameters
    for( int i = 0; i < MAX_PARAMS; i++ )
        Chosen->ChangeParam( i );
    Chosen->ProcessChangedParams();

    // Animate
    if( !Chosen->IsAnim() )
        Chosen->AnimateStay();

    // Refresh borders
    RebuildLookBorders = true;

    WriteLogX( "complete\n" );
}

void FOClient::Net_OnChosenParam()
{
    uint16 index;
    int    value;
    Bin >> index;
    Bin >> value;
    if( GameOpt.DebugNet )
        AddMess( MSGBOX_GAME, Str::FormatBuf( " - index<%u> value<%d>.", index, value ) );

    // Chosen specified parameters
    if( !Chosen )
        return;

    int old_value = 0;
    if( index < MAX_PARAMS )
    {
        Chosen->ChangeParam( index );
        old_value = Chosen->Params[index];
        Chosen->Params[index] = value;
        Chosen->ProcessChangedParams();
    }

    if( index >= ST_ANIM3D_LAYER_BEGIN && index <= ST_ANIM3D_LAYER_END )
    {
        if( !Chosen->IsAnim() )
            Chosen->Action( CRITTER_ACTION_REFRESH, 0, NULL, false );
        return;
    }

    switch( index )
    {
        case ST_CURRENT_AP:
        {
            Chosen->ApRegenerationTick = 0;
			WriteLog("FOClient::Net_OnChosenParam - ST_CURRENT_AP param called, it will set regenTick to 0. apRegenTick = %u\n", Chosen->ApRegenerationTick);
			break;
        }
        case TO_BATTLE:
        {
            if( Chosen->GetParam( TO_BATTLE ) )
            {
                AnimRun( IntWCombatAnim, ANIMRUN_TO_END );
                if( AnimGetCurSprCnt( IntWCombatAnim ) == 0 )
                    SndMngr.PlaySound( SOUND_COMBAT_MODE_ON );
            }
            else
            {
                AnimRun( IntWCombatAnim, ANIMRUN_FROM_END );
                if( AnimGetCurSprCnt( IntWCombatAnim ) == AnimGetSprCount( IntWCombatAnim ) - 1 )
                    SndMngr.PlaySound( SOUND_COMBAT_MODE_OFF );
            }
            break;
        }
        case OTHER_BREAK_TIME:
        {
            if( value < 0 )
                value = 0;
            Chosen->TickStart( value );
            SetAction( CHOSEN_NONE );
            MoveDirs.clear();
            Chosen->MoveSteps.clear();
            break;
        }
        case OTHER_FLAGS:
        {
            Chosen->Flags = value;
            break;
        }
        case OTHER_BASE_TYPE:
        {
            if( Chosen->Multihex < 0 && CritType::GetMultihex( Chosen->GetCrType() ) != CritType::GetMultihex( value ) )
            {
                HexMngr.SetMultihex( Chosen->GetHexX(), Chosen->GetHexY(), CritType::GetMultihex( Chosen->GetCrType() ), false );
                HexMngr.SetMultihex( Chosen->GetHexX(), Chosen->GetHexY(), CritType::GetMultihex( value ), true );
            }

            Chosen->SetBaseType( value );
            if( !Chosen->IsAnim() )
                Chosen->Action( CRITTER_ACTION_REFRESH, 0, NULL, false );
            break;
        }
        case OTHER_MULTIHEX:
        {
            int old_mh = Chosen->GetMultihex();
            Chosen->Multihex = value;
            if( old_mh != (int)Chosen->GetMultihex() )
            {
                HexMngr.SetMultihex( Chosen->GetHexX(), Chosen->GetHexY(), old_mh, false );
                HexMngr.SetMultihex( Chosen->GetHexX(), Chosen->GetHexY(), Chosen->GetMultihex(), true );
            }
            break;
        }
        case OTHER_YOU_TURN:
        {
            if( value < 0 )
                value = 0;
            ChosenAction.clear();
            TurnBasedTime = Timer::GameTick() + value;
            TurnBasedCurCritterId = Chosen->GetId();
            HexMngr.SetCritterContour( 0, 0 );
            FlashGameWindow();
            break;
        }
        case OTHER_CLEAR_MAP:
        {
            CritMap crits = HexMngr.GetCritters();
            for( auto it = crits.begin(), end = crits.end(); it != end; ++it )
            {
                CritterCl* cr = (*it).second;
                if( cr != Chosen )
                    EraseCritter( cr->GetId() );
            }
            ItemHexVec items = HexMngr.GetItems();
            for( auto it = items.begin(), end = items.end(); it != end; ++it )
            {
                ItemHex* item = *it;
                if( item->IsItem() )
                    HexMngr.DeleteItem( *it, true );
            }
            break;
        }
        case OTHER_TELEPORT:
        {
            uint16 hx = (value >> 16) & 0xFFFF;
            uint16 hy = value & 0xFFFF;
            if( hx < HexMngr.GetMaxHexX() && hy < HexMngr.GetMaxHexY() )
            {
                CritterCl* cr = HexMngr.GetField( hx, hy ).Crit;
                if( Chosen == cr )
                    break;
                if( !Chosen->IsDead() && cr )
                    EraseCritter( cr->GetId() );
                HexMngr.RemoveCrit( Chosen );
                Chosen->HexX = hx;
                Chosen->HexY = hy;
                HexMngr.SetCrit( Chosen );
                HexMngr.ScrollToHex( Chosen->GetHexX(), Chosen->GetHexY(), 0.1, true );
            }
            break;
        }
        default:
            break;
    }

    if( IsScreenPresent( CLIENT_SCREEN_CHARACTER ) )
        ChaPrepareSwitch();
    RebuildLookBorders = true;     // Maybe changed some parameter influencing on look borders
}

void FOClient::Net_OnChosenClearItems()
{
    if( Chosen )
    {
        if( Chosen->IsHaveLightSources() )
            HexMngr.RebuildLight();
        Chosen->EraseAllItems();
    }
    CollectContItems();
}

/**
*	AS item add/remove callbacks:
*	Here we have an issue when splitting items, AS will show same amount removed as added, it is bugged somehow.
*	This does not happen when buying with barter for example. 
*/
void FOClient::Net_OnChosenAddItem()
{
    uint   item_id;
    uint16 pid;
    uint8  slot;
    Bin >> item_id;
    Bin >> pid;
    Bin >> slot;

	int itemsRemoved = 0;
	int itemsAdded = 0;

    Item* item = NULL;
    uint8 prev_slot = SLOT_INV;
    uint  prev_light_hash = 0;
    if( Chosen )
    {
        item = Chosen->GetItem( item_id );
        if( item )
        {
            prev_slot = item->AccCritter.Slot;
            prev_light_hash = item->LightGetHash();
			itemsRemoved = item->GetCount();
			Chosen->EraseItem( item, false );
            item = NULL;
        }

        ProtoItem* proto_item = ItemMngr.GetProtoItem( pid );
        if( proto_item )
        {
            item = new Item();
            item->Id = item_id;
            item->Init( proto_item );
        }
    }

    if( !item )
    {
        WriteLogF( _FUNC_, " - Can't create item, pid<%u>.\n", pid );
        Item::ItemData dummy;
        Bin.Pop( (char*)&dummy, sizeof(dummy) );
        return;
    }

    Bin.Pop( (char*)&item->Data, sizeof(item->Data) );
    item->Accessory = ITEM_ACCESSORY_CRITTER;
    item->AccCritter.Slot = slot;
    if( item != Chosen->ItemSlotMain || !item->IsWeapon() )
        item->SetMode( item->Data.Mode );

	itemsAdded = item->GetCount();
    Chosen->AddItem( item );

    if( slot == SLOT_HAND1 || prev_slot == SLOT_HAND1 )
        RebuildLookBorders = true;
    if( item->LightGetHash() != prev_light_hash && (slot != SLOT_INV || prev_slot != SLOT_INV) )
        HexMngr.RebuildLight();
	if (item->IsHidden()) {
		Chosen->EraseItem(item, true);
	} else {
		//	Need to do this shit in case of stackables									   
		float netGain = itemsAdded - itemsRemoved;
		if (itemsAdded > itemsRemoved) {
			//	Player received an item
			addItemASCallback(item, netGain, "Net_OnChosenAddItem() :: NET GAIN");
		}
		else {
			//	Player lost items, return their currnet item amount in case of stackables
			addItemASCallback(item, netGain, "Net_OnChosenAddItem() :: NET LOSS");
		}
	}
    CollectContItems();
}

void FOClient::addItemASCallback(Item* item, float netGain, string debugMessage) {
	if (Script::PrepareContext(ClientFunctions.ItemInvIn, _FUNC_, "Game"))
	{
		Script::SetArgObject(item);
		Script::SetArgFloat(netGain);
		Script::SetArgObject(new ScriptString(debugMessage));
		Script::RunPrepared();
	}
}

void FOClient::removeItemASCallback(Item* item, float netGain, string debugMessage) {
	if (Script::PrepareContext(ClientFunctions.ItemInvOut, _FUNC_, "Game"))
	{
		Script::SetArgObject(item);
		Script::SetArgFloat(netGain);
		Script::SetArgObject(new ScriptString(debugMessage));
		Script::RunPrepared();
	}
}


void FOClient::Net_OnChosenEraseItem()
{
    uint item_id;
    Bin >> item_id;

    if( !Chosen )
    {
        WriteLogF( _FUNC_, " - Chosen is not created.\n" );
        return;
    }

    Item* item = Chosen->GetItem( item_id );
    if( !item )
    {
        WriteLogF( _FUNC_, " - Item not found, id<%u>.\n", item_id );
        return;
    }

	removeItemASCallback(item, -(float)(item->GetCount()), "Net_OnChosenEraseItem()");

	if( item->IsLight() && item->AccCritter.Slot != SLOT_INV )
        HexMngr.RebuildLight();
    Chosen->EraseItem( item, true );
    CollectContItems();
}

void FOClient::Net_OnAddItemOnMap()
{
    uint           item_id;
    uint16         item_pid;
    uint16         item_x;
    uint16         item_y;
    uint8          is_added;
    Item::ItemData data;
    Bin >> item_id;
    Bin >> item_pid;
    Bin >> item_x;
    Bin >> item_y;
    Bin >> is_added;
    Bin.Pop( (char*)&data, sizeof(data) );

    if( HexMngr.IsMapLoaded() )
    {
        HexMngr.AddItem( item_id, item_pid, item_x, item_y, is_added != 0, &data );
    }
    else     // Global map car
    {
        ProtoItem* proto_item = ItemMngr.GetProtoItem( item_pid );
        if( proto_item && proto_item->IsCar() )
        {
            SAFEREL( GmapCar.Car );
            GmapCar.Car = new Item();
            GmapCar.Car->Id = item_id;
            GmapCar.Car->Init( proto_item );
            GmapCar.Car->Data = data;
        }
    }

    Item* item = HexMngr.GetItemById( item_id );
    if( item && Script::PrepareContext( ClientFunctions.ItemMapIn, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( item );
        Script::RunPrepared();
    }

    // Refresh borders
    if( item && !item->IsRaked() )
        RebuildLookBorders = true;
}

void FOClient::Net_OnChangeItemOnMap()
{
    uint           item_id;
    Item::ItemData data;
    Bin >> item_id;
    Bin.Pop( (char*)&data, sizeof(data) );

    Item* item = HexMngr.GetItemById( item_id );
    bool  is_raked = (item && item->IsRaked() );

    HexMngr.ChangeItem( item_id, data );

    if( item && Script::PrepareContext( ClientFunctions.ItemMapChanged, _FUNC_, "Game" ) )
    {
        Item* prev_state = new Item( *item );
        prev_state->RefCounter = 1;
        prev_state->Data = data;
        Script::SetArgObject( item );
        Script::SetArgObject( prev_state );
        Script::RunPrepared();
        prev_state->Release();
    }

    // Refresh borders
    if( item && is_raked != item->IsRaked() )
        RebuildLookBorders = true;
}

void FOClient::Net_OnEraseItemFromMap()
{
    uint item_id;
    bool is_deleted;
    Bin >> item_id;
    Bin >> is_deleted;

    HexMngr.FinishItem( item_id, is_deleted );

    Item* item = HexMngr.GetItemById( item_id );
    if( item && Script::PrepareContext( ClientFunctions.ItemMapOut, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( item );
        Script::RunPrepared();
    }

    // Refresh borders
    if( item && !item->IsRaked() )
        RebuildLookBorders = true;
}

void FOClient::Net_OnAnimateItem()
{
    uint  item_id;
    uint8 from_frm;
    uint8 to_frm;
    Bin >> item_id;
    Bin >> from_frm;
    Bin >> to_frm;

    ItemHex* item = HexMngr.GetItemById( item_id );
    if( item )
        item->SetAnim( from_frm, to_frm );
}

void FOClient::Net_OnCombatResult()
{
    uint    msg_len;
    uint    data_count;
    UIntVec data_vec;
    Bin >> msg_len;
    Bin >> data_count;
    if( data_count > GameOpt.FloodSize / sizeof(uint) )
        return;                                               // Insurance
    if( data_count )
    {
        data_vec.resize( data_count );
        Bin.Pop( (char*)&data_vec[0], data_count * sizeof(uint) );
    }

    CHECK_IN_BUFF_ERROR;

    ScriptArray* arr = Script::CreateArray( "uint[]" );
    if( !arr )
        return;
    arr->Resize( data_count );
    for( uint i = 0; i < data_count; i++ )
        *( (uint*)arr->At( i ) ) = data_vec[i];

    if( Script::PrepareContext( ClientFunctions.CombatResult, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( arr );
        Script::RunPrepared();
    }

    arr->Release();
}

void FOClient::Net_OnEffect()
{
    uint16 eff_pid;
    uint16 hx;
    uint16 hy;
    uint16 radius;
    Bin >> eff_pid;
    Bin >> hx;
    Bin >> hy;
    Bin >> radius;

    // Base hex effect
    HexMngr.RunEffect( eff_pid, hx, hy, hx, hy );

    // Radius hexes effect
    if( radius > MAX_HEX_OFFSET )
        radius = MAX_HEX_OFFSET;
    int    cnt = NumericalNumber( radius ) * DIRS_COUNT;
    short* sx, * sy;
    GetHexOffsets( hx & 1, sx, sy );
    int    maxhx = HexMngr.GetMaxHexX();
    int    maxhy = HexMngr.GetMaxHexY();

    for( int i = 0; i < cnt; i++ )
    {
        int ex = hx + sx[i];
        int ey = hy + sy[i];
        if( ex >= 0 && ey >= 0 && ex < maxhx && ey < maxhy )
            HexMngr.RunEffect( eff_pid, ex, ey, ex, ey );
    }
}

#pragma MESSAGE("Synchronize effects showing.")
void FOClient::Net_OnFlyEffect()
{
    uint16 eff_pid;
    uint   eff_cr1_id;
    uint   eff_cr2_id;
    uint16 eff_cr1_hx;
    uint16 eff_cr1_hy;
    uint16 eff_cr2_hx;
    uint16 eff_cr2_hy;
    Bin >> eff_pid;
    Bin >> eff_cr1_id;
    Bin >> eff_cr2_id;
    Bin >> eff_cr1_hx;
    Bin >> eff_cr1_hy;
    Bin >> eff_cr2_hx;
    Bin >> eff_cr2_hy;

    CritterCl* cr1 = GetCritter( eff_cr1_id );
    CritterCl* cr2 = GetCritter( eff_cr2_id );

    if( cr1 )
    {
        eff_cr1_hx = cr1->GetHexX();
        eff_cr1_hy = cr1->GetHexY();
    }

    if( cr2 )
    {
        eff_cr2_hx = cr2->GetHexX();
        eff_cr2_hy = cr2->GetHexY();
    }

    if( !HexMngr.RunEffect( eff_pid, eff_cr1_hx, eff_cr1_hy, eff_cr2_hx, eff_cr2_hy ) )
    {
        WriteLogF( _FUNC_, " - Run effect fail, pid<%u>.\n", eff_pid );
        return;
    }
}

void FOClient::Net_OnPlaySound( bool by_type )
{
    if( !by_type )
    {
        uint synchronize_crid;
        char sound_name[101];
        Bin >> synchronize_crid;
        Bin.Pop( sound_name, 100 );
        sound_name[100] = 0;
        SndMngr.PlaySound( sound_name );
    }
    else
    {
        uint  synchronize_crid;
        uint8 sound_type;
        uint8 sound_type_ext;
        uint8 sound_id;
        uint8 sound_id_ext;
        Bin >> synchronize_crid;
        Bin >> sound_type;
        Bin >> sound_type_ext;
        Bin >> sound_id;
        Bin >> sound_id_ext;
        SndMngr.PlaySoundType( sound_type, sound_type_ext, sound_id, sound_id_ext );
    }
}

void FOClient::Net_OnPing()
{
    uint8 ping;
    Bin >> ping;

    if( ping == PING_WAIT )
    {
        if( GetActiveScreen() == CLIENT_SCREEN_BARTER )
            SetCurMode( CURSOR_HAND );
        else
            SetLastCurMode();
    }
    else if( ping == PING_CLIENT )
    {
        if( UIDFail )
            return;

        Bout << NETMSG_PING;
        Bout << (uint8)PING_CLIENT;
    }
    else if( ping == PING_PING )
    {
        GameOpt.Ping = Timer::FastTick() - PingTick;
        PingTick = 0;
        PingCallTick = Timer::FastTick() + GameOpt.PingPeriod;
    }

    //CHECK_MULTIPLY_WINDOWS8;
    //CHECK_MULTIPLY_WINDOWS9;
}

void FOClient::Net_OnChosenTalk()
{
    uint  msg_len;
    uint8 is_npc;
    uint  talk_id;
    //uint8 count_answ;
    uint  text_id;
    uint  talk_time;

    Bin >> msg_len;
    Bin >> is_npc;
    Bin >> talk_id;
    Bin >> count_answ;

    DlgCurAnswPage = 0;
    DlgMaxAnswPage = 0;
    DlgAllAnswers.clear();
    DlgAnswers.clear();
	answers_texts.clear();

    if( !count_answ )
    {
        // End dialog or barter
        if( IsScreenPresent( CLIENT_SCREEN_DIALOG ) )
            HideScreen( CLIENT_SCREEN_DIALOG );
        if( IsScreenPresent( CLIENT_SCREEN_BARTER ) )
            HideScreen( CLIENT_SCREEN_BARTER );
        return;
    }

    // Text params
    uint16 lexems_len;
    memset(lexems, 0, MAX_DLG_LEXEMS_TEXT + 1);
    Bin >> lexems_len;
    if( lexems_len && lexems_len <= MAX_DLG_LEXEMS_TEXT )
    {
        Bin.Pop( lexems, lexems_len );
        lexems[lexems_len] = '\0';
    }

    // Find critter
    DlgIsNpc = is_npc;
    DlgNpcId = talk_id;
    npc = (is_npc ? GetCritter( talk_id ) : NULL);

    // Avatar
    DlgAvatarPic = NULL;
    if( npc && npc->Avatar.length() )
        DlgAvatarPic = ResMngr.GetAnim( Str::GetHash( npc->Avatar.c_str() ), 0, RES_IFACE_EXT );

    // Main text
    Bin >> text_id;

    char str[MAX_FOTEXT];
    Str::Copy( str, MsgDlg->GetStr( text_id ) );
    FormatTags( str, MAX_FOTEXT, Chosen, npc, lexems );
    DlgMainText = str;
    DlgMainTextCur = 0;
    DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, str, dialogFont);

    // Answers
    //UIntVec answers_texts;
    for( int i = 0; i < count_answ; i++ )
    {
        Bin >> text_id;
        answers_texts.push_back( text_id );
    }

    const char answ_beg[] = { ' ', ' ', (char)TEXT_SYMBOL_DOT, ' ', 0 };
    const char page_up[] = { (char)TEXT_SYMBOL_UP, (char)TEXT_SYMBOL_UP, (char)TEXT_SYMBOL_UP, 0 };
    const int  page_up_height = SprMngr.GetLinesHeight( DlgAnswText.W(), 0, page_up, dialogFont);
    const char page_down[] = { (char)TEXT_SYMBOL_DOWN, (char)TEXT_SYMBOL_DOWN, (char)TEXT_SYMBOL_DOWN, 0 };
    const int  page_down_height = SprMngr.GetLinesHeight( DlgAnswText.W(), 0, page_down, dialogFont);

    int        line = 0, height = 0, page = 0, answ = 0;
    while( true )
    {
        Rect pos(
            DlgAnswText.L + DlgNextAnswX* line,
            DlgAnswText.T + DlgNextAnswY* line + height,
            DlgAnswText.R + DlgNextAnswX* line,
            DlgAnswText.T + DlgNextAnswY* line + height );

        // Up arrow
        if( page && !line )
        {
            height += page_up_height;
            pos.B += page_up_height;
            line++;
            DlgAllAnswers.push_back( Answer( page, pos, page_up, -1 ) );
            continue;
        }

        Str::Copy( str, MsgDlg->GetStr( answers_texts[answ] ) );
        FormatTags( str, MAX_FOTEXT, Chosen, npc, lexems );
        Str::Insert( str, answ_beg );      // TODO: GetStr

        height += SprMngr.GetLinesHeight( DlgAnswText.W(), 0, str, dialogFont);
        pos.B = DlgAnswText.T + DlgNextAnswY * line + height;

        if( pos.B >= DlgAnswText.B && line > 1 )
        {
            // Down arrow
            Answer& answ_prev = DlgAllAnswers.back();
            if( line > 2 && DlgAnswText.B - answ_prev.Position.B < page_down_height )     // Check free space
            {
                // Not enough space
                // Migrate last answer to next page
                DlgAllAnswers.pop_back();
                pos = answ_prev.Position;
                pos.B = pos.T + page_down_height;
                DlgAllAnswers.push_back( Answer( page, pos, page_down, -2 ) );
                answ--;
            }
            else
            {
                // Add down arrow
                pos.B = pos.T + page_down_height;
                DlgAllAnswers.push_back( Answer( page, pos, page_down, -2 ) );
            }

            page++;
            height = 0;
            line = 0;
            DlgMaxAnswPage = page;
        }
        else
        {
            // Answer
            DlgAllAnswers.push_back( Answer( page, pos, str, answ ) );
            line++;
            answ++;
            if( answ >= count_answ )
                break;
        }
    }

    Bin >> talk_time;

    DlgCollectAnswers( false );
    if( talk_time )
        DlgEndTick = Timer::GameTick() + talk_time;
    else
        DlgEndTick = 0;
    if( IsScreenPresent( CLIENT_SCREEN_BARTER ) )
        HideScreen( CLIENT_SCREEN_BARTER );
    ShowScreen( CLIENT_SCREEN_DIALOG );
}

void FOClient::Net_OnCheckUID2()
{
    CHECKUIDBIN;
    if( CHECK_UID2( uid ) )
        UIDFail = true;
    //CHECK_MULTIPLY_WINDOWS6;
}

void FOClient::Net_OnGameInfo()
{
    int    time;
    uint8  rain;
    bool   turn_based;
    bool   no_log_out;
    int*   day_time = HexMngr.GetMapDayTime();
    uint8* day_color = HexMngr.GetMapDayColor();
    Bin >> GameOpt.YearStart;
    Bin >> GameOpt.Year;
    Bin >> GameOpt.Month;
    Bin >> GameOpt.Day;
    Bin >> GameOpt.Hour;
    Bin >> GameOpt.Minute;
    Bin >> GameOpt.Second;
    Bin >> GameOpt.TimeMultiplier;
    Bin >> time;
    Bin >> rain;
    Bin >> turn_based;
    Bin >> no_log_out;
    Bin.Pop( (char*)day_time, sizeof(int) * 4 );
    Bin.Pop( (char*)day_color, sizeof(uint8) * 12 );

    CHECK_IN_BUFF_ERROR;

    DateTime dt = { GameOpt.YearStart, 1, 0, 1, 0, 0, 0, 0 };
    uint64   start_ft;
    Timer::DateTimeToFullTime( dt, start_ft );
    GameOpt.YearStartFTHi = (start_ft >> 32) & 0xFFFFFFFF;
    GameOpt.YearStartFTLo = start_ft & 0xFFFFFFFF;
    GameOpt.FullSecond = Timer::GetFullSecond( GameOpt.Year, GameOpt.Month, GameOpt.Day, GameOpt.Hour, GameOpt.Minute, GameOpt.Second );
    GameOpt.FullSecondStart = GameOpt.FullSecond;
    GameOpt.GameTimeTick = Timer::GameTick();

    HexMngr.SetWeather( time, rain );
    SetDayTime( true );
    IsTurnBased = turn_based;
    if( !IsTurnBased )
        HexMngr.SetCritterContour( 0, 0 );
    NoLogOut = no_log_out;
}

void FOClient::Net_OnLoadMap()
{
    WriteLog( "Change map...\n" );

    uint16 map_pid;
    int    map_time;
    uint8  map_rain;
    uint   hash_tiles;
    uint   hash_walls;
    uint   hash_scen;
    Bin >> map_pid;
    Bin >> map_time;
    Bin >> map_rain;
    Bin >> hash_tiles;
    Bin >> hash_walls;
    Bin >> hash_scen;

    GameOpt.SpritesZoom = 1.0f;
    WorldmapZoom = 1.0f;

    GameMapTexts.clear();
    HexMngr.UnloadMap();
    SndMngr.ClearSounds();
    FlashGameWindow();
    ShowMainScreen( CLIENT_MAIN_SCREEN_WAIT );
    ClearCritters();

    ResMngr.FreeResources( RES_IFACE_EXT );
    if( map_pid )     // Free global map resources
    {
        GmapFreeResources();
    }
    else     // Free local map resources
    {
        ResMngr.FreeResources( RES_ITEMS );
        ResMngr.FreeResources( RES_CRITTERS );
    }

    DropScroll();
    IsTurnBased = false;

    // Global
    if( !map_pid )
    {
        WorldmapNullParams();
        ShowMainScreen( CLIENT_MAIN_SCREEN_WORLDMAP );
        Net_SendLoadMapOk();
        #ifndef FO_D3D
        if( IsVideoPlayed() )
            MusicAfterVideo = MsgGM->GetStr( STR_MAP_MUSIC_( map_pid ) );
        else
            SndMngr.PlayMusic( MsgGM->GetStr( STR_MAP_MUSIC_( map_pid ) ) );
        #else
        SndMngr.PlayMusic( MsgGM->GetStr( STR_MAP_MUSIC_( map_pid ) ) );
        #endif
        WriteLog( "WorldMap loaded.\n" );
        return;
    }

    // Local
    uint hash_tiles_cl = 0;
    uint hash_walls_cl = 0;
    uint hash_scen_cl = 0;
    HexMngr.GetMapHash( map_pid, hash_tiles_cl, hash_walls_cl, hash_scen_cl );

    if( hash_tiles != hash_tiles_cl || hash_walls != hash_walls_cl || hash_scen != hash_scen_cl )
    {
        Net_SendGiveMap( false, map_pid, 0, hash_tiles_cl, hash_walls_cl, hash_scen_cl );
        return;
    }

    if( !HexMngr.LoadMap( map_pid ) )
    {
        WriteLog( "Map not loaded. Disconnect.\n" );
        IsConnected = false;
        return;
    }

    HexMngr.SetWeather( map_time, map_rain );
    SetDayTime( true );
    Net_SendLoadMapOk();
    LookBorders.clear();
    ShootBorders.clear();
    #ifndef FO_D3D
    if( IsVideoPlayed() )
        MusicAfterVideo = MsgGM->GetStr( STR_MAP_MUSIC_( map_pid ) );
    else
        SndMngr.PlayMusic( MsgGM->GetStr( STR_MAP_MUSIC_( map_pid ) ) );
    #else
    SndMngr.PlayMusic( MsgGM->GetStr( STR_MAP_MUSIC_( map_pid ) ) );
    #endif
    WriteLog( "Local map loaded.\n" );
}

void FOClient::Net_OnMap()
{
    WriteLog( "Get map" );

    uint   msg_len;
    uint16 map_pid;
    uint16 maxhx, maxhy;
    uint8  send_info;
    Bin >> msg_len;
    Bin >> map_pid;
    Bin >> maxhx;
    Bin >> maxhy;
    Bin >> send_info;

    WriteLogX( "<%u>... ", map_pid );

    char map_name[256];
    Str::Format( map_name, "map%u", map_pid );

    bool  tiles = false;
    char* tiles_data = NULL;
    uint  tiles_len = 0;
    bool  walls = false;
    char* walls_data = NULL;
    uint  walls_len = 0;
    bool  scen = false;
    char* scen_data = NULL;
    uint  scen_len = 0;

    WriteLogX( "New:" );
    if( FLAG( send_info, SENDMAP_TILES ) )
    {
        WriteLogX( " Tiles" );
        uint count_tiles;
        Bin >> count_tiles;
        if( count_tiles )
        {
            tiles_len = count_tiles * sizeof(ProtoMap::Tile);
            tiles_data = new char[tiles_len];
            Bin.Pop( tiles_data, tiles_len );
        }
        tiles = true;
    }

    if( FLAG( send_info, SENDMAP_WALLS ) )
    {
        WriteLogX( " Walls" );
        uint count_walls = 0;
        Bin >> count_walls;
        if( count_walls )
        {
            walls_len = count_walls * sizeof(SceneryCl);
            walls_data = new char[walls_len];
            Bin.Pop( walls_data, walls_len );
        }
        walls = true;
    }

    if( FLAG( send_info, SENDMAP_SCENERY ) )
    {
        WriteLogX( " Scenery" );
        uint count_scen = 0;
        Bin >> count_scen;
        if( count_scen )
        {
            scen_len = count_scen * sizeof(SceneryCl);
            scen_data = new char[scen_len];
            Bin.Pop( scen_data, scen_len );
        }
        scen = true;
    }

    CHECK_IN_BUFF_ERROR;

    uint        cache_len;
    uint8*      cache = Crypt.GetCache( map_name, cache_len );
    FileManager fm;

    WriteLogX( " Old:" );
    if( cache && fm.LoadStream( cache, cache_len ) )
    {
        uint   buf_len = fm.GetFsize();
        uint8* buf = Crypt.Uncompress( fm.GetBuf(), buf_len, 50 );
        if( buf )
        {
            fm.UnloadFile();
            fm.LoadStream( buf, buf_len );
            delete[] buf;

            if( fm.GetBEUInt() == CLIENT_MAP_FORMAT_VER )
            {
                fm.SetCurPos( 0x04 );           // Skip pid
                fm.GetBEUInt();                 // Skip max hx/hy
                fm.GetBEUInt();                 // Reserved
                fm.GetBEUInt();                 // Reserved

                fm.SetCurPos( 0x20 );
                uint old_tiles_len = fm.GetBEUInt();
                uint old_walls_len = fm.GetBEUInt();
                uint old_scen_len = fm.GetBEUInt();

                if( !tiles )
                {
                    WriteLogX( " Tiles" );
                    tiles_len = old_tiles_len;
                    fm.SetCurPos( 0x2C );
                    tiles_data = new char[tiles_len];
                    fm.CopyMem( tiles_data, tiles_len );
                    tiles = true;
                }

                if( !walls )
                {
                    WriteLogX( " Walls" );
                    walls_len = old_walls_len;
                    fm.SetCurPos( 0x2C + old_tiles_len );
                    walls_data = new char[walls_len];
                    fm.CopyMem( walls_data, walls_len );
                    walls = true;
                }

                if( !scen )
                {
                    WriteLogX( " Scenery" );
                    scen_len = old_scen_len;
                    fm.SetCurPos( 0x2C + old_tiles_len + old_walls_len );
                    scen_data = new char[scen_len];
                    fm.CopyMem( scen_data, scen_len );
                    scen = true;
                }

                fm.UnloadFile();
            }
        }
    }
    SAFEDELA( cache );

    if( tiles && walls && scen )
    {
        fm.ClearOutBuf();

        fm.SetBEUInt( CLIENT_MAP_FORMAT_VER );
        fm.SetBEUInt( map_pid );
        fm.SetBEUShort( maxhx );
        fm.SetBEUShort( maxhy );
        fm.SetBEUInt( 0 );
        fm.SetBEUInt( 0 );

        fm.SetBEUInt( tiles_len / sizeof(ProtoMap::Tile) );
        fm.SetBEUInt( walls_len / sizeof(SceneryCl) );
        fm.SetBEUInt( scen_len / sizeof(SceneryCl) );
        fm.SetBEUInt( tiles_len );
        fm.SetBEUInt( walls_len );
        fm.SetBEUInt( scen_len );

        if( tiles_len )
            fm.SetData( tiles_data, tiles_len );
        if( walls_len )
            fm.SetData( walls_data, walls_len );
        if( scen_len )
            fm.SetData( scen_data, scen_len );

        uint   obuf_len = fm.GetOutBufLen();
        uint8* buf = Crypt.Compress( fm.GetOutBuf(), obuf_len );
        if( !buf )
        {
            WriteLog( "Failed to compress data<%s>, disconnect.\n", map_name );
            IsConnected = false;
            fm.ClearOutBuf();
            return;
        }

        fm.ClearOutBuf();
        fm.SetData( buf, obuf_len );
        delete[] buf;

        Crypt.SetCache( map_name, fm.GetOutBuf(), fm.GetOutBufLen() );
        fm.ClearOutBuf();
    }
    else
    {
        WriteLog( "Not for all data of map, disconnect.\n" );
        IsConnected = false;
        SAFEDELA( tiles_data );
        SAFEDELA( walls_data );
        SAFEDELA( scen_data );
        return;
    }

    SAFEDELA( tiles_data );
    SAFEDELA( walls_data );
    SAFEDELA( scen_data );

    AutomapWaitPids.erase( map_pid );
    AutomapReceivedPids.insert( map_pid );

    WriteLog( "Map saved.\n" );
}

void FOClient::Net_OnGlobalInfo()
{
    uint  msg_len;
    uint8 info_flags;
    Bin >> msg_len;
    Bin >> info_flags;

    if( FLAG( info_flags, GM_INFO_LOCATIONS ) )
    {
        GmapLoc.clear();
        uint16 count_loc;
        Bin >> count_loc;

        for( int i = 0; i < count_loc; i++ )
        {
            GmapLocation loc;
            Bin >> loc.LocId;
            Bin >> loc.LocPid;
            Bin >> loc.LocWx;
            Bin >> loc.LocWy;
            Bin >> loc.Radius;
            Bin >> loc.Color;

            if( loc.LocId )
                GmapLoc.push_back( loc );
        }
    }

    if( FLAG( info_flags, GM_INFO_LOCATION ) )
    {
        GmapLocation loc;
        bool         add;
        Bin >> add;
        Bin >> loc.LocId;
        Bin >> loc.LocPid;
        Bin >> loc.LocWx;
        Bin >> loc.LocWy;
        Bin >> loc.Radius;
        Bin >> loc.Color;

        auto it = std::find( GmapLoc.begin(), GmapLoc.end(), loc.LocId );
        if( add )
        {
            if( it != GmapLoc.end() )
                *it = loc;
            else
                GmapLoc.push_back( loc );
        }
        else
        {
            if( it != GmapLoc.end() )
                GmapLoc.erase( it );
        }
    }

    if( FLAG( info_flags, GM_INFO_GROUP_PARAM ) )
    {
        uint16 cur_x, cur_y, to_x, to_y;
        uint   speed;
        Bin >> cur_x;
        Bin >> cur_y;
        Bin >> to_x;
        Bin >> to_y;
        Bin >> speed;
        Bin >> WorldmapWait;

        GmapGroupRealOldX = WorldmapGroupCurX;
        GmapGroupRealOldY = WorldmapGroupCurY;
        GmapGroupRealCurX = cur_x;
        GmapGroupRealCurY = cur_y;
        WorldmapGroupToX = to_x;
        WorldmapGroupToY = to_y;
        WorldmapGroupSpeed = (float)speed / 1000000.0f;
        GmapMoveTick = Timer::GameTick();

        if( !WorldmapActive )
        {
            GmapGroupRealOldX = GmapGroupRealCurX;
            GmapGroupRealOldY = GmapGroupRealCurY;
            WorldmapGroupCurX = GmapGroupRealCurX;
            WorldmapGroupCurY = GmapGroupRealCurY;
            WorldmapOffsetX = (WorldmapWMap[2] - WorldmapWMap[0]) / 2 + WorldmapWMap[0] - WorldmapGroupCurX;
            WorldmapOffsetY = (WorldmapWMap[3] - WorldmapWMap[1]) / 2 + WorldmapWMap[1] - WorldmapGroupCurY;
            GmapTrace.clear();

            int w = GM_MAXX;
            int h = GM_MAXY;
            if( WorldmapOffsetX > WorldmapWMap[0] )
                WorldmapOffsetX = WorldmapWMap[0];
            if( WorldmapOffsetY > WorldmapWMap[1] )
                WorldmapOffsetY = WorldmapWMap[1];
            if( WorldmapOffsetX < WorldmapWMap[2] - w )
                WorldmapOffsetX = WorldmapWMap[2] - w;
            if( WorldmapOffsetY < WorldmapWMap[3] - h )
                WorldmapOffsetY = WorldmapWMap[3] - h;

            SetCurMode( CURSOR_DEFAULT );
        }

        // Car master id
        Bin >> GmapCar.MasterId;

        SAFEREL( GmapCar.Car );

        WorldmapActive = true;
    }

    if( FLAG( info_flags, GM_INFO_ZONES_FOG ) )
    {
        Bin.Pop( (char*)WorldmapFog.GetData(), GM_ZONES_FOG_SIZE );
    }

    if( FLAG( info_flags, GM_INFO_FOG ) )
    {
        uint16 zx, zy;
        uint8  fog;
        Bin >> zx;
        Bin >> zy;
        Bin >> fog;
        WorldmapFog.Set2Bit( zx, zy, fog );
    }

    if( FLAG( info_flags, GM_INFO_CRITTERS ) )
    {
        ClearCritters();
        // After wait AddCritters
    }

    CHECK_IN_BUFF_ERROR;
}

void FOClient::Net_OnGlobalEntrances()
{
    uint  msg_len;
    uint  loc_id;
    uint8 count;
    bool  entrances[0x100];
    memzero( entrances, sizeof(entrances) );
    Bin >> msg_len;
    Bin >> loc_id;
    Bin >> count;

    for( int i = 0; i < count; i++ )
    {
        uint8 e;
        Bin >> e;
        entrances[e] = true;
    }

    CHECK_IN_BUFF_ERROR;

    WorldmapShowEntrancesLocId = loc_id;
    memcpy( WorldmapShowEntrances, entrances, sizeof(entrances) );
}

void FOClient::Net_OnContainerInfo()
{
    uint   msg_len;
    uint8  transfer_type;
    uint   talk_time;
    uint   cont_id;
    uint16 cont_pid;     // Or Barter K
    uint   items_count;
    Bin >> msg_len;
    Bin >> transfer_type;

    bool open_screen = (FLAG( transfer_type, 0x80 ) ? true : false);
    UNSETFLAG( transfer_type, 0x80 );

    if( transfer_type == TRANSFER_CLOSE )
    {
        if( IsScreenPresent( CLIENT_SCREEN_BARTER ) )
            HideScreen( CLIENT_SCREEN_BARTER );
        if( IsScreenPresent( CLIENT_SCREEN_PICKUP ) )
            HideScreen( CLIENT_SCREEN_PICKUP );
        return;
    }

    Bin >> talk_time;
    Bin >> cont_id;
    Bin >> cont_pid;
    Bin >> items_count;

    ItemVec item_container;
    for( uint i = 0; i < items_count; i++ )
    {
        uint           item_id;
        uint16         item_pid;
        Item::ItemData data;
        Bin >> item_id;
        Bin >> item_pid;
        Bin.Pop( (char*)&data, sizeof(data) );

        ProtoItem* proto_item = ItemMngr.GetProtoItem( item_pid );
        if( item_id && proto_item )
        {
            Item item;
            item.Init( proto_item );
            item.Id = item_id;
            item.Data = data;
            item_container.push_back( item );
        }
    }

    CHECK_IN_BUFF_ERROR;

    if( !Chosen )
        return;

    Item::SortItems( item_container );
    if( talk_time )
        DlgEndTick = Timer::GameTick() + talk_time;
    else
        DlgEndTick = 0;
    PupContId = cont_id;
    PupContPid = 0;

    if( transfer_type == TRANSFER_CRIT_BARTER )
    {
        PupTransferType = transfer_type;
        BarterK = cont_pid;
        BarterCount = items_count;
        BarterCont2Init.clear();
        BarterCont2Init = item_container;
        BarterScroll1o = 0;
        BarterScroll2o = 0;
        BarterCont1oInit.clear();
        BarterCont2oInit.clear();

        if( open_screen )
        {
            if( IsScreenPresent( CLIENT_SCREEN_DIALOG ) )
                HideScreen( CLIENT_SCREEN_DIALOG );
            if( !IsScreenPresent( CLIENT_SCREEN_BARTER ) )
                ShowScreen( CLIENT_SCREEN_BARTER );
            BarterScroll1 = 0;
            BarterScroll2 = 0;
            BarterText = "";
            DlgMainTextCur = 0;
            DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, BarterText.c_str(), dialogFont);
        }
    }
    else
    {
        PupTransferType = transfer_type;
        PupContPid = (transfer_type == TRANSFER_HEX_CONT_UP || transfer_type == TRANSFER_HEX_CONT_DOWN || transfer_type == TRANSFER_FAR_CONT ? cont_pid : 0);
        PupCount = items_count;
        PupCont2Init.clear();
        PupCont2Init = item_container;
        PupScrollCrit = 0;
        PupLastPutId = 0;

        if( open_screen )
        {
            if( !IsScreenPresent( CLIENT_SCREEN_PICKUP ) )
                ShowScreen( CLIENT_SCREEN_PICKUP );
            PupScroll1 = 0;
            PupScroll2 = 0;
        }

        if( PupTransferType == TRANSFER_CRIT_LOOT )
        {
            CritVec& loot = PupGetLootCrits();
            for( uint i = 0, j = (uint)loot.size(); i < j; i++ )
            {
                if( loot[i]->GetId() == PupContId )
                {
                    PupScrollCrit = i;
                    break;
                }
            }
        }
    }

    CollectContItems();
}

void FOClient::Net_OnFollow()
{
    uint   rule;
    uint8  follow_type;
    uint16 map_pid;
    uint   wait_time;
    Bin >> rule;
    Bin >> follow_type;
    Bin >> map_pid;
    Bin >> wait_time;

    ShowScreen( CLIENT_SCREEN_DIALOGBOX );
    DlgboxType = DIALOGBOX_FOLLOW;
    FollowRuleId = rule;
    FollowType = follow_type;
    FollowMap = map_pid;
    DlgboxWait = Timer::GameTick() + wait_time;

    // Find rule
    char       cr_name[64];
    CritterCl* cr = GetCritter( rule );
    Str::Copy( cr_name, cr ? cr->GetName() : MsgGame->GetStr( STR_FOLLOW_UNKNOWN_CRNAME ) );
    // Find map
    char map_name[64];
    Str::Copy( map_name, map_pid ? "local map" : MsgGame->GetStr( STR_FOLLOW_GMNAME ) );

    switch( FollowType )
    {
        case FOLLOW_PREP:
            Str::Copy( DlgboxText, FmtGameText( STR_FOLLOW_PREP, cr_name, map_name ) );
            break;
        case FOLLOW_FORCE:
            Str::Copy( DlgboxText, FmtGameText( STR_FOLLOW_FORCE, cr_name, map_name ) );
            break;
        default:
            WriteLogF( _FUNC_, " - Error FollowType\n" );
            Str::Copy( DlgboxText, "ERROR!" );
            break;
    }

    DlgboxButtonText[0] = MsgGame->GetStr( STR_DIALOGBOX_FOLLOW );
    DlgboxButtonText[1] = MsgGame->GetStr( STR_DIALOGBOX_CANCEL );
    DlgboxButtonsCount = 2;
}

void FOClient::Net_OnPlayersBarter()
{
    uint8 barter;
    uint  param;
    uint  param_ext;
    Bin >> barter;
    Bin >> param;
    Bin >> param_ext;

    switch( barter )
    {
        case BARTER_BEGIN:
        {
            CritterCl* cr = GetCritter( param );
            if( !cr )
                break;
            BarterText = FmtGameText( STR_BARTER_BEGIN, cr->GetName(),
                                      !FLAG( param_ext, 1 ) ? MsgGame->GetStr( STR_BARTER_OPEN_MODE ) : MsgGame->GetStr( STR_BARTER_HIDE_MODE ),
                                      !FLAG( param_ext, 2 ) ? MsgGame->GetStr( STR_BARTER_OPEN_MODE ) : MsgGame->GetStr( STR_BARTER_HIDE_MODE ) );
            BarterText += "\n";
            DlgMainTextCur = 0;
            DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, BarterText.c_str(), dialogFont);
            param_ext = (FLAG( param_ext, 2 ) ? true : false);
        }
        case BARTER_REFRESH:
        {
            CritterCl* cr = GetCritter( param );
            if( !cr )
                break;

            if( IsScreenPresent( CLIENT_SCREEN_DIALOG ) )
                HideScreen( CLIENT_SCREEN_DIALOG );
            if( !IsScreenPresent( CLIENT_SCREEN_BARTER ) )
                ShowScreen( CLIENT_SCREEN_BARTER );

            BarterIsPlayers = true;
            BarterOpponentId = param;
            BarterOpponentHide = (param_ext != 0);
            BarterOffer = false;
            BarterOpponentOffer = false;
            BarterCont2Init.clear();
            BarterCont1oInit.clear();
            BarterCont2oInit.clear();
            CollectContItems();
            break;
        }
        case BARTER_END:
        {
            if( IsScreenPlayersBarter() )
                ShowScreen( 0 );
            break;
        }
        case BARTER_TRY:
        {
            CritterCl* cr = GetCritter( param );
            if( !cr )
                break;
            ShowScreen( CLIENT_SCREEN_DIALOGBOX );
            DlgboxType = DIALOGBOX_BARTER;
            PBarterPlayerId = param;
            BarterOpponentHide = (param_ext != 0);
            PBarterHide = BarterOpponentHide;
            Str::Copy( DlgboxText, FmtGameText( STR_BARTER_DIALOGBOX, cr->GetName(), !BarterOpponentHide ? MsgGame->GetStr( STR_BARTER_OPEN_MODE ) : MsgGame->GetStr( STR_BARTER_HIDE_MODE ) ) );
            DlgboxWait = Timer::GameTick() + 20000;
            DlgboxButtonText[0] = MsgGame->GetStr( STR_DIALOGBOX_BARTER_OPEN );
            DlgboxButtonText[1] = MsgGame->GetStr( STR_DIALOGBOX_BARTER_HIDE );
            DlgboxButtonText[2] = MsgGame->GetStr( STR_DIALOGBOX_CANCEL );
            DlgboxButtonsCount = 3;
            break;
        }
        case BARTER_SET_SELF:
        case BARTER_SET_OPPONENT:
        {
            if( !IsScreenPlayersBarter() )
            {
                Net_SendPlayersBarter( BARTER_END, 0, 0 );
                break;
            }

            BarterOffer = false;
            BarterOpponentOffer = false;
            bool     is_hide = (barter == BARTER_SET_OPPONENT && BarterOpponentHide);
            ItemVec& cont = (barter == BARTER_SET_SELF ? InvContInit : BarterCont2Init);
            ItemVec& cont_o = (barter == BARTER_SET_SELF ? BarterCont1oInit : BarterCont2oInit);

            if( !is_hide )
            {
                auto it = std::find( cont.begin(), cont.end(), param );
                if( it == cont.end() || param_ext > (*it).GetCount() )
                {
                    Net_SendPlayersBarter( BARTER_REFRESH, 0, 0 );
                    break;
                }
                Item& citem = *it;
                auto  it_ = std::find( cont_o.begin(), cont_o.end(), param );
                if( it_ == cont_o.end() )
                {
                    cont_o.push_back( citem );
                    it_ = cont_o.begin() + cont_o.size() - 1;
                    (*it_).Count_Set( 0 );
                }
                (*it_).Count_Add( param_ext );
                citem.Count_Sub( param_ext );
                if( !citem.GetCount() || !citem.IsStackable() )
                    cont.erase( it );
            }
            else             // Hide
            {
                WriteLogF( _FUNC_, " - Invalid argument.\n" );
                Net_SendPlayersBarter( BARTER_END, 0, 0 );
                // See void FOClient::Net_OnPlayersBarterSetHide()
            }
            CollectContItems();
            break;
        }
        case BARTER_UNSET_SELF:
        case BARTER_UNSET_OPPONENT:
        {
            if( !IsScreenPlayersBarter() )
            {
                Net_SendPlayersBarter( BARTER_END, 0, 0 );
                break;
            }

            BarterOffer = false;
            BarterOpponentOffer = false;
            bool     is_hide = (barter == BARTER_UNSET_OPPONENT && BarterOpponentHide);
            ItemVec& cont = (barter == BARTER_UNSET_SELF ? InvContInit : BarterCont2Init);
            ItemVec& cont_o = (barter == BARTER_UNSET_SELF ? BarterCont1oInit : BarterCont2oInit);

            if( !is_hide )
            {
                auto it = std::find( cont_o.begin(), cont_o.end(), param );
                if( it == cont_o.end() || param_ext > (*it).GetCount() )
                {
                    Net_SendPlayersBarter( BARTER_REFRESH, 0, 0 );
                    break;
                }
                Item& citem = *it;
                auto  it_ = std::find( cont.begin(), cont.end(), param );
                if( it_ == cont.end() )
                {
                    cont.push_back( citem );
                    it_ = cont.begin() + cont.size() - 1;
                    (*it_).Count_Set( 0 );
                }
                (*it_).Count_Add( param_ext );
                citem.Count_Sub( param_ext );
                if( !citem.GetCount() || !citem.IsStackable() )
                    cont_o.erase( it );
            }
            else             // Hide
            {
                Item* citem = GetContainerItem( cont_o, param );
                if( !citem || param_ext > citem->GetCount() )
                {
                    Net_SendPlayersBarter( BARTER_REFRESH, 0, 0 );
                    break;
                }
                citem->Count_Sub( param_ext );
                if( !citem->GetCount() || !citem->IsStackable() )
                {
                    auto it = std::find( cont_o.begin(), cont_o.end(), param );
                    cont_o.erase( it );
                }
            }
            CollectContItems();
            break;
        }
        case BARTER_OFFER:
        {
            if( !IsScreenPlayersBarter() )
            {
                Net_SendPlayersBarter( BARTER_END, 0, 0 );
                break;
            }

            if( !param_ext )
                BarterOffer = (param != 0);
            else
                BarterOpponentOffer = (param != 0);

            if( BarterOpponentOffer && !BarterOffer )
            {
                CritterCl* cr = GetCritter( BarterOpponentId );
                if( !cr )
                    break;
                BarterText += FmtGameText( STR_BARTER_READY_OFFER, cr->GetName() );
                BarterText += "\n";
                DlgMainTextLinesReal = SprMngr.GetLinesCount( DlgWText.W(), 0, BarterText.c_str(), dialogFont);
            }
            break;
        }
        default:
            Net_SendPlayersBarter( BARTER_END, 0, 0 );
            break;
    }
}

void FOClient::Net_OnPlayersBarterSetHide()
{
    uint           id;
    uint16         pid;
    uint           count;
    Item::ItemData data;
    Bin >> id;
    Bin >> pid;
    Bin >> count;
    Bin.Pop( (char*)&data, sizeof(data) );

    if( !IsScreenPlayersBarter() )
    {
        Net_SendPlayersBarter( BARTER_END, 0, 0 );
        return;
    }

    ProtoItem* proto_item = ItemMngr.GetProtoItem( pid );
    if( !proto_item )
    {
        WriteLogF( _FUNC_, " - Proto target_item<%u> not found.\n", pid );
        Net_SendPlayersBarter( BARTER_REFRESH, 0, 0 );
        return;
    }

    Item* citem = GetContainerItem( BarterCont2oInit, id );
    if( !citem )
    {
        Item item;

        // hidden trade crash fix
        // faulty code preserved for future generations
        // memzero( &item, sizeof(item) );

        item.Init( proto_item );
        item.Id = id;
        item.Count_Set( count );
        BarterCont2oInit.push_back( item );
    }
    else
    {
        citem->Count_Add( count );
        citem->Data = data;
    }
    CollectContItems();
}

void FOClient::Net_OnShowScreen()
{
    int  screen_type;
    uint param;
    bool need_answer;
    Bin >> screen_type;
    Bin >> param;
    Bin >> need_answer;

    // Close current
    switch( screen_type )
    {
        case SHOW_SCREEN_TIMER:
        case SHOW_SCREEN_DIALOGBOX:
        case SHOW_SCREEN_SKILLBOX:
        case SHOW_SCREEN_BAG:
        case SHOW_SCREEN_SAY:
            ShowScreen( CLIENT_SCREEN_NONE );
            break;
        default:
            ShowScreen( CLIENT_SCREEN_NONE );
            break;
    }

    // Open new
    switch( screen_type )
    {
        case SHOW_SCREEN_TIMER:
            TimerStart( 0, ResMngr.GetInvAnim( param ), 0 );
            break;
        case SHOW_SCREEN_DIALOGBOX:
            ShowScreen( CLIENT_SCREEN_DIALOGBOX );
            DlgboxWait = 0;
            DlgboxText[0] = 0;
            DlgboxType = DIALOGBOX_MANUAL;
            if( param >= MAX_DLGBOX_BUTTONS )
                param = MAX_DLGBOX_BUTTONS - 1;
            DlgboxButtonsCount = param + 1;
            for( uint i = 0; i < DlgboxButtonsCount; i++ )
                DlgboxButtonText[i] = "";
            DlgboxButtonText[param] = MsgGame->GetStr( STR_DIALOGBOX_CANCEL );
            break;
        case SHOW_SCREEN_SKILLBOX:
            SboxUseOn.Clear();
            ShowScreen( CLIENT_SCREEN_SKILLBOX );
            break;
        case SHOW_SCREEN_BAG:
            UseSelect.Clear();
            ShowScreen( CLIENT_SCREEN_USE );
            break;
        case SHOW_SCREEN_SAY:
            ShowScreen( CLIENT_SCREEN_SAY );
            SayType = DIALOGSAY_TEXT;
            SayText = "";
            if( param )
                SayOnlyNumbers = true;
            break;
        case SHOW_SCREEN_ELEVATOR:
            ElevatorGenerate( param );
            break;
        // Just open
        case SHOW_SCREEN_INVENTORY:
            ShowScreen( CLIENT_SCREEN_INVENTORY );
            return;
        case SHOW_SCREEN_CHARACTER:
            ShowScreen( CLIENT_SCREEN_CHARACTER );
            return;
        case SHOW_SCREEN_FIXBOY:
            ShowScreen( CLIENT_SCREEN_FIXBOY );
            return;
        case SHOW_SCREEN_PIPBOY:
            ShowScreen( CLIENT_SCREEN_PIPBOY );
            return;
        case SHOW_SCREEN_MINIMAP:
            ShowScreen( CLIENT_SCREEN_MINIMAP );
            return;
        case SHOW_SCREEN_CLOSE:
            return;
        default:
            return;
    }

    ShowScreenType = screen_type;
    ShowScreenParam = param;
    ShowScreenNeedAnswer = need_answer;
}

void FOClient::Net_OnRunClientScript()
{
    char          str[MAX_FOTEXT];
    uint          msg_len;
    uint16        func_name_len;
    ScriptString* func_name = new ScriptString();
    int           p0, p1, p2;
    uint16        p3len;
    ScriptString* p3 = NULL;
    uint16        p4size;
    ScriptArray*  p4 = NULL;
    Bin >> msg_len;
    Bin >> func_name_len;
    if( func_name_len && func_name_len < MAX_FOTEXT )
    {
        Bin.Pop( str, func_name_len );
        str[func_name_len] = 0;
        *func_name = str;
    }
    Bin >> p0;
    Bin >> p1;
    Bin >> p2;
    Bin >> p3len;
    if( p3len )
    {
        Bin.Pop( str, p3len );
        str[p3len] = 0;
        p3 = new ScriptString( str );
    }
    Bin >> p4size;
    if( p4size )
    {
        p4 = Script::CreateArray( "int[]" );
        if( p4 )
        {
            p4->Resize( p4size );
            Bin.Pop( (char*)p4->At( 0 ), p4size * sizeof(uint) );
        }
    }

    CHECK_IN_BUFF_ERROR;

    // Reparse module
    int bind_id;
    if( Str::Substring( func_name->c_str(), "@" ) )
        bind_id = Script::Bind( func_name->c_str(), "void %s(int,int,int,string@,int[]@)", true );
    else
        bind_id = Script::Bind( "client_main", func_name->c_str(), "void %s(int,int,int,string@,int[]@)", true );

    if( bind_id > 0 && Script::PrepareContext( bind_id, _FUNC_, "Game" ) )
    {
        Script::SetArgUInt( p0 );
        Script::SetArgUInt( p1 );
        Script::SetArgUInt( p2 );
        Script::SetArgObject( p3 );
        Script::SetArgObject( p4 );
        Script::RunPrepared();
    }

    func_name->Release();
    if( p3 )
        p3->Release();
    if( p4 )
        p4->Release();
}

void FOClient::Net_OnDropTimers()
{
    ScoresNextUploadTick = 0;
    FixNextShowCraftTick = 0;
    WorldmapNextShowEntrancesTick = 0;
    WorldmapShowEntrancesLocId = 0;
}

void FOClient::Net_OnCritterLexems()
{
    uint   msg_len;
    uint   critter_id;
    uint16 lexems_len;
    char   lexems[LEXEMS_SIZE];
    Bin >> msg_len;
    Bin >> critter_id;
    Bin >> lexems_len;

    if( lexems_len + 1 >= LEXEMS_SIZE )
    {
        WriteLogF( _FUNC_, " - Invalid lexems length<%u>, disconnect.\n", lexems_len );
        IsConnected = false;
        return;
    }

    if( lexems_len )
        Bin.Pop( lexems, lexems_len );
    lexems[lexems_len] = 0;

    CHECK_IN_BUFF_ERROR;

    CritterCl* cr = GetCritter( critter_id );
    if( cr )
    {
        cr->Lexems = lexems;
        const char* look = FmtCritLook( cr, CRITTER_LOOK_NAME );
        if( look )
            cr->Name = look;
    }
}

void FOClient::Net_OnItemLexems()
{
    uint   msg_len;
    uint   item_id;
    uint16 lexems_len;
    char   lexems[LEXEMS_SIZE];
    Bin >> msg_len;
    Bin >> item_id;
    Bin >> lexems_len;

    if( lexems_len >= LEXEMS_SIZE )
    {
        WriteLogF( _FUNC_, " - Invalid lexems length<%u>, disconnect.\n", lexems_len );
        IsConnected = false;
        return;
    }

    if( lexems_len )
        Bin.Pop( lexems, lexems_len );
    lexems[lexems_len] = 0;

    CHECK_IN_BUFF_ERROR;

    // Find on map
    Item* item = GetItem( item_id );
    if( item )
        item->Lexems = lexems;
    // Find in inventory
    item = (Chosen ? Chosen->GetItem( item_id ) : NULL);
    if( item )
        item->Lexems = lexems;
    // Find in containers
    UpdateContLexems( InvContInit, item_id, lexems );
    UpdateContLexems( BarterCont1oInit, item_id, lexems );
    UpdateContLexems( BarterCont2Init, item_id, lexems );
    UpdateContLexems( BarterCont2oInit, item_id, lexems );
    UpdateContLexems( PupCont2Init, item_id, lexems );
    UpdateContLexems( InvCont, item_id, lexems );
    UpdateContLexems( BarterCont1, item_id, lexems );
    UpdateContLexems( PupCont1, item_id, lexems );
    UpdateContLexems( UseCont, item_id, lexems );
    UpdateContLexems( BarterCont1o, item_id, lexems );
    UpdateContLexems( BarterCont2, item_id, lexems );
    UpdateContLexems( BarterCont2o, item_id, lexems );
    UpdateContLexems( PupCont2, item_id, lexems );

    // Some item
    if( SomeItem.GetId() == item_id )
        SomeItem.Lexems = lexems;
}

void FOClient::Net_OnCheckUID3()
{
    CHECKUIDBIN;
    if( CHECK_UID1( uid ) )
        Net_SendPing( PING_UID_FAIL );
    //CHECK_MULTIPLY_WINDOWS7;
}

void FOClient::Net_OnMsgData()
{
    uint    msg_len;
    uint    lang;
    uint16  num_msg;
    uint    data_hash;
    CharVec data;
    Bin >> msg_len;
    Bin >> lang;
    Bin >> num_msg;
    Bin >> data_hash;
    data.resize( msg_len - (sizeof(uint) + sizeof(msg_len) + sizeof(lang) + sizeof(num_msg) + sizeof(data_hash) ) );
    Bin.Pop( (char*)&data[0], (uint)data.size() );

    CHECK_IN_BUFF_ERROR;

    if( lang != CurLang.Name )
    {
        WriteLogF( _FUNC_, " - Received text in another language, set as default.\n" );
        CurLang.Name = lang;
    }

    if( num_msg >= TEXTMSG_MAX )
    {
        WriteLogF( _FUNC_, " - Incorrect value of msg num.\n" );
        return;
    }

    if( data_hash != Crypt.Crc32( (uint8*)&data[0], (uint)data.size() ) )
    {
        WriteLogF( _FUNC_, " - Invalid hash<%s>.\n", TextMsgFileName[num_msg] );
        return;
    }

    if( CurLang.Msg[num_msg].LoadMsgStream( data ) < 0 )
    {
        WriteLogF( _FUNC_, " - Unable to load<%s> from stream.\n", TextMsgFileName[num_msg] );
        return;
    }

    CurLang.Msg[num_msg].SaveMsgFile( Str::FormatBuf( "%s\\%s", CurLang.NameStr, TextMsgFileName[num_msg] ), PATH_TEXTS );
    CurLang.Msg[num_msg].CalculateHash();

    switch( num_msg )
    {
        case TEXTMSG_ITEM:
            MrFixit.GenerateNames( *MsgGame, *MsgItem );
            break;
        case TEXTMSG_CRAFT:
            // Reload crafts
            MrFixit.Finish();
            MrFixit.LoadCrafts( *MsgCraft );
            MrFixit.GenerateNames( *MsgGame, *MsgItem );
            break;
        case TEXTMSG_INTERNAL:
            // Reload critter types
            CritType::InitFromMsg( MsgInternal );
            ResMngr.FreeResources( RES_CRITTERS );   // Free animations, maybe critters table is changed

            // Reload scripts
            if( !ReloadScripts() )
                IsConnected = false;

            // Names
            ConstantsManager::Initialize( PATH_DATA );

            // Reload interface
            if( int res = InitIface() )
            {
                WriteLog( "Init interface fail, error<%d>.\n", res );
                AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_TO_LOAD_IFACE ) );
                IsConnected = false;
            }
            break;
        default:
            break;
    }
}

void FOClient::Net_OnProtoItemData()
{
    uint         msg_len;
    uint8        type;
    uint         data_hash;
    ProtoItemVec data;
    Bin >> msg_len;
    Bin >> type;
    Bin >> data_hash;
    data.resize( (msg_len - sizeof(uint) - sizeof(msg_len) - sizeof(type) - sizeof(data_hash) ) / sizeof(ProtoItem) );
    Bin.Pop( (char*)&data[0], (uint)data.size() * sizeof(ProtoItem) );

    CHECK_IN_BUFF_ERROR;

    if( data_hash != Crypt.Crc32( (uint8*)&data[0], (uint)data.size() * sizeof(ProtoItem) ) )
    {
        WriteLogF( _FUNC_, " - Hash error.\n" );
        return;
    }

    ItemMngr.ClearProtos( type );
    ItemMngr.ParseProtos( data );

    ProtoItemVec proto_items;
    ItemMngr.GetCopyAllProtos( proto_items );
    uint         len = (uint)proto_items.size() * sizeof(ProtoItem);
    uint8*       proto_data = Crypt.Compress( (uint8*)&proto_items[0], len );
    if( !proto_data )
    {
        WriteLogF( _FUNC_, " - Compression fail.\n" );
        return;
    }
    Crypt.SetCache( "item_protos", proto_data, len );
    delete[] proto_data;

    uint count = (uint)proto_items.size();
    Crypt.SetCache( "item_protos_count", (uint8*)&count, sizeof(count) );

    // Refresh craft names
    MrFixit.GenerateNames( *MsgGame, *MsgItem );
}

void FOClient::Net_OnQuest( bool many )
{
    WriteLog( "Quest%s... ", many ? "s" : "" );

    if( many )
    {
        uint msg_len;
        uint q_count;
        Bin >> msg_len;
        Bin >> q_count;

        QuestMngr.Finish();
        if( q_count )
        {
            UIntVec quests;
            quests.resize( q_count );
            Bin.Pop( (char*)&quests[0], q_count * sizeof(uint) );
            for( uint i = 0; i < quests.size(); ++i )
                QuestMngr.OnQuest( quests[i] );
        }
    }
    else
    {
        uint q_num;
        Bin >> q_num;
        QuestMngr.OnQuest( q_num );

        // Inform player
        Quest* quest = QuestMngr.GetQuest( q_num );
		if (quest) {
			if (quest->progress != HIDDEN && quest->progress != NO_POPUP) {
				AddMess(MSGBOX_GAME, (QuestMngr.removeFormat(quest->str)).c_str());
				QuestMngr.updateQuestASCallback(quest->info, quest->str.c_str());
			}
		}
    }

    WriteLogX( "complete\n" );
}

void FOClient::Net_OnHoloInfo()
{
    uint   msg_len;
    bool   clear;
    uint16 offset;
    uint16 count;
    Bin >> msg_len;
    Bin >> clear;
    Bin >> offset;
    Bin >> count;

    if( clear )
        memzero( HoloInfo, sizeof(HoloInfo) );
    if( count )
        Bin.Pop( (char*)&HoloInfo[offset], count * sizeof(uint) );
}

void FOClient::Net_OnScores()
{
    Bin.Pop( &BestScores[0][0], SCORE_NAME_LEN * SCORES_MAX );
    for( int i = 0; i < SCORES_MAX; i++ )
        BestScores[i][SCORE_NAME_LEN - 1] = '\0';
}

void FOClient::Net_OnUserHoloStr()
{
    uint   msg_len;
    uint   str_num;
    uint16 text_len;
    char   text[USER_HOLO_MAX_LEN + 1];
    Bin >> msg_len;
    Bin >> str_num;
    Bin >> text_len;

    if( text_len > USER_HOLO_MAX_LEN )
    {
        WriteLogF( _FUNC_, " - Text length greater than maximum, cur<%u>, max<%u>. Disconnect.\n", text_len, USER_HOLO_MAX_LEN );
        IsConnected = false;
        return;
    }

    Bin.Pop( text, text_len );
    text[text_len] = '\0';

    CHECK_IN_BUFF_ERROR;

    if( MsgUserHolo->Count( str_num ) )
        MsgUserHolo->EraseStr( str_num );
    MsgUserHolo->AddStr( str_num, text );
    MsgUserHolo->SaveMsgFile( USER_HOLO_TEXTMSG_FILE, PATH_TEXTS );
}

void FOClient::Net_OnAutomapsInfo()
{
    uint   msg_len;
    bool   clear;
    uint16 locs_count;
    Bin >> msg_len;
    Bin >> clear;

    if( clear )
    {
        Automaps.clear();
        AutomapWaitPids.clear();
        AutomapReceivedPids.clear();
        AutomapPoints.clear();
        AutomapCurMapPid = 0;
        AutomapScrX = 0.0f;
        AutomapScrY = 0.0f;
        AutomapZoom = 1.0f;
    }

    Bin >> locs_count;
    for( uint16 i = 0; i < locs_count; i++ )
    {
        uint   loc_id;
        uint16 loc_pid;
        uint16 maps_count;
        Bin >> loc_id;
        Bin >> loc_pid;
        Bin >> maps_count;

        auto it = std::find( Automaps.begin(), Automaps.end(), loc_id );

        // Delete from collection
        if( !maps_count )
        {
            if( it != Automaps.end() )
                Automaps.erase( it );
        }
        // Add or modify
        else
        {
            Automap amap;
            amap.LocId = loc_id;
            amap.LocPid = loc_pid;
            amap.LocName = MsgGM->GetStr( STR_GM_NAME_( loc_pid ) );

            for( uint16 j = 0; j < maps_count; j++ )
            {
                uint16 map_pid;
                Bin >> map_pid;

                amap.MapPids.push_back( map_pid );
                amap.MapNames.push_back( MsgGM->GetStr( STR_MAP_NAME_( map_pid ) ) );
            }

            if( it != Automaps.end() )
                *it = amap;
            else
                Automaps.push_back( amap );
        }
    }

    CHECK_IN_BUFF_ERROR;
}

void FOClient::Net_OnCheckUID4()
{
    CHECKUIDBIN;
    if( CHECK_UID2( uid ) )
        Net_SendPing( PING_UID_FAIL );
}

void FOClient::Net_OnViewMap()
{
    uint16 hx, hy;
    uint   loc_id, loc_ent;
    Bin >> hx;
    Bin >> hy;
    Bin >> loc_id;
    Bin >> loc_ent;

    if( !HexMngr.IsMapLoaded() )
        return;

    HexMngr.FindSetCenter( hx, hy );
    SndMngr.PlayAmbient( MsgGM->GetStr( STR_MAP_AMBIENT_( HexMngr.GetCurPidMap() ) ) );
    ShowMainScreen( CLIENT_MAIN_SCREEN_GAME );
    ScreenFadeOut();
    HexMngr.RebuildLight();
    ShowScreen( CLIENT_SCREEN_WM_TOWNVIEW );

    if( loc_id )
    {
        TViewType = TOWN_VIEW_FROM_GLOBAL;
        TViewGmapLocId = loc_id;
        TViewGmapLocEntrance = loc_ent;
    }
}

void FOClient::Net_OnCraftAsk()
{
    uint   msg_len;
    uint16 count;
    Bin >> msg_len;
    Bin >> count;

    FixShowCraft.clear();
    for( int i = 0; i < count; i++ )
    {
        uint craft_num;
        Bin >> craft_num;
        FixShowCraft.insert( craft_num );
    }

    CHECK_IN_BUFF_ERROR;

    if( IsScreenPresent( CLIENT_SCREEN_FIXBOY ) && FixMode == FIX_MODE_LIST )
        FixGenerate( FIX_MODE_LIST );
}

void FOClient::Net_OnCraftResult()
{
    uint8 craft_result;
    Bin >> craft_result;

    if( craft_result != CRAFT_RESULT_NONE )
    {
        FixResult = craft_result;
        FixGenerate( FIX_MODE_RESULT );
    }
}

void FOClient::SetGameColor( uint color )
{
    SprMngr.SetSpritesColor( color );
    HexMngr.RefreshMap();
}

bool FOClient::IsCurInRect( const Rect& rect, int ax, int ay )
{
    return !rect.IsZero() && (GameOpt.MouseX >= rect.L + ax && GameOpt.MouseY >= rect.T + ay && GameOpt.MouseX <= rect.R + ax && GameOpt.MouseY <= rect.B + ay);
}

bool FOClient::IsCurInRect( const Rect& rect )
{
    return !rect.IsZero() && (GameOpt.MouseX >= rect.L && GameOpt.MouseY >= rect.T && GameOpt.MouseX <= rect.R && GameOpt.MouseY <= rect.B);
}

bool FOClient::IsCurInRectNoTransp( uint spr_id, Rect& rect, int ax, int ay )
{
    return IsCurInRect( rect, ax, ay ) && SprMngr.IsPixNoTransp( spr_id, GameOpt.MouseX - rect.L - ax, GameOpt.MouseY - rect.T - ay, false );
}

bool FOClient::IsCurInInterface()
{
    if( IntVisible && IsCurInRectNoTransp( IntMainPic->GetCurSprId(), IntWMain, 0, 0 ) )
        return true;
    if( IntVisible && IntAddMess && IsCurInRectNoTransp( IntPWAddMess->GetCurSprId(), IntWAddMess, 0, 0 ) )
        return true;
    // if( ConsoleActive && IsCurInRectNoTransp( ConsolePic, Main, 0, 0 ) ) // Todo: need check console?
    return false;
}

bool FOClient::GetCurHex( uint16& hx, uint16& hy, bool ignore_interface )
{
    hx = hy = 0;
    if( !ignore_interface && IsCurInInterface() )
        return false;
    return HexMngr.GetHexPixel( GameOpt.MouseX, GameOpt.MouseY, hx, hy );
}

bool FOClient::RegCheckData( CritterCl* newcr )
{
    // Name
    uint name_len_utf8 = Str::LengthUTF8( newcr->Name.c_str() );
    if( name_len_utf8 < MIN_NAME || name_len_utf8 < GameOpt.MinNameLength ||
        name_len_utf8 > MAX_NAME || name_len_utf8 > GameOpt.MaxNameLength )
    {
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_WRONG_NAME ) );
        return false;
    }

    if( !Str::IsValidUTF8( newcr->Name.c_str() ) || Str::Substring( newcr->Name.c_str(), "*" ) )
    {
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_NAME_WRONG_CHARS ) );
        return false;
    }

    // Password
    if( !Singleplayer )
    {
        uint pass_len_utf8 = Str::LengthUTF8( newcr->Pass.c_str() );
        if( pass_len_utf8 < MIN_NAME || pass_len_utf8 < GameOpt.MinNameLength ||
            pass_len_utf8 > MAX_NAME || pass_len_utf8 > GameOpt.MaxNameLength )
        {
            AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_WRONG_PASS ) );
            return false;
        }

        if( !Str::IsValidUTF8( newcr->Pass.c_str() ) || Str::Substring( newcr->Pass.c_str(), "*" ) )
        {
            AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_PASS_WRONG_CHARS ) );
            return false;
        }
    }

    if( Script::PrepareContext( ClientFunctions.PlayerGenerationCheck, _FUNC_, "Registration" ) )
    {
        ScriptString* name = new ScriptString( newcr->Name );
        ScriptArray*  arr = Script::CreateArray( "int[]" );

        arr->Resize( MAX_PARAMS );
        for( int i = 0; i < MAX_PARAMS; i++ )
            (*(int*)arr->At( i ) ) = newcr->ParamsReg[i];

        bool result = false;
        Script::SetArgObject( name );
        Script::SetArgObject( arr );
        if( Script::RunPrepared() )
            result = Script::GetReturnedBool();
        name->Release();

        if( !result )
        {
            arr->Release();
            return false;
        }

        if( arr->GetSize() == MAX_PARAMS )
            for( int i = 0; i < MAX_PARAMS; i++ )
                newcr->ParamsReg[i] = (*(int*)arr->At( i ) );
        arr->Release();
    }

    return true;
}

void FOClient::SetDayTime( bool refresh )
{
    if( !HexMngr.IsMapLoaded() )
        return;

    static uint old_color = -1;
    uint        color = GetColorDay( HexMngr.GetMapDayTime(), HexMngr.GetMapDayColor(), HexMngr.GetMapTime(), NULL );
    color = COLOR_GAME_RGB( (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF );

    if( refresh )
        old_color = -1;
    if( old_color != color )
    {
        old_color = color;
        SetGameColor( old_color );
    }
}

Item* FOClient::GetTargetContItem()
{
    static Item inv_slot;
    #define TRY_SEARCH_IN_CONT( cont )                                                        \
        do { auto it = std::find( cont.begin(), cont.end(), item_id ); if( it != cont.end() ) \
                 return &(*it);                                                               \
        }                                                                                     \
        while( 0 )
    #define TRY_SEARCH_IN_SLOT( target_item )    do { if( target_item->GetId() == item_id ) { inv_slot = *target_item; return &inv_slot; } } while( 0 )

    if( !TargetSmth.IsContItem() )
        return NULL;
    uint item_id = TargetSmth.GetId();

    if( GetActiveScreen() != CLIENT_SCREEN_NONE )
    {
        switch( GetActiveScreen() )
        {
            case CLIENT_SCREEN_USE:
                TRY_SEARCH_IN_CONT( UseCont );
                break;
            case CLIENT_SCREEN_INVENTORY:
                TRY_SEARCH_IN_CONT( InvCont );
                if( Chosen )
                {
                    Item* item = Chosen->GetItem( item_id );
                    if( item )
                        TRY_SEARCH_IN_SLOT( item );
                }
                break;
            case CLIENT_SCREEN_PICKUP:
                TRY_SEARCH_IN_CONT( PupCont1 );
                TRY_SEARCH_IN_CONT( PupCont2 );
                break;
            case CLIENT_SCREEN_BARTER:
                if( TargetSmth.GetParam() == 0 )
                    TRY_SEARCH_IN_CONT( BarterCont1 );
                if( TargetSmth.GetParam() == 0 )
                    TRY_SEARCH_IN_CONT( BarterCont2 );
                if( TargetSmth.GetParam() == 1 )
                    TRY_SEARCH_IN_CONT( BarterCont1o );
                if( TargetSmth.GetParam() == 1 )
                    TRY_SEARCH_IN_CONT( BarterCont2o );
                break;
            default:
                break;
        }
    }
    TargetSmth.Clear();
    return NULL;
}

void FOClient::AddAction( bool to_front, ActionEvent act )
{
    for( auto it = ChosenAction.begin(); it != ChosenAction.end();)
    {
        ActionEvent& a = *it;
        if( a == act )
            it = ChosenAction.erase( it );
        else
            ++it;
    }

    if( to_front )
        ChosenAction.insert( ChosenAction.begin(), act );
    else
        ChosenAction.push_back( act );
}

void FOClient::SetAction( uint type_action, uint param0, uint param1, uint param2, uint param3, uint param4, uint param5 )
{
	//	temp fix, so if right click move is used, the last - not up to date pos - would not show.
	//	TODO: complete code so even on these actions, the position is updated
	if (!LegacyMouseCursor) {
		if (type_action == CHOSEN_MOVE_TO_CRITTER ||
			type_action == CHOSEN_USE_SKILL_ON_CRITTER ||
			type_action == CHOSEN_USE_SKILL_ON_ITEM ||
			type_action == CHOSEN_USE_SKILL_ON_SCENERY ||
			type_action == CHOSEN_TALK_NPC ||
			type_action == CHOSEN_PICK_ITEM ||
			type_action == CHOSEN_PICK_CRITTER)
		{
			HideSmartCursorLastPosition();
		}
	}
    SetAction( ActionEvent( type_action, param0, param1, param2, param3, param4, param5 ) );
}

void FOClient::SetAction( ActionEvent act )
{
    ChosenAction.clear();
    AddActionBack( act );
}

void FOClient::AddActionBack( uint type_action, uint param0, uint param1, uint param2, uint param3, uint param4, uint param5 )
{
    AddActionBack( ActionEvent( type_action, param0, param1, param2, param3, param4, param5 ) );
}

void FOClient::AddActionBack( ActionEvent act )
{
    AddAction( false, act );
}

void FOClient::AddActionFront( uint type_action, uint param0, uint param1, uint param2, uint param3, uint param4, uint param5 )
{
    AddAction( true, ActionEvent( type_action, param0, param1, param2, param3, param4, param5 ) );
}

void FOClient::EraseFrontAction()
{
    if( ChosenAction.size() )
        ChosenAction.erase( ChosenAction.begin() );
}

void FOClient::EraseBackAction()
{
    if( ChosenAction.size() )
        ChosenAction.pop_back();
}

bool FOClient::IsAction( uint type_action )
{
    return ChosenAction.size() && (*ChosenAction.begin() ).Type == type_action;
}

void FOClient::ChosenChangeSlot()
{
    if( !Chosen )
        return;

    if( Chosen->ItemSlotExt->GetId() )
        AddActionBack( CHOSEN_MOVE_ITEM, Chosen->ItemSlotExt->GetId(), Chosen->ItemSlotExt->GetCount(), SLOT_HAND1 );
    else if( Chosen->ItemSlotMain->GetId() )
        AddActionBack( CHOSEN_MOVE_ITEM, Chosen->ItemSlotMain->GetId(), Chosen->ItemSlotMain->GetCount(), SLOT_HAND2 );
    else
    {
        uint8      tree = Chosen->DefItemSlotHand->Proto->Weapon_UnarmedTree + 1;
        ProtoItem* unarmed = Chosen->GetUnarmedItem( tree, 0 );
        if( !unarmed )
            unarmed = Chosen->GetUnarmedItem( 0, 0 );
        Chosen->DefItemSlotHand->Init( unarmed );
    }
}

void FOClient::WaitPing()
{
    SetCurMode( CURSOR_WAIT );
    Net_SendPing( PING_WAIT );
}

void FOClient::CrittersProcess()
{
/************************************************************************/
/* All critters                                                         */
/************************************************************************/
    for( auto it = HexMngr.GetCritters().begin(); it != HexMngr.GetCritters().end();)
    {
        CritterCl* crit = (*it).second;
        ++it;

        crit->Process();

        if( crit->IsNeedReSet() )
        {
            HexMngr.RemoveCrit( crit );
            HexMngr.SetCrit( crit );
            crit->ReSetOk();
        }

        if( !crit->IsChosen() && crit->IsNeedMove() && HexMngr.TransitCritter( crit, crit->MoveSteps[0].first, crit->MoveSteps[0].second, true, crit->CurMoveStep > 0 ) )
        {
            crit->MoveSteps.erase( crit->MoveSteps.begin() );
            crit->CurMoveStep--;
        }

        if( crit->IsFinish() )
        {
            EraseCritter( crit->GetId() );
        }
    }

/************************************************************************/
/* Chosen                                                               */
/************************************************************************/
    if( !Chosen )
        return;

    if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) )
    {
        // Timeout mode
        if( !Chosen->GetParam( TO_BATTLE ) && AnimGetCurSprCnt( IntWCombatAnim ) > 0 )
        {
            AnimRun( IntWCombatAnim, ANIMRUN_FROM_END );
            if( AnimGetCurSprCnt( IntWCombatAnim ) == AnimGetSprCount( IntWCombatAnim ) - 1 )
                SndMngr.PlaySound( SOUND_COMBAT_MODE_OFF );
        }

        // Roof Visible
        static int last_hx = 0;
        static int last_hy = 0;

        if( last_hx != Chosen->GetHexX() || last_hy != Chosen->GetHexY() )
        {
            last_hx = Chosen->GetHexX();
            last_hy = Chosen->GetHexY();
            HexMngr.SetSkipRoof( last_hx, last_hy );
        }

        // Hide Mode
        if( Chosen->IsRawParam( MODE_HIDE ) )
            Chosen->Alpha = 0x82;
        else
            Chosen->Alpha = 0xFF;
    }

	if (Chosen->IsWalkAnim()) {
		//WriteLog("FOClient::CrittersProcess - WALK ANIM \tap<%u.%u> - gameTick = %u, apRegenTick = %u, gameOpt.ApRegen = %u\n", Chosen->Params[ST_CURRENT_AP] / AP_DIVIDER, Chosen->Params[ST_CURRENT_AP] % AP_DIVIDER, Timer::GameTick(), Chosen->ApRegenerationTick, GameOpt.ApRegeneration);
		// Ap regeneration while running or walking
		if (Chosen->GetParam(ST_CURRENT_AP) < Chosen->GetParam(ST_ACTION_POINTS) && !IsTurnBased)
		{
			uint tick = Timer::GameTick();
			if (!Chosen->ApRegenerationTick)
				Chosen->ApRegenerationTick = tick;
			else
			{
				uint delta = tick - Chosen->ApRegenerationTick;
				if (delta >= 100)
				{
					int regenFactor = 0;
					if (Chosen->IsRunning) {
						regenFactor = CLAMP((Chosen->GetParam(ST_AP_REGEN_RATE) / 100) % 100, 0, 100);
					}
					else {
						regenFactor = CLAMP(Chosen->GetParam(ST_AP_REGEN_RATE) % 100, 0, 100);
					}
					if (regenFactor > 0) {
						int max_ap = Chosen->GetParam(ST_ACTION_POINTS) * AP_DIVIDER;
						Chosen->Params[ST_CURRENT_AP] += regenFactor * max_ap * delta / GameOpt.ApRegeneration / 100;
						if (Chosen->Params[ST_CURRENT_AP] > max_ap)
							Chosen->Params[ST_CURRENT_AP] = max_ap;
					}
					Chosen->ApRegenerationTick = tick;
					//WriteLog("FOClient::CrittersProcess - REGEN TICKED WALK\tap<%u.%u> - gameTick = %u, apRegenTick = %u, gameOpt.ApRegen = %u, regenFactor = %u\n", Chosen->Params[ST_CURRENT_AP] / AP_DIVIDER, Chosen->Params[ST_CURRENT_AP] % AP_DIVIDER, Timer::GameTick(), Chosen->ApRegenerationTick, GameOpt.ApRegeneration, regenFactor);
				}
			}
		}
		if (Chosen->GetParam(ST_CURRENT_AP) > Chosen->GetParam(ST_ACTION_POINTS))
			Chosen->Params[ST_CURRENT_AP] = Chosen->GetParam(ST_ACTION_POINTS) * AP_DIVIDER;
	}

	/*
	//	use item AP regen not implemented yet, do to useful design, one can just reduce the AP cost and have similar effect, ex: Mutant mining costs no AP
	if (Chosen->IsAnim()) {
		WriteLog("FOClient::CrittersProcess - USE ITEM ANIM \tap<%u.%u> - gameTick = %u, apRegenTick = %u, gameOpt.ApRegen = %u\n", Chosen->Params[ST_CURRENT_AP] / AP_DIVIDER, Chosen->Params[ST_CURRENT_AP] % AP_DIVIDER, Timer::GameTick(), Chosen->ApRegenerationTick, GameOpt.ApRegeneration);
	}*/

	// Actions
	if (!Chosen->IsFree()) {
		return;
	}

	if (Chosen->IsWalkAnim()) {
		WriteLog("FOClient::CrittersProcess - NEVER HAPPENS");
	}

    // Game pause
    if( Timer::IsGamePaused() )
        return;

    // Ap regeneration
    if( Chosen->GetParam( ST_CURRENT_AP ) < Chosen->GetParam( ST_ACTION_POINTS ) && !IsTurnBased )
    {
        uint tick = Timer::GameTick();
        if( !Chosen->ApRegenerationTick )
            Chosen->ApRegenerationTick = tick;
        else
        {
            uint delta = tick - Chosen->ApRegenerationTick;
            if( delta >= 500 )
            {
                int max_ap = Chosen->GetParam( ST_ACTION_POINTS ) * AP_DIVIDER;
                Chosen->Params[ST_CURRENT_AP] += max_ap * delta / GameOpt.ApRegeneration;
                if( Chosen->Params[ST_CURRENT_AP] > max_ap )
                    Chosen->Params[ST_CURRENT_AP] = max_ap;
                Chosen->ApRegenerationTick = tick;
            }
        }
    }
    if( Chosen->GetParam( ST_CURRENT_AP ) > Chosen->GetParam( ST_ACTION_POINTS ) )
        Chosen->Params[ST_CURRENT_AP] = Chosen->GetParam( ST_ACTION_POINTS ) * AP_DIVIDER;

    if( ChosenAction.empty() )
        return;

    if( !Chosen->IsLife() || (IsTurnBased && !IsTurnBasedMyTurn() ) )
    {
        ChosenAction.clear();
        Chosen->AnimateStay();
        return;
    }

    ActionEvent act = ChosenAction[0];
    #define CHECK_NEED_AP( need_ap )                                                                                                                                                                                                                                                                                                     \
        { if( IsTurnBased && !IsTurnBasedMyTurn() )                                                                                                                                                                                                                                                                                      \
              break; if( Chosen->GetParam( ST_ACTION_POINTS ) < (int)(need_ap) ) { AddMess( MSGBOX_GAME, FmtCombatText( STR_COMBAT_NEED_AP, need_ap ) ); break; } if( Chosen->GetParam( ST_CURRENT_AP ) < (int)(need_ap) ) { if( IsTurnBased ) { if( Chosen->GetParam( ST_CURRENT_AP ) )                                                 \
                                                                                                                                                                                                                                                     AddMess( MSGBOX_GAME, FmtCombatText( STR_COMBAT_NEED_AP, need_ap ) ); break; } else \
                                                                                                                                                                                                                                 return; }                                                                                               \
        }
    #define CHECK_NEED_REAL_AP( need_ap )                                                                                                                                                                                                                                                                                                                             \
        { if( IsTurnBased && !IsTurnBasedMyTurn() )                                                                                                                                                                                                                                                                                                                   \
              break; if( Chosen->GetParam( ST_ACTION_POINTS ) * AP_DIVIDER < (int)(need_ap) ) { AddMess( MSGBOX_GAME, FmtCombatText( STR_COMBAT_NEED_AP, (need_ap) / AP_DIVIDER ) ); break; } if( Chosen->GetRealAp() < (int)(need_ap) ) { if( IsTurnBased ) { if( Chosen->GetRealAp() )                                                                              \
                                                                                                                                                                                                                                                                   AddMess( MSGBOX_GAME, FmtCombatText( STR_COMBAT_NEED_AP, (need_ap) / AP_DIVIDER ) ); break; } else \
                                                                                                                                                                                                                                               return; } }

    // Force end move
    if( act.Type != CHOSEN_MOVE && act.Type != CHOSEN_MOVE_TO_CRITTER && MoveDirs.size() )
    {
        MoveLastHx = -1;
        MoveLastHy = -1;
        MoveDirs.clear();
        Chosen->MoveSteps.clear();
        if( !Chosen->IsAnim() )
            Chosen->AnimateStay();
        Net_SendMove( MoveDirs );
    }

    switch( act.Type )
    {
        case CHOSEN_NONE:
            break;
        case CHOSEN_MOVE_TO_CRITTER:
        case CHOSEN_MOVE:
        {
            uint16 hx = act.Param[0];
            uint16 hy = act.Param[1];
            bool   is_run = act.Param[2] != 0;
            int    cut = act.Param[3];
            bool   wait_click = act.Param[4] != 0;
            uint   start_tick = act.Param[5];

            uint16 from_hx = 0;
            uint16 from_hy = 0;
            int    ap_cost_real = 0;
            bool   skip_find = false;

            if( !HexMngr.IsMapLoaded() )
                break;

            if( act.Type == CHOSEN_MOVE_TO_CRITTER )
            {
                CritterCl* cr = GetCritter( act.Param[0] );
                if( !cr )
                    goto label_EndMove;
                hx = cr->GetHexX();
                hy = cr->GetHexY();
            }

            from_hx = Chosen->GetHexX();
            from_hy = Chosen->GetHexY();

            if( Chosen->IsDoubleOverweight() )
            {
                AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_OVERWEIGHT_TITLE ) );
                SetAction( CHOSEN_NONE );
                goto label_EndMove;
            }

            if( CheckDist( from_hx, from_hy, hx, hy, cut ) )
                goto label_EndMove;

            if( !GameOpt.RunOnCombat && Chosen->GetParam( TO_BATTLE ) )
                is_run = false;
            else if( !GameOpt.RunOnTransfer && Chosen->GetParam( TO_TRANSFER ) )
                is_run = false;
            else if( Chosen->IsDmgLeg() || Chosen->IsOverweight() )
                is_run = false;
            else if( wait_click && Timer::GameTick() - start_tick < (GameOpt.DoubleClickTime ? GameOpt.DoubleClickTime : GetDoubleClickTicks() ) )
                return;
            else if( is_run && !IsTurnBased && Chosen->GetApCostCritterMove( is_run ) > 0 && Chosen->GetRealAp() < (GameOpt.RunModMul * Chosen->GetParam( ST_ACTION_POINTS ) * AP_DIVIDER) / GameOpt.RunModDiv + GameOpt.RunModAdd )
                is_run = false;

            if( is_run && !CritType::IsCanRun( Chosen->GetCrType() ) )
                is_run = false;
            if( is_run && Chosen->IsRawParam( MODE_NO_RUN ) )
                is_run = false;

            if( !is_run && (!CritType::IsCanWalk( Chosen->GetCrType() ) || Chosen->IsRawParam( MODE_NO_WALK ) ) )
            {
                AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_CRITTER_CANT_MOVE ) );
                SetAction( CHOSEN_NONE );
                goto label_EndMove;
            }

            ap_cost_real = Chosen->GetApCostCritterMove( is_run );
            if( !(IsTurnBasedMyTurn() && Chosen->GetAllAp() >= ap_cost_real / AP_DIVIDER) )
                CHECK_NEED_REAL_AP( ap_cost_real );
            Chosen->IsRunning = is_run;

            if( hx == MoveLastHx && hy == MoveLastHy && MoveDirs.size() )
            {
                uint16 hx_ = from_hx;
                uint16 hy_ = from_hy;
                MoveHexByDir( hx_, hy_, MoveDirs[0], HexMngr.GetMaxHexX(), HexMngr.GetMaxHexY() );
                if( !HexMngr.GetField( hx_, hy_ ).IsNotPassed )
                    skip_find = true;
            }

            // Find steps
            if( !skip_find )
            {
                // MoveDirs.resize(min(MoveDirs.size(),4)); // TODO:
                MoveDirs.clear();
                if( !IsTurnBased && MoveDirs.size() )
                {
                    uint16 hx_ = from_hx;
                    uint16 hy_ = from_hy;
                    MoveHexByDir( hx_, hy_, MoveDirs[0], HexMngr.GetMaxHexX(), HexMngr.GetMaxHexY() );
                    if( !HexMngr.GetField( hx_, hy_ ).IsNotPassed )
                    {
                        for( uint i = 1; i < MoveDirs.size() && i < 4; i++ )
                            MoveHexByDir( hx_, hy_, MoveDirs[i], HexMngr.GetMaxHexX(), HexMngr.GetMaxHexY() );
                        from_hx = hx_;
                        from_hy = hy_;
                    }
                    else
                        MoveDirs.clear();
                }
                else
                    MoveDirs.clear();

                uint16 hx_ = hx;
                uint16 hy_ = hy;
                bool   result = HexMngr.CutPath( Chosen, from_hx, from_hy, hx_, hy_, cut );
                if( !result )
                {
                    int max_cut = (int)DistGame( from_hx, from_hy, hx_, hy_ );
                    if( max_cut > cut + 1 )
                        result = HexMngr.CutPath( Chosen, from_hx, from_hy, hx_, hy_, ++cut );
                    if( !result )
                        goto label_EndMove;
                }

                ChosenAction[0].Param[3] = cut;
                UInt8Vec steps;
                if( HexMngr.FindPath( Chosen, from_hx, from_hy, hx_, hy_, steps, -1 ) )
                {
                    for( uint i = 0, j = (uint)steps.size(); i < j; i++ )
                        MoveDirs.push_back( steps[i] );
                }
            }

label_EndMove:
            if( IsTurnBased && ap_cost_real > 0 && (int)MoveDirs.size() / (ap_cost_real / AP_DIVIDER) > Chosen->GetParam( ST_CURRENT_AP ) + Chosen->GetParam( ST_MOVE_AP ) )
                MoveDirs.resize( Chosen->GetParam( ST_CURRENT_AP ) + Chosen->GetParam( ST_MOVE_AP ) );
            if( !IsTurnBased && ap_cost_real > 0 && (int)MoveDirs.size() > Chosen->GetRealAp() / ap_cost_real )
                MoveDirs.resize( Chosen->GetRealAp() / ap_cost_real );
            if( MoveDirs.size() > 1 && Chosen->IsDmgTwoLeg() )
                MoveDirs.resize( 1 );

            // Transit
            if( MoveDirs.size() )
            {
                uint16 hx_ = Chosen->GetHexX();
                uint16 hy_ = Chosen->GetHexY();
                MoveHexByDir( hx_, hy_, MoveDirs[0], HexMngr.GetMaxHexX(), HexMngr.GetMaxHexY() );
                HexMngr.TransitCritter( Chosen, hx_, hy_, true, false );
                if( IsTurnBased )
                {
                    int ap_cost = ap_cost_real / AP_DIVIDER;
                    int move_ap = Chosen->GetParam( ST_MOVE_AP );
                    if( move_ap )
                    {
                        if( ap_cost > move_ap )
                        {
                            Chosen->SubMoveAp( move_ap );
                            Chosen->SubAp( ap_cost - move_ap );
                        }
                        else
                            Chosen->SubMoveAp( ap_cost );
                    }
                    else
                    {
                        Chosen->SubAp( ap_cost );
                    }
                }
                else if( Chosen->GetParam( TO_BATTLE ) )
                {
                    Chosen->Params[ST_CURRENT_AP] -= ap_cost_real;
                }
				if (!Chosen->IsWalkAnim()) {
					WriteLog("FOClient::CrittersProcess - Not walk or run action, okay to set to 0\n");
					Chosen->ApRegenerationTick = 0;
				}
				//    To check if smart cursor is activated, if so, may not redraw to keep move-hex outline on clicked position.
				if (!LegacyMouseCursor && !SmartMouseLastClick)
					HexMngr.SetCursorPos(GameOpt.MouseX, GameOpt.MouseY, Keyb::CtrlDwn, true);
                RebuildLookBorders = true;
            }

            // Send about move
            Net_SendMove( MoveDirs );
            if( MoveDirs.size() )
                MoveDirs.erase( MoveDirs.begin() );

            // End move
            if( MoveDirs.empty() )
            {
                MoveLastHx = -1;
                MoveLastHy = -1;
                Chosen->MoveSteps.clear();
                if( !Chosen->IsAnim() )
                    Chosen->AnimateStay();
            }
            // Continue move
            else
            {
                MoveLastHx = hx;
                MoveLastHy = hy;
                Chosen->MoveSteps.resize( 1 );
                return;
            }
            break;
        }
        case CHOSEN_DIR:
        {
            int cw = act.Param[0];

            if( !HexMngr.IsMapLoaded() || (IsTurnBased && !IsTurnBasedMyTurn() ) )
                break;

            int dir = Chosen->GetDir();
            if( cw == 0 )
            {
                dir++;
                if( dir >= DIRS_COUNT )
                    dir = 0;
            }
            else
            {
                dir--;
                if( dir < 0 )
                    dir = DIRS_COUNT - 1;
            }
            Chosen->SetDir( dir );
            Net_SendDir();
            break;
        }
        case CHOSEN_USE_ITEM:
        {
            uint   item_id = act.Param[0];
            uint16 item_pid = act.Param[1];
            uint8  target_type = act.Param[2];
            uint   target_id = act.Param[3];
            uint8  rate = act.Param[4];
            uint   param = act.Param[5];
            uint8  use = (rate & 0xF);
            uint8  aim = (rate >> 4);

            // Find item
            Item* item = (item_id ? Chosen->GetItem( item_id ) : Chosen->DefItemSlotHand);
            if( !item )
                break;
            bool is_main_item = (item == Chosen->ItemSlotMain);

            // Check proto item
            if( !item_pid )
                item_pid = item->GetProtoId();
            ProtoItem* proto_item = ItemMngr.GetProtoItem( item_pid );
            if( !proto_item )
                break;
            if( item->GetProtoId() != item_pid )         // Unarmed proto
            {
                static Item      temp_item;
                static ProtoItem temp_proto_item;
                temp_item = *item;
                temp_proto_item = *proto_item;
                item = &temp_item;
                proto_item = &temp_proto_item;
                item->Init( proto_item );
            }

            // Find target
            CritterCl* target_cr = NULL;
            ItemHex*   target_item = NULL;
            Item*      target_item_self = NULL;
            if( target_type == TARGET_SELF )
                target_cr = Chosen;
            else if( target_type == TARGET_SELF_ITEM )
                target_item_self = Chosen->GetItem( target_id );
            else if( target_type == TARGET_CRITTER )
                target_cr = GetCritter( target_id );
            else if( target_type == TARGET_ITEM )
                target_item = GetItem( target_id );
            else if( target_type == TARGET_SCENERY )
                target_item = GetItem( target_id );
            else
                break;
            if( target_type == TARGET_CRITTER && Chosen == target_cr )
                target_type = TARGET_SELF;
            if( target_type == TARGET_SELF )
                target_id = Chosen->GetId();

            // Check
            if( target_type == TARGET_CRITTER && !target_cr )
                break;
            else if( target_type == TARGET_ITEM && !target_item )
                break;
            else if( target_type == TARGET_SCENERY && !target_item )
                break;
            if( target_type != TARGET_CRITTER && !item->GetId() )
                break;

            // Parse use
            bool is_attack = (target_type == TARGET_CRITTER && is_main_item && item->IsWeapon() && use < USE_MAX);
            bool is_reload = (target_type == TARGET_SELF_ITEM && use == USE_RELOAD && item->IsWeapon() );
            bool is_self = (target_type == TARGET_SELF || target_type == TARGET_SELF_ITEM);
            if( !is_attack && !is_reload && (IsCurMode( CURSOR_USE_ITEM ) || IsCurMode( CURSOR_USE_WEAPON ) ) )
                SetCurMode( CURSOR_DEFAULT );

            // Aim overriding
            if( is_attack && ClientFunctions.HitAim && Script::PrepareContext( ClientFunctions.HitAim, _FUNC_, "Game" ) )
            {
                uint8 new_aim = aim;
                Script::SetArgAddress( &new_aim );
                if( Script::RunPrepared() && new_aim != aim )
                {
                    aim = new_aim;
                    rate = MAKE_ITEM_MODE( use, aim );
                }
            }

            // Calculate ap cost
            int ap_cost = Chosen->GetUseApCost( item, rate );

            // Check weapon
            if( is_attack )
            {
                if( !HexMngr.IsMapLoaded() )
                    break;
                if( is_self )
                    break;
                if( !item->IsWeapon() )
                    break;
                if( target_cr->IsDead() )
                    break;
                if( !Chosen->IsRawParam( MODE_UNLIMITED_AMMO ) && item->WeapGetMaxAmmoCount() && item->WeapIsEmpty() )
                {
                    SndMngr.PlaySoundType( 'W', 'O', item->Proto->Weapon_SoundId[use], '1' );
                    break;
                }
                if( item->IsTwoHands() && Chosen->IsDmgArm() )
                {
                    AddMess( MSGBOX_GAME, FmtCombatText( STR_COMBAT_NEED_DMG_ARM ) );
                    break;
                }
                if( Chosen->IsDmgTwoArm() && item->GetId() )
                {
                    AddMess( MSGBOX_GAME, FmtCombatText( STR_COMBAT_NEED_DMG_TWO_ARMS ) );
                    break;
                }
                if( item->IsDeteriorable() && item->IsBroken() )
                {
                    AddMess( MSGBOX_GAME, FmtGameText( STR_INV_WEAR_WEAPON_BROKEN ) );
                    break;
                }
            }
            else if( is_reload )
            {
                if( !is_self )
                    break;

                CHECK_NEED_AP( ap_cost );

                if( is_main_item )
                {
                    item->SetMode( USE_PRIMARY );
                    Net_SendRateItem();
                    if( GetActiveScreen() == CLIENT_SCREEN_NONE )
                        SetCurMode( CURSOR_USE_WEAPON );
                }

                if( !item->WeapGetMaxAmmoCount() )
                    break;                        // No have holder

                if( target_id == uint( -1 ) )     // Unload
                {
                    if( item->WeapIsEmpty() )
                        break;                    // Is empty
                    target_id = 0;
                }
                else if( !target_item_self )      // Reload
                {
                    if( item->WeapIsFull() )
                        break;                    // Is full
                    if( item->WeapGetAmmoCaliber() )
                    {
                        target_item_self = Chosen->GetAmmoAvialble( item );
                        if( !target_item_self )
                            break;
                        target_id = target_item_self->GetId();
                    }
                }
                else            // Load
                {
                    if( item->WeapGetAmmoCaliber() != target_item_self->AmmoGetCaliber() )
                        break;  // Different caliber
                    if( item->WeapGetAmmoPid() == target_item_self->GetProtoId() && item->WeapIsFull() )
                        break;  // Is full
                }
            }
            else                // Use
            {
                if( use != USE_USE )
                    break;
            }

            // Find Target
            if( !is_self && HexMngr.IsMapLoaded() )
            {
                uint16 hx, hy;
                if( target_cr )
                {
                    hx = target_cr->GetHexX();
                    hy = target_cr->GetHexY();
                }
                else
                {
                    hx = target_item->GetHexX();
                    hy = target_item->GetHexY();
                }

                uint max_dist = (is_attack ? Chosen->GetAttackDist() : Chosen->GetUseDist() ) + (target_cr ? target_cr->GetMultihex() : 0);

                // Target find
                bool need_move = false;
                if( is_attack )
                    need_move = !HexMngr.TraceBullet( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, max_dist, 0.0f, target_cr, false, NULL, 0, NULL, NULL, NULL, true );
                else
                    need_move = !CheckDist( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, max_dist );
                if( need_move )
                {
                    // If target too far, then move to it
                    uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy );
                    if( dist > max_dist )
                    {
                        if( IsTurnBased )
                            break;
                        bool is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                        if( target_cr )
                            SetAction( CHOSEN_MOVE_TO_CRITTER, target_cr->GetId(), 0, is_run, max_dist );
                        else
                            SetAction( CHOSEN_MOVE, hx, hy, is_run, max_dist, 0 );
                        if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, max_dist ) )
                            AddActionBack( act );
                        return;
                    }
                    AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_FINDPATH_AIMBLOCK ) );
                    break;
                }

                // Refresh orientation
                CHECK_NEED_AP( ap_cost );
                uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy );
                if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy ) >= 1 && Chosen->GetDir() != dir )
                {
                    Chosen->SetDir( dir );
                    Net_SendDir();
                }
            }

            // Use
            CHECK_NEED_AP( ap_cost );
			if (target_item && target_item->IsScenOrGrid()) {
				Net_SendUseItem(ap_cost, item_id, item_pid, rate, target_type, (target_item->GetHexX() << 16) | (target_item->GetHexY() & 0xFFFF), target_item->GetProtoId(), param);
			} else {
				Net_SendUseItem(ap_cost, item_id, item_pid, rate, target_type, target_id, 0, param);  // Item or critter
			}

            int usei = use;                                                                             // Avoid 'warning: comparison is always true due to limited range of data type' for GCC
			if (usei >= USE_PRIMARY && use <= USE_THIRD)
                Chosen->Action( CRITTER_ACTION_USE_WEAPON, rate, item );
            else if( use == USE_RELOAD )
                Chosen->Action( CRITTER_ACTION_RELOAD_WEAPON, 0, item );
            else
                Chosen->Action( CRITTER_ACTION_USE_ITEM, 0, item );

			//	 to allow animation canceling, AP cost will be subtracted AS side
			if (!is_attack) {
				Chosen->SubAp(ap_cost);
			}

            if( is_attack && !aim && Keyb::ShiftDwn )           // Continue battle after attack
            {
                AddActionBack( act );
                return;
            }
            break;
        }
        case CHOSEN_MOVE_ITEM:
        {
            uint  item_id = act.Param[0];
            uint  item_count = act.Param[1];
            int   to_slot = act.Param[2];
            bool  is_barter_cont = (act.Param[3] != 0);
            bool  is_second_try = (act.Param[4] != 0);

            Item* item = Chosen->GetItem( item_id );
            if( !item )
                break;

            uint8 from_slot = item->AccCritter.Slot;
            if( item->AccCritter.Slot != from_slot || from_slot == to_slot )
                break;
            if( to_slot != SLOT_GROUND && !CritterCl::SlotEnabled[to_slot] )
                break;

            Item* item_swap = ( (to_slot != SLOT_INV && to_slot != SLOT_GROUND) ? Chosen->GetItemSlot( to_slot ) : NULL );
            bool  allow = false;
            if( Script::PrepareContext( ClientFunctions.CritterCheckMoveItem, _FUNC_, "Game" ) )
            {
                Script::SetArgObject( Chosen );
                Script::SetArgObject( item );
                Script::SetArgUChar( to_slot );
                Script::SetArgObject( item_swap );
                if( Script::RunPrepared() )
                    allow = Script::GetReturnedBool();
            }
            if( !allow )
            {
                // Gameplay swap workaround
                if( item_swap && !is_second_try )
                {
                    // Hindering item to inventory
                    bool allow1 = false;
                    if( Script::PrepareContext( ClientFunctions.CritterCheckMoveItem, _FUNC_, "Game" ) )
                    {
                        Script::SetArgObject( Chosen );
                        Script::SetArgObject( item_swap );
                        Script::SetArgUChar( SLOT_INV );
                        Script::SetArgObject( NULL );
                        if( Script::RunPrepared() )
                            allow1 = Script::GetReturnedBool();
                    }

                    // Current item to empty slot
                    bool allow2 = false;
                    if( allow1 && Script::PrepareContext( ClientFunctions.CritterCheckMoveItem, _FUNC_, "Game" ) )
                    {
                        Script::SetArgObject( Chosen );
                        Script::SetArgObject( item );
                        Script::SetArgUChar( to_slot );
                        Script::SetArgObject( NULL );
                        if( Script::RunPrepared() )
                            allow2 = Script::GetReturnedBool();
                    }

                    // Add actions
                    if( allow1 && allow2 )
                    {
                        EraseFrontAction();
                        AddActionFront( CHOSEN_MOVE_ITEM, item_id, item_count, to_slot, is_barter_cont, true );                        // Second
                        AddActionFront( CHOSEN_MOVE_ITEM, item_swap->GetId(), item_swap->GetCount(), SLOT_INV, is_barter_cont, true ); // First
                        return;
                    }
                }
                break;
            }

            // Action points
            bool is_castling = ( (from_slot == SLOT_HAND1 && to_slot == SLOT_HAND2) || (from_slot == SLOT_HAND2 && to_slot == SLOT_HAND1) );
            uint ap_cost = (is_castling ? 0 : Chosen->GetApCostMoveItemInventory() );
            if( to_slot == SLOT_GROUND )
                ap_cost = Chosen->GetApCostDropItem();
            CHECK_NEED_AP( ap_cost );

            // Move
            if( to_slot == SLOT_GROUND )
            {
                Chosen->Action( CRITTER_ACTION_DROP_ITEM, from_slot, item );
                if( item_count < item->GetCount() )
                    item->Count_Sub( item_count );
				else {
					removeItemASCallback(item, -(float)(item->GetCount()), "CrittersProcess()");
					Chosen->EraseItem(item, true);
				}
            }
            else
            {
                if( from_slot == SLOT_HAND1 || to_slot == SLOT_HAND1 )
                    Chosen->ItemSlotMain = Chosen->DefItemSlotHand;
                if( from_slot == SLOT_HAND2 || to_slot == SLOT_HAND2 )
                    Chosen->ItemSlotExt = Chosen->DefItemSlotHand;
                if( from_slot == SLOT_ARMOR || to_slot == SLOT_ARMOR )
                    Chosen->ItemSlotArmor = Chosen->DefItemSlotArmor;

                if( to_slot == SLOT_HAND1 )
                    Chosen->ItemSlotMain = item;
                else if( to_slot == SLOT_HAND2 )
                    Chosen->ItemSlotExt = item;
                else if( to_slot == SLOT_ARMOR )
                    Chosen->ItemSlotArmor = item;
                if( item_swap )
                {
                    if( from_slot == SLOT_HAND1 )
                        Chosen->ItemSlotMain = item_swap;
                    else if( from_slot == SLOT_HAND2 )
                        Chosen->ItemSlotExt = item_swap;
                    else if( from_slot == SLOT_ARMOR )
                        Chosen->ItemSlotArmor = item_swap;
                }

                item->AccCritter.Slot = to_slot;
                if( item_swap )
                    item_swap->AccCritter.Slot = from_slot;

                Chosen->Action( CRITTER_ACTION_MOVE_ITEM, from_slot, item );
                if( item_swap )
                    Chosen->Action( CRITTER_ACTION_MOVE_ITEM_SWAP, to_slot, item_swap );
            }

            // Affect barter screen
            if( to_slot == SLOT_GROUND && is_barter_cont && IsScreenPresent( CLIENT_SCREEN_BARTER ) )
            {
                auto it = std::find( BarterCont1oInit.begin(), BarterCont1oInit.end(), item_id );
                if( it != BarterCont1oInit.end() )
                {
                    Item& item_ = *it;
                    if( item_count >= item_.GetCount() )
                        BarterCont1oInit.erase( it );
                    else
                        item_.Count_Sub( item_count );
                }
                CollectContItems();
            }

            // Light
            if( to_slot == SLOT_HAND1 || from_slot == SLOT_HAND1 )
                RebuildLookBorders = true;
            if( item->IsLight() && (to_slot == SLOT_INV || (from_slot == SLOT_INV && to_slot != SLOT_GROUND) ) )
                HexMngr.RebuildLight();

            // Notice server
            Net_SendChangeItem( ap_cost, item_id, from_slot, to_slot, item_count );

            // Spend AP
            Chosen->SubAp( ap_cost );
            break;
        }
        case CHOSEN_MOVE_ITEM_CONTAINER:
        {
            uint item_id = act.Param[0];
            uint cont = act.Param[1];
            uint count = act.Param[2];

            CHECK_NEED_AP( Chosen->GetApCostMoveItemContainer() );

            Item* item = GetContainerItem( cont == IFACE_PUP_CONT1 ? InvContInit : PupCont2Init, item_id );
            if( !item )
                break;
            if( count > item->GetCount() )
                break;

            if( cont == IFACE_PUP_CONT2 )
            {
                if( Chosen->GetFreeWeight() < (int)(item->GetWeight1st() * count) )
                {
                    AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_OVERWEIGHT ) );
                    break;
                }
                if( Chosen->GetFreeVolume() < (int)(item->GetVolume1st() * count) )
                {
                    AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_OVERVOLUME ) );
                    break;
                }
            }

            Chosen->Action( CRITTER_ACTION_OPERATE_CONTAINER, PupTransferType * 10 + (cont == IFACE_PUP_CONT2 ? 0 : 2), item );
            PupTransfer( item_id, cont, count );
            Chosen->SubAp( Chosen->GetApCostMoveItemContainer() );
            break;
        }
        case CHOSEN_TAKE_ALL:
        {
            CHECK_NEED_AP( Chosen->GetApCostMoveItemContainer() );

            if( PupCont2Init.empty() )
                break;

            uint c, w, v;
            ContainerCalcInfo( PupCont2Init, c, w, v, MAX_INT, false );
            if( Chosen->GetFreeWeight() < (int)w )
                AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_BARTER_OVERWEIGHT ) );
            else if( Chosen->GetFreeVolume() < (int)v )
                AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_BARTER_OVERSIZE ) );
            else
            {
                PupCont2Init.clear();
                PupCount = 0;
                Net_SendItemCont( PupTransferType, PupContId, PupHoldId, 0, CONTAINER_GETALL );
                Chosen->Action( CRITTER_ACTION_OPERATE_CONTAINER, PupTransferType * 10 + 1, NULL );
                Chosen->SubAp( Chosen->GetApCostMoveItemContainer() );
                CollectContItems();
            }
            break;
        }
        case CHOSEN_USE_SKILL_ON_CRITTER:
        {
            uint16     skill = act.Param[0];
            uint       crid = act.Param[1];

            CritterCl* cr = (crid ? GetCritter( crid ) : Chosen);
            if( !cr )
                break;

            if( skill == SK_STEAL && cr->IsRawParam( MODE_NO_STEAL ) )
                break;

            if( cr != Chosen && HexMngr.IsMapLoaded() )
            {
                // Check distantance
                uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() );
                switch( skill )
                {
                    case SK_LOCKPICK:
                    case SK_STEAL:
                    case SK_TRAPS:
                    case SK_FIRST_AID:
                    case SK_DOCTOR:
                    case SK_SCIENCE:
                    case SK_REPAIR:
                        if( dist > Chosen->GetUseDist() + cr->GetMultihex() )
                        {
                            if( IsTurnBased )
                                break;
                            bool   is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                            uint   cut_dist = Chosen->GetUseDist() + cr->GetMultihex();
                            SetAction( CHOSEN_MOVE_TO_CRITTER, cr->GetId(), 0, is_run, cut_dist );
                            uint16 hx = cr->GetHexX(), hy = cr->GetHexY();
                            if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, cut_dist ) )
                                AddActionBack( act );
                            return;
                        }
                        break;
                    default:
                        break;
                }

                if( cr != Chosen )
                {
                    // Refresh orientation
                    CHECK_NEED_AP( Chosen->GetApCostUseSkill() );
                    uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() );
                    if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() ) >= 1 && Chosen->GetDir() != dir )
                    {
                        Chosen->SetDir( dir );
                        Net_SendDir();
                    }
                }
            }

            CHECK_NEED_AP( Chosen->GetApCostUseSkill() );
            Chosen->Action( CRITTER_ACTION_USE_SKILL, skill - (GameOpt.AbsoluteOffsets ? 0 : SKILL_BEGIN), NULL );
            Net_SendUseSkill( skill, cr );
            Chosen->SubAp( Chosen->GetApCostUseSkill() );
            WaitPing();
            break;
        }
        case CHOSEN_USE_SKILL_ON_ITEM:
        {
            bool   is_inv = act.Param[0] != 0;
            uint16 skill = act.Param[1];
            uint   item_id = act.Param[2];

            Item*  item_action;
            if( is_inv )
            {
                Item* item = Chosen->GetItem( item_id );
                if( !item )
                    break;
                item_action = item;

                if( skill == SK_SCIENCE && item->IsHolodisk() )
                {
                    ShowScreen( CLIENT_SCREEN_INPUTBOX );
                    IboxMode = IBOX_MODE_HOLO;
                    IboxHolodiskId = item->GetId();
                    IboxTitle = GetHoloText( STR_HOLO_INFO_NAME_( item->HolodiskGetNum() ) );
                    IboxText = GetHoloText( STR_HOLO_INFO_DESC_( item->HolodiskGetNum() ) );
                    if( IboxTitle.length() > USER_HOLO_MAX_TITLE_LEN )
                        IboxTitle.resize( USER_HOLO_MAX_TITLE_LEN );
                    if( IboxText.length() > USER_HOLO_MAX_LEN )
                        IboxText.resize( USER_HOLO_MAX_LEN );
                    break;
                }

                CHECK_NEED_AP( Chosen->GetApCostUseSkill() );
                Net_SendUseSkill( skill, item );
            }
            else
            {
                ItemHex* item = HexMngr.GetItemById( item_id );
                if( !item )
                    break;
                item_action = item;

                if( HexMngr.IsMapLoaded() )
                {
                    uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), item->GetHexX(), item->GetHexY() );
                    if( dist > Chosen->GetUseDist() )
                    {
                        if( IsTurnBased )
                            break;
                        bool   is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                        SetAction( CHOSEN_MOVE, item->GetHexX(), item->GetHexY(), is_run, Chosen->GetUseDist(), 0 );
                        uint16 hx = item->GetHexX(), hy = item->GetHexY();
                        if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, Chosen->GetUseDist() ) )
                            AddActionBack( act );
                        return;
                    }

                    // Refresh orientation
                    CHECK_NEED_AP( Chosen->GetApCostUseSkill() );
                    uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), item->GetHexX(), item->GetHexY() );
                    if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), item->GetHexX(), item->GetHexY() ) >= 1 && Chosen->GetDir() != dir )
                    {
                        Chosen->SetDir( dir );
                        Net_SendDir();
                    }
                }

                CHECK_NEED_AP( Chosen->GetApCostUseSkill() );
                Net_SendUseSkill( skill, item );
            }

            Chosen->Action( CRITTER_ACTION_USE_SKILL, skill - (GameOpt.AbsoluteOffsets ? 0 : SKILL_BEGIN), item_action );
            Chosen->SubAp( Chosen->GetApCostUseSkill() );
            WaitPing();
            break;
        }
        case CHOSEN_USE_SKILL_ON_SCENERY:
        {
            uint16 skill = act.Param[0];
            uint16 pid = act.Param[1];
            uint16 hx = act.Param[2];
            uint16 hy = act.Param[3];

            if( !HexMngr.IsMapLoaded() )
                break;

            ItemHex* item = HexMngr.GetItem( hx, hy, pid );
            if( !item )
                break;

            uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy );
            if( dist > Chosen->GetUseDist() )
            {
                if( IsTurnBased )
                    break;
                bool is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                SetAction( CHOSEN_MOVE, hx, hy, is_run, Chosen->GetUseDist(), 0 );
                if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, Chosen->GetUseDist() ) )
                    AddActionBack( act );
                return;
            }

            CHECK_NEED_AP( Chosen->GetApCostUseSkill() );

            // Refresh orientation
            uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), item->GetHexX(), item->GetHexY() );
            if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), item->GetHexX(), item->GetHexY() ) >= 1 && Chosen->GetDir() != dir )
            {
                Chosen->SetDir( dir );
                Net_SendDir();
            }

            Chosen->Action( CRITTER_ACTION_USE_SKILL, skill - (GameOpt.AbsoluteOffsets ? 0 : SKILL_BEGIN), item );
            Net_SendUseSkill( skill, item );
            Chosen->SubAp( Chosen->GetApCostUseSkill() );
            // WaitPing();
            break;
        }
        case CHOSEN_TALK_NPC:
        {
            uint crid = act.Param[0];

            if( !HexMngr.IsMapLoaded() )
                break;

            CritterCl* cr = GetCritter( crid );
            if( !cr )
                break;

            if( IsTurnBased )
                break;

            if( cr->IsDead() )
            {
                // TODO: AddMess Dead CritterCl
                break;
            }

            uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() );
            uint talk_distance = cr->GetTalkDistance() + Chosen->GetMultihex();
            if( dist > talk_distance )
            {
                if( IsTurnBased )
                    break;
                bool   is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                SetAction( CHOSEN_MOVE_TO_CRITTER, cr->GetId(), 0, is_run, talk_distance );
                uint16 hx = cr->GetHexX(), hy = cr->GetHexY();
                if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, talk_distance ) )
                    AddActionBack( act );
                return;
            }

            if( !HexMngr.TraceBullet( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY(), talk_distance + Chosen->GetMultihex(), 0.0f, cr, false, NULL, 0, NULL, NULL, NULL, true ) )
            {
                AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_FINDPATH_AIMBLOCK ) );
                break;
            }

            // Refresh orientation
            uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() );
            if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() ) >= 1 && Chosen->GetDir() != dir )
            {
                Chosen->SetDir( dir );
                Net_SendDir();
            }

            Net_SendTalk( true, cr->GetId(), ANSWER_BEGIN );
            WaitPing();
            break;
        }
        case CHOSEN_PICK_ITEM:
        {
            uint16 pid = act.Param[0];
            uint16 hx = act.Param[1];
            uint16 hy = act.Param[2];

            if( !HexMngr.IsMapLoaded() )
                break;

            ItemHex* item = HexMngr.GetItem( hx, hy, pid );
            if( !item )
                break;

            uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy );
            if( dist > Chosen->GetUseDist() )
            {
                if( IsTurnBased )
                    break;
                bool is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                SetAction( CHOSEN_MOVE, hx, hy, is_run, Chosen->GetUseDist(), 0 );
                if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, Chosen->GetUseDist() ) )
                    AddActionBack( act );
                AddActionBack( act );
                return;
            }

            CHECK_NEED_AP( Chosen->GetApCostPickItem() );

            // Refresh orientation
            uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy );
            if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), hx, hy ) >= 1 && Chosen->GetDir() != dir )
            {
                Chosen->SetDir( dir );
                Net_SendDir();
            }

            Net_SendPickItem( hx, hy, pid );
            Chosen->Action( CRITTER_ACTION_PICK_ITEM, 0, item );
            Chosen->SubAp( Chosen->GetApCostPickItem() );
            // WaitPing();
            break;
        }
        case CHOSEN_PICK_CRITTER:
        {
            uint crid = act.Param[0];
            bool is_loot = (act.Param[1] == 0);

            if( !HexMngr.IsMapLoaded() )
                break;

            CritterCl* cr = GetCritter( crid );
            if( !cr )
                break;

            if( is_loot && (!cr->IsDead() || cr->IsRawParam( MODE_NO_LOOT ) ) )
                break;
            if( !is_loot && (!cr->IsLife() || cr->IsRawParam( MODE_NO_PUSH ) ) )
                break;

            uint dist = DistGame( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() );
            uint pick_dist = Chosen->GetUseDist() + cr->GetMultihex();
            if( dist > pick_dist )
            {
                if( IsTurnBased )
                    break;
                bool   is_run = (GameOpt.AlwaysRun && dist >= GameOpt.AlwaysRunUseDist);
                SetAction( CHOSEN_MOVE_TO_CRITTER, cr->GetId(), 0, is_run, pick_dist );
                uint16 hx = cr->GetHexX(), hy = cr->GetHexY();
                if( HexMngr.CutPath( Chosen, Chosen->GetHexX(), Chosen->GetHexY(), hx, hy, pick_dist ) )
                    AddActionBack( act );
                return;
            }

            CHECK_NEED_AP( Chosen->GetApCostPickCritter() );

            // Refresh orientation
            uint8 dir = GetFarDir( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() );
            if( DistGame( Chosen->GetHexX(), Chosen->GetHexY(), cr->GetHexX(), cr->GetHexY() ) >= 1 && Chosen->GetDir() != dir )
            {
                Chosen->SetDir( dir );
                Net_SendDir();
            }

            if( is_loot )
            {
                Net_SendPickCritter( cr->GetId(), PICK_CRIT_LOOT );
                Chosen->Action( CRITTER_ACTION_PICK_CRITTER, 0, NULL );
            }
            else if( cr->IsLife() )
            {
                Net_SendPickCritter( cr->GetId(), PICK_CRIT_PUSH );
                Chosen->Action( CRITTER_ACTION_PICK_CRITTER, 2, NULL );
            }
            Chosen->SubAp( Chosen->GetApCostPickCritter() );
            WaitPing();
            break;
        }
        case CHOSEN_WRITE_HOLODISK:
        {
            uint holodisk_id = act.Param[0];
            if( holodisk_id != IboxHolodiskId )
                break;
            Item* holo = Chosen->GetItem( IboxHolodiskId );
            if( !holo->IsHolodisk() )
                break;
            const char* old_title = GetHoloText( STR_HOLO_INFO_NAME_( holo->HolodiskGetNum() ) );
            const char* old_text = GetHoloText( STR_HOLO_INFO_DESC_( holo->HolodiskGetNum() ) );
            if( holo && IboxTitle.length() && IboxText.length() && (IboxTitle != old_title || IboxText != old_text) )
            {
                Net_SendSetUserHoloStr( holo, IboxTitle.c_str(), IboxText.c_str() );
                Chosen->Action( CRITTER_ACTION_USE_ITEM, 0, holo );
            }
            break;
        }
    }

    EraseFrontAction();
}

void FOClient::TryPickItemOnGround()
{
    if( !Chosen )
        return;
    ItemHexVec items;
    HexMngr.GetItems( Chosen->GetHexX(), Chosen->GetHexY(), items );
    for( auto it = items.begin(); it != items.end();)
    {
        ItemHex* item = *it;
        if( item->IsFinishing() || item->Proto->IsDoor() || !item->IsUsable() )
            it = items.erase( it );
        else
            ++it;
    }
    if( items.empty() )
        return;
    ItemHex* item = items[Random( 0, (int)items.size() - 1 )];
    SetAction( CHOSEN_PICK_ITEM, item->GetProtoId(), item->HexX, item->HexY );
}

void FOClient::TryExit()
{
    int active = GetActiveScreen();
    if( active != CLIENT_SCREEN_NONE )
    {
        switch( active )
        {
            case CLIENT_SCREEN_TIMER:
                TimerClose( false );
                break;
            case CLIENT_SCREEN_SPLIT:
                SplitClose( false );
                break;
            case CLIENT_SCREEN_WM_TOWNVIEW:
                Net_SendRefereshMe();
                break;
            case CLIENT_SCREEN_DIALOG:
                DlgKeyDown( true, DIK_ESCAPE, "" );
                break;
            case CLIENT_SCREEN_BARTER:
                DlgKeyDown( false, DIK_ESCAPE, "" );
                break;
            case CLIENT_SCREEN_DIALOGBOX:
                if( DlgboxType >= DIALOGBOX_ENCOUNTER_ANY && DlgboxType <= DIALOGBOX_ENCOUNTER_TB )
                    Net_SendRuleGlobal( GM_CMD_ANSWER, -1 );
            case CLIENT_SCREEN_PERK:
            case CLIENT_SCREEN_ELEVATOR:
            case CLIENT_SCREEN_SAY:
            case CLIENT_SCREEN_SKILLBOX:
            case CLIENT_SCREEN_USE:
            case CLIENT_SCREEN_AIM:
            case CLIENT_SCREEN_INVENTORY:
            case CLIENT_SCREEN_PICKUP:
            case CLIENT_SCREEN_MINIMAP:
            case CLIENT_SCREEN_CHARACTER:
            case CLIENT_SCREEN_PIPBOY:
            case CLIENT_SCREEN_FIXBOY:
            case CLIENT_SCREEN_MENU:
            case CLIENT_SCREEN_LOADSAVE:
            default:
                ShowScreen( CLIENT_SCREEN_NONE );
                break;
        }
    }
    else
    {
        switch( GetMainScreen() )
        {
            case CLIENT_MAIN_SCREEN_LOGIN:
                GameOpt.Quit = true;
                break;
            case CLIENT_MAIN_SCREEN_REGISTRATION:
            case CLIENT_MAIN_SCREEN_CREDITS:
                ShowMainScreen( CLIENT_MAIN_SCREEN_LOGIN );
                break;
            case CLIENT_MAIN_SCREEN_WAIT:
				WriteLog("DC: FOClient::TryExit()");
                NetDisconnect();
                break;
            case CLIENT_MAIN_SCREEN_WORLDMAP:
                ShowScreen( CLIENT_SCREEN_MENU );
                break;
            case CLIENT_MAIN_SCREEN_GAME:
                ShowScreen( CLIENT_SCREEN_MENU );
                break;
            default:
                break;
        }
    }
}

bool FOClient::IsScroll()
{
    return GameOpt.ScrollMouseUp || GameOpt.ScrollMouseRight || GameOpt.ScrollMouseDown || GameOpt.ScrollMouseLeft || GameOpt.ScrollKeybUp || GameOpt.ScrollKeybRight || GameOpt.ScrollKeybDown || GameOpt.ScrollKeybLeft;
}

void FOClient::ProcessMouseScroll()
{
    if( IsLMenu() || !GameOpt.MouseScroll )
        return;

    if( GameOpt.MouseX >= MODE_WIDTH - 1 )
        GameOpt.ScrollMouseRight = true;
    else
        GameOpt.ScrollMouseRight = false;

    if( GameOpt.MouseX <= 0 )
        GameOpt.ScrollMouseLeft = true;
    else
        GameOpt.ScrollMouseLeft = false;

    if( GameOpt.MouseY >= MODE_HEIGHT - 1 )
        GameOpt.ScrollMouseDown = true;
    else
        GameOpt.ScrollMouseDown = false;

    if( GameOpt.MouseY <= 0 )
        GameOpt.ScrollMouseUp = true;
    else
        GameOpt.ScrollMouseUp = false;
}

void FOClient::ProcessKeybScroll( bool down, uint8 dik )
{
    if( down && IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && ConsoleActive )
        return;

    switch( dik )
    {
        case DIK_LEFT:
            GameOpt.ScrollKeybLeft = down;
            break;
        case DIK_RIGHT:
            GameOpt.ScrollKeybRight = down;
            break;
        case DIK_UP:
            GameOpt.ScrollKeybUp = down;
            break;
        case DIK_DOWN:
            GameOpt.ScrollKeybDown = down;
            break;
        default:
            break;
    }
}

void FOClient::DropScroll()
{
    GameOpt.ScrollMouseUp = false;
    GameOpt.ScrollMouseRight = false;
    GameOpt.ScrollMouseDown = false;
    GameOpt.ScrollMouseLeft = false;
    GameOpt.ScrollKeybUp = false;
    GameOpt.ScrollKeybRight = false;
    GameOpt.ScrollKeybDown = false;
    GameOpt.ScrollKeybLeft = false;
}

bool FOClient::IsCurInWindow()
{
    if( !MainWindow->Focused )
        return false;

    if( !GameOpt.FullScreen )
    {
        if( IsLMenu() )
            return true;

        int mx = 0, my = 0;
        Fl::get_mouse( mx, my );
        return mx >= MainWindow->x() && mx <= MainWindow->x() + MainWindow->w() &&
               my >= MainWindow->y() && my <= MainWindow->y() + MainWindow->h();
    }
    return true;
}

void FOClient::FlashGameWindow()
{
    if( MainWindow->Focused )
        return;

    #ifdef FO_WINDOWS
    if( GameOpt.MessNotify )
        FlashWindow( fl_xid( MainWindow ), true );
    if( GameOpt.SoundNotify )
        Beep( 100, 200 );
    #else
    // Todo: linux
    #endif
}

const char* FOClient::GetHoloText( uint str_num )
{
    if( str_num / 10 >= USER_HOLO_START_NUM )
    {
        if( !MsgUserHolo->Count( str_num ) )
        {
            Net_SendGetUserHoloStr( str_num );
            static char wait[32] = "...";
            return wait;
        }
        return MsgUserHolo->GetStr( str_num );
    }
    return MsgHolo->GetStr( str_num );
}

const char* FOClient::FmtGameText( uint str_num, ... )
{
//	return Str::FormatBuf(MsgGame->GetStr(str_num),(void*)(_ADDRESSOF(str_num)+_INTSIZEOF(str_num)));

    static char res[MAX_FOTEXT];
    static char str[MAX_FOTEXT];

    Str::Copy( str, MsgGame->GetStr( str_num ) );
    Str::Replacement( str, '\\', 'n', '\n' );

    va_list list;
    va_start( list, str_num );
    vsprintf( res, str, list );
    va_end( list );

    return res;
}

const char* FOClient::FmtCombatText( uint str_num, ... )
{
//	return Str::FormatBuf(MsgCombat->GetStr(str_num),(void*)(_ADDRESSOF(str_num)+_INTSIZEOF(str_num)));

    static char res[MAX_FOTEXT];
    static char str[MAX_FOTEXT];

    Str::Copy( str, MsgCombat->GetStr( str_num ) );

    va_list list;
    va_start( list, str_num );
    vsprintf( res, str, list );
    va_end( list );

    return res;
}

const char* FOClient::FmtGenericDesc( int desc_type, int& ox, int& oy )
{
    ox = 0;
    oy = 0;
    if( Script::PrepareContext( ClientFunctions.GenericDesc, _FUNC_, "Game" ) )
    {
        Script::SetArgUInt( desc_type );
        Script::SetArgAddress( &ox );
        Script::SetArgAddress( &oy );
        if( Script::RunPrepared() )
        {
            ScriptString* result = (ScriptString*)Script::GetReturnedObject();
            if( result )
            {
                static char str[MAX_FOTEXT];
                Str::Copy( str, result->c_str() );
                return str[0] ? str : NULL;
            }
        }
    }

    return Str::FormatBuf( "<error>" );
}

const char* FOClient::FmtCritLook( CritterCl* cr, int look_type )
{
    if( Script::PrepareContext( ClientFunctions.CritterLook, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( cr );
        Script::SetArgUInt( look_type );
        if( Script::RunPrepared() )
        {
            ScriptString* result = (ScriptString*)Script::GetReturnedObject();
            if( result )
            {
                static char str[MAX_FOTEXT];
                Str::Copy( str, MAX_FOTEXT, result->c_str() );
                return str[0] ? str : NULL;
            }
        }
    }

    return MsgGame->GetStr( STR_CRIT_LOOK_NOTHING );
}

const char* FOClient::FmtItemLook( Item* item, int look_type )
{
    if( Script::PrepareContext( ClientFunctions.ItemLook, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( item );
        Script::SetArgUInt( look_type );
        if( Script::RunPrepared() )
        {
            ScriptString* result = (ScriptString*)Script::GetReturnedObject();
            if( result )
            {
                static char str[MAX_FOTEXT];
                Str::Copy( str, MAX_FOTEXT, result->c_str() );
                return str[0] ? str : NULL;
            }
        }
    }

    return MsgGame->GetStr( STR_ITEM_LOOK_NOTHING );
}

void FOClient::ParseIntellectWords( char* words, PCharPairVec& text )
{
    Str::SkipLine( words );

    bool parse_in = true;
    char in[MAX_FOTEXT] = { 0 };
    char out[MAX_FOTEXT] = { 0 };

    while( true )
    {
        if( *words == 0 )
            break;

        if( *words == '\n' || *words == '\r' )
        {
            Str::SkipLine( words );
            parse_in = true;

            uint in_len = Str::Length( in ) + 1;
            uint out_len = Str::Length( out ) + 1;
            if( in_len < 2 )
                continue;

            char* in_ = new char[in_len];
            char* out_ = new char[out_len];
            Str::Copy( in_, in_len, in );
            Str::Copy( out_, out_len, out );

            text.push_back( PAIR( in_, out_ ) );
            in[0] = 0;
            out[0] = 0;
            continue;
        }

        if( *words == ' ' && *(words + 1) == ' ' && parse_in )
        {
            if( !Str::Length( in ) )
            {
                Str::SkipLine( words );
            }
            else
            {
                words += 2;
                parse_in = false;
            }
            continue;
        }

        strncat( parse_in ? in : out, words, 1 );
        words++;
    }
}

auto FOClient::FindIntellectWord( const char* word, PCharPairVec& text, Randomizer& rnd )->PCharPairVec::iterator
{
    auto it = text.begin();
    auto end = text.end();
    for( ; it != end; ++it )
    {
        if( Str::CompareCaseUTF8( word, (*it).first ) )
            break;
    }

    if( it != end )
    {
        auto it_ = it;
        it++;
        int  cnt = 0;
        for( ; it != end; ++it )
        {
            if( Str::CompareCaseUTF8( (*it_).first, (*it).first ) )
                cnt++;
            else
                break;
        }
        it_ += rnd.Random( 0, cnt );
        it = it_;
    }

    return it;
}

void FOClient::FmtTextIntellect( char* str, uint16 intellect )
{
    static bool is_parsed = false;
    if( !is_parsed )
    {
        ParseIntellectWords( (char*)MsgGame->GetStr( STR_INTELLECT_WORDS ), IntellectWords );
        ParseIntellectWords( (char*)MsgGame->GetStr( STR_INTELLECT_SYMBOLS ), IntellectSymbols );
        is_parsed = true;
    }

    uint8 intellegence = intellect & 0xF;
    if( !intellect || intellegence >= 5 )
        return;

    int word_proc;
    int symbol_proc;
    switch( intellegence )
    {
        default:
        case 1:
            word_proc = 100;
            symbol_proc = 20;
            break;
        case 2:
            word_proc = 80;
            symbol_proc = 15;
            break;
        case 3:
            word_proc = 60;
            symbol_proc = 10;
            break;
        case 4:
            word_proc = 30;
            symbol_proc = 5;
            break;
    }

    Randomizer rnd( (intellect << 16) | intellect );

    char       word[1024] = { 0 };
    while( true )
    {
        while( *str )
        {
            uint length;
            Str::DecodeUTF8( str, &length );
            if( length == 1 && !( (*str >= 'a' && *str <= 'z') || (*str >= 'A' && *str <= 'Z') ) )
                break;
            strncat( word, str, length );
            str += length;
        }

        uint len = Str::Length( word );
        if( len )
        {
            auto it = FindIntellectWord( word, IntellectWords, rnd );
            if( it != IntellectWords.end() && rnd.Random( 1, 100 ) <= word_proc )
            {
                Str::EraseInterval( str - len, len );
                Str::Insert( str - len, (*it).second );
                str = str - len + Str::Length( (*it).second );
            }
            else
            {
                for( char* s = str - len; s < str;)
                {
                    if( rnd.Random( 1, 100 ) > symbol_proc )
                        continue;

                    uint length;
                    Str::DecodeUTF8( s, &length );
                    strncpy( word, s, length );
                    word[length] = 0;

                    it = FindIntellectWord( word, IntellectSymbols, rnd );
                    if( it != IntellectSymbols.end() )
                    {
                        uint f_len = Str::Length( (*it).first );
                        uint s_len = Str::Length( (*it).second );
                        Str::EraseInterval( s, f_len );
                        Str::Insert( s, (*it).second );
                        str -= f_len;
                        str += s_len;
                    }

                    s += length;
                }
            }
            word[0] = 0;
        }

        if( *str == 0 )
            break;

        uint length;
        Str::DecodeUTF8( str, &length );
        str += length;
    }
}

bool FOClient::SaveLogFile()
{
    if( MessBox.empty() )
        return false;

    DateTime dt;
    Timer::GetCurrentDateTime( dt );
    char     log_path[MAX_FOPATH];
    Str::Format( log_path, "messbox_%04d.%02d.%02d_%02d-%02d-%02d.txt",
                 dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second );

    if( Script::PrepareContext( ClientFunctions.FilenameLogfile, _FUNC_, "Game" ) )
    {
        char*         str = log_path;
        ScriptString* sstr = new ScriptString( str );
        Script::SetArgObject( sstr );
        if( Script::RunPrepared() )
            Str::Copy( log_path, sstr->c_str() );
        sstr->Release();
    }

    if( Str::Compare( log_path, "" ) )
        return false;

    FileManager::FormatPath( log_path );
    FileManager::CreateDirectoryTree( FileManager::GetFullPath( log_path, PATH_ROOT ) );

    void* f = FileOpen( log_path, true );
    if( !f )
        return false;

    char   cur_mess[MAX_FOTEXT];
    string fmt_log;
    for( uint i = 0; i < MessBox.size(); ++i )
    {
        MessBoxMessage& m = MessBox[i];
        // Skip
        if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) && std::find( MessBoxFilters.begin(), MessBoxFilters.end(), m.Type ) != MessBoxFilters.end() )
            continue;
        // Concat
        Str::Copy( cur_mess, m.Mess.c_str() );
        Str::EraseWords( cur_mess, '|', ' ' );
        fmt_log += MessBox[i].Time + string( cur_mess );
    }

    FileWrite( f, fmt_log.c_str(), (uint)fmt_log.length() );
    FileClose( f );
    return true;
}

bool FOClient::SaveScreenshot()
{
    if( !SprMngr.IsInit() )
        return false;

    DateTime dt;
    Timer::GetCurrentDateTime( dt );
    char     screen_path[MAX_FOPATH];
    Str::Format( screen_path, "screen_%04d.%02d.%02d_%02d-%02d-%02d.jpg",
                 dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second );

    if( Script::PrepareContext( ClientFunctions.FilenameScreenshot, _FUNC_, "Game" ) )
    {
        char*         str = screen_path;
        ScriptString* sstr = new ScriptString( str );
        Script::SetArgObject( sstr );
        if( Script::RunPrepared() )
            Str::Copy( screen_path, sstr->c_str() );
        sstr->Release();
    }

    if( Str::Compare( screen_path, "" ) )
        return false;

    FileManager::FormatPath( screen_path );
    FileManager::CreateDirectoryTree( FileManager::GetFullPath( screen_path, PATH_ROOT ) );

    #ifdef FO_D3D
    LPDIRECT3DSURFACE9 surf = NULL;
    if( FAILED( SprMngr.GetDevice()->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &surf ) ) )
        return false;

    D3DXIMAGE_FILEFORMAT format = D3DXIFF_BMP;
    char*                extension = (char*)FileManager::GetExtension( screen_path );
    Str::Lower( extension );
    if( Str::Compare( extension, "jpg" ) )
        format = D3DXIFF_JPG;
    else if( Str::Compare( extension, "tga" ) )
        format = D3DXIFF_TGA;
    else if( Str::Compare( extension, "png" ) )
        format = D3DXIFF_PNG;

    if( FAILED( D3DXSaveSurfaceToFile( screen_path, format, surf, NULL, NULL ) ) )
    {
        surf->Release();
        return false;
    }
    surf->Release();
    #else
    SprMngr.SaveTexture( NULL, screen_path, false );
    #endif

    return true;
}

void FOClient::SoundProcess()
{
    // Manager
    SndMngr.Process();

    // Ambient
    static uint next_ambient = 0;
    if( Timer::GameTick() > next_ambient )
    {
        if( IsMainScreen( CLIENT_MAIN_SCREEN_GAME ) )
            SndMngr.PlayAmbient( MsgGM->GetStr( STR_MAP_AMBIENT_( HexMngr.GetCurPidMap() ) ) );
        next_ambient = Timer::GameTick() + Random( AMBIENT_SOUND_TIME / 2, AMBIENT_SOUND_TIME );
    }
}

#ifndef FO_D3D
void FOClient::AddVideo( const char* video_name, bool can_stop, bool clear_sequence )
{
    // Stop current
    if( clear_sequence && ShowVideos.size() )
        StopVideo();

    // Show parameters
    ShowVideo sw;

    // Paths
    char  str[MAX_FOPATH];
    Str::Copy( str, video_name );
    char* sound = Str::Substring( str, "|" );
    if( sound )
    {
        *sound = 0;
        sound++;
        if( !Str::Substring( sound, "/" ) )
            sw.SoundName = FileManager::GetPath( PATH_VIDEO );
        sw.SoundName += sound;
    }
    if( !Str::Substring( str, "/" ) )
        sw.FileName = FileManager::GetPath( PATH_VIDEO );
    sw.FileName += str;

    // Add video in sequence
    sw.CanStop = can_stop;
    ShowVideos.push_back( sw );

    // Instant play
    if( ShowVideos.size() == 1 )
    {
        // Clear screen
        if( SprMngr.BeginScene( COLOR_XRGB( 0, 0, 0 ) ) )
            SprMngr.EndScene();

        // Play
        PlayVideo();
    }
}

void FOClient::PlayVideo()
{
    // Start new context
    ShowVideo& video = ShowVideos[0];
    CurVideo = new VideoContext();
    CurVideo->CurFrame = 0;
    CurVideo->StartTime = Timer::AccurateTick();
    CurVideo->AverageRenderTime = 0.0;

    // Open file
    if( !CurVideo->RawData.LoadFile( video.FileName.c_str(), PATH_DATA ) )
    {
        WriteLogF( _FUNC_, " - Video file<%s> not found.\n", video.FileName.c_str() );
        SAFEDEL( CurVideo );
        NextVideo();
        return;
    }

    // Initialize Theora stuff
    ogg_sync_init( &CurVideo->SyncState );

    for( uint i = 0; i < CurVideo->SS.COUNT; i++ )
        CurVideo->SS.StreamsState[i] = false;
    CurVideo->SS.MainIndex = -1;

    th_info_init( &CurVideo->VideoInfo );
    th_comment_init( &CurVideo->Comment );
    CurVideo->SetupInfo = NULL;
    while( true )
    {
        int stream_index = VideoDecodePacket();
        if( stream_index < 0 )
        {
            WriteLogF( _FUNC_, " - Decode header packet fail.\n" );
            SAFEDEL( CurVideo );
            NextVideo();
            return;
        }
        int r = th_decode_headerin( &CurVideo->VideoInfo, &CurVideo->Comment, &CurVideo->SetupInfo, &CurVideo->Packet );
        if( !r )
        {
            if( stream_index != CurVideo->SS.MainIndex )
            {
                while( true )
                {
                    stream_index = VideoDecodePacket();
                    if( stream_index == CurVideo->SS.MainIndex )
                        break;
                    if( stream_index < 0 )
                    {
                        WriteLogF( _FUNC_, " - Seek first data packet fail.\n" );
                        SAFEDEL( CurVideo );
                        NextVideo();
                        return;
                    }
                }
            }
            break;
        }
        else
        {
            CurVideo->SS.MainIndex = stream_index;
        }
    }

    CurVideo->Context = th_decode_alloc( &CurVideo->VideoInfo, CurVideo->SetupInfo );

    // Create texture
    if( !SprMngr.CreateRenderTarget( CurVideo->RT, false, false, CurVideo->VideoInfo.pic_width, CurVideo->VideoInfo.pic_height ) )
    {
        WriteLogF( _FUNC_, " - Can't create render target.\n" );
        SAFEDEL( CurVideo );
        NextVideo();
        return;
    }

    // Start sound
    if( video.SoundName != "" )
    {
        MusicVolumeRestore = SndMngr.GetMusicVolume();
        SndMngr.SetMusicVolume( 100 );
        SndMngr.PlayMusic( video.SoundName.c_str(), 0, 0 );
    }
}

int FOClient::VideoDecodePacket()
{
    ogg_stream_state* streams = CurVideo->SS.Streams;
    bool*             initiated_stream = CurVideo->SS.StreamsState;
    uint              num_streams = CurVideo->SS.COUNT;
    ogg_packet*       packet = &CurVideo->Packet;

    int               b = 0;
    int               rv = 0;
    for( uint i = 0; initiated_stream[i] && i < num_streams; i++ )
    {
        int a = ogg_stream_packetout( &streams[i], packet );
        switch( a )
        {
            case  1:
                b = i + 1;
            case  0:
            case -1:
                break;
        }
    }
    if( b )
        return rv = b - 1;

    do
    {
        ogg_page op;
        while( ogg_sync_pageout( &CurVideo->SyncState, &op ) != 1 )
        {
            int left = CurVideo->RawData.GetFsize() - CurVideo->RawData.GetCurPos();
            int bytes = min( 1024, left );
            if( !bytes )
                return -2;
            char* buffer = ogg_sync_buffer( &CurVideo->SyncState, bytes );
            CurVideo->RawData.CopyMem( buffer, bytes );
            ogg_sync_wrote( &CurVideo->SyncState, bytes );
        }

        if( ogg_page_bos( &op ) && rv != 1 )
        {
            uint i = 0;
            while( initiated_stream[i] && i < num_streams )
                i++;
            if( !initiated_stream[i] )
            {
                int a = ogg_stream_init( &streams[i], ogg_page_serialno( &op ) );
                initiated_stream[i] = true;
                if( a )
                    return -1;
                else
                    rv = 1;
            }
        }

        for( uint i = 0; initiated_stream[i] && i < num_streams; i++ )
        {
            ogg_stream_pagein( &streams[i], &op );
            int a = ogg_stream_packetout( &streams[i], packet );
            switch( a )
            {
                case  1:
                    rv = i;
                    b = i + 1;
                case  0:
                case -1:
                    break;
            }
        }
    }
    while( !b );

    return rv;
}

void FOClient::RenderVideo()
{
    // Count function call time to interpolate output time
    double render_time = Timer::AccurateTick();

    // Calculate next frame
    double cur_second = (render_time - CurVideo->StartTime + CurVideo->AverageRenderTime) / 1000.0;
    uint   new_frame = (uint)floor( cur_second * (double)CurVideo->VideoInfo.fps_numerator / (double)CurVideo->VideoInfo.fps_denominator + 0.5 );
    uint   skip_frames = new_frame - CurVideo->CurFrame;
    if( !skip_frames )
        return;
    bool last_frame = false;
    CurVideo->CurFrame = new_frame;

    // Process frames
    for( uint f = 0; f < skip_frames; f++ )
    {
        // Decode frame
        int r = th_decode_packetin( CurVideo->Context, &CurVideo->Packet, NULL );
        if( r != TH_DUPFRAME )
        {
            if( r )
            {
                WriteLogF( _FUNC_, " - Frame does not contain encoded video data, error<%d>.\n", r );
                NextVideo();
                return;
            }

            // Decode color
            r = th_decode_ycbcr_out( CurVideo->Context, CurVideo->ColorBuffer );
            if( r )
            {
                WriteLogF( _FUNC_, " - th_decode_ycbcr_out() fail, error<%d>\n.", r );
                NextVideo();
                return;
            }
        }

        // Seek next packet
        do
        {
            r = VideoDecodePacket();
            if( r == -2 )
            {
                last_frame = true;
                break;
            }
        }
        while( r != CurVideo->SS.MainIndex );
        if( last_frame )
            break;
    }

    // Data offsets
    char di, dj;
    switch( CurVideo->VideoInfo.pixel_fmt )
    {
        case TH_PF_420:
            di = 2;
            dj = 2;
            break;
        case TH_PF_422:
            di = 2;
            dj = 1;
            break;
        case TH_PF_444:
            di = 1;
            dj = 1;
            break;
        default:
            WriteLogF( _FUNC_, " - Wrong pixel format.\n" );
            NextVideo();
            return;
    }

    // Render
    uint w = CurVideo->VideoInfo.pic_width;
    uint h = CurVideo->VideoInfo.pic_height;
    SprMngr.PushRenderTarget( CurVideo->RT );
    GL( glMatrixMode( GL_PROJECTION ) );
    GL( glLoadIdentity() );
    GL( gluOrtho2D( 0, w, h, 0 ) );
    GL( glMatrixMode( GL_MODELVIEW ) );
    GL( glLoadIdentity() );
    GL( glDisable( GL_TEXTURE_2D ) );
    GL( glDisable( GL_POINT_SMOOTH ) );
    glBegin( GL_POINTS );
    for( uint y = 0; y < h; y++ )
    {
        for( uint x = 0; x < w; x++ )
        {
            // Get YUV
            th_ycbcr_buffer& cbuf = CurVideo->ColorBuffer;
            uint8            cy = *(cbuf[0].data + y * cbuf[0].stride + x);
            uint8            cu = *(cbuf[1].data + y / dj * cbuf[1].stride + x / di);
            uint8            cv = *(cbuf[2].data + y / dj * cbuf[2].stride + x / di);

            // Convert YUV to RGB
            float cr = cy + 1.402f * (cv - 127);
            float cg = cy - 0.344f * (cu - 127) - 0.714f * (cv - 127);
            float cb = cy + 1.722f * (cu - 127);

            // Draw
            glColor3f( cr / 255.0f, cg / 255.0f, cb / 255.0f );
            glVertex2f( (float)x, (float)y + 1.0f );
        }
    }
    GL( glEnd() );
    GL( glEnable( GL_TEXTURE_2D ) );
    GL( glEnable( GL_POINT_SMOOTH ) );
    SprMngr.PopRenderTarget();

    // Render to window
    float mw = (float)MODE_WIDTH;
    float mh = (float)MODE_HEIGHT;
    float k = min( mw / w, mh / h );
    w = (uint)( (float)w * k );
    h = (uint)( (float)h * k );
    int x = (MODE_WIDTH - w) / 2;
    int y = (MODE_HEIGHT - h) / 2;
    if( SprMngr.BeginScene( COLOR_XRGB( 0, 0, 0 ) ) )
    {
        Rect r = Rect( x, y, x + w, y + h );
        SprMngr.DrawRenderTarget( CurVideo->RT, false, NULL, &r );
        SprMngr.EndScene();
    }

    // Store render time
    render_time = Timer::AccurateTick() - render_time;
    if( CurVideo->AverageRenderTime > 0.0 )
        CurVideo->AverageRenderTime = (CurVideo->AverageRenderTime + render_time) / 2.0;
    else
        CurVideo->AverageRenderTime = render_time;

    // Last frame
    if( last_frame )
        NextVideo();
}

void FOClient::NextVideo()
{
    if( ShowVideos.size() )
    {
        // Clear screen
        if( SprMngr.BeginScene( COLOR_XRGB( 0, 0, 0 ) ) )
            SprMngr.EndScene();

        // Stop current
        StopVideo();
        ShowVideos.erase( ShowVideos.begin() );

        // Manage next
        if( ShowVideos.size() )
        {
            PlayVideo();
        }
        else
        {
            ScreenFadeOut();
            if( MusicAfterVideo != "" )
            {
                SndMngr.PlayMusic( MusicAfterVideo.c_str() );
                MusicAfterVideo = "";
            }
        }
    }
}

void FOClient::StopVideo()
{
    // Video
    if( CurVideo )
    {
        ogg_sync_clear( &CurVideo->SyncState );
        th_info_clear( &CurVideo->VideoInfo );
        th_setup_free( CurVideo->SetupInfo );
        th_decode_free( CurVideo->Context );
        SprMngr.DeleteRenderTarget( CurVideo->RT );
        SAFEDEL( CurVideo );
    }

    // Music
    SndMngr.StopMusic();
    if( MusicVolumeRestore != -1 )
    {
        SndMngr.SetMusicVolume( MusicVolumeRestore );
        MusicVolumeRestore = -1;
    }
}
#endif

uint FOClient::AnimLoad( uint name_hash, uint8 dir, int res_type )
{
    AnyFrames* anim = ResMngr.GetAnim( name_hash, dir, res_type );
    if( !anim )
        return 0;
    IfaceAnim* ianim = new IfaceAnim( anim, res_type );
    if( !ianim )
        return 0;

    uint index = 1;
    for( uint j = (uint)Animations.size(); index < j; index++ )
        if( !Animations[index] )
            break;
    if( index < (uint)Animations.size() )
        Animations[index] = ianim;
    else
        Animations.push_back( ianim );
    return index;
}

uint FOClient::AnimLoad( const char* fname, int path_type, int res_type )
{
    char full_name[MAX_FOPATH];
    Str::Copy( full_name, FileManager::GetPath( path_type ) );
    Str::Append( full_name, fname );

    AnyFrames* anim = ResMngr.GetAnim( Str::GetHash( full_name ), 0, res_type );
    if( !anim )
        return 0;
    IfaceAnim* ianim = new IfaceAnim( anim, res_type );
    if( !ianim )
        return 0;

    uint index = 1;
    for( uint j = (uint)Animations.size(); index < j; index++ )
        if( !Animations[index] )
            break;
    if( index < (uint)Animations.size() )
        Animations[index] = ianim;
    else
        Animations.push_back( ianim );
    return index;
}

uint FOClient::AnimGetCurSpr( uint anim_id )
{
    if( anim_id >= Animations.size() || !Animations[anim_id] )
        return 0;
    return Animations[anim_id]->Frames->Ind[Animations[anim_id]->CurSpr];
}

uint FOClient::AnimGetCurSprCnt( uint anim_id )
{
    if( anim_id >= Animations.size() || !Animations[anim_id] )
        return 0;
    return Animations[anim_id]->CurSpr;
}

uint FOClient::AnimGetSprCount( uint anim_id )
{
    if( anim_id >= Animations.size() || !Animations[anim_id] )
        return 0;
    return Animations[anim_id]->Frames->CntFrm;
}

AnyFrames* FOClient::AnimGetFrames( uint anim_id )
{
    if( anim_id >= Animations.size() || !Animations[anim_id] )
        return 0;
    return Animations[anim_id]->Frames;
}

void FOClient::AnimRun( uint anim_id, uint flags )
{
    if( anim_id >= Animations.size() || !Animations[anim_id] )
        return;
    IfaceAnim* anim = Animations[anim_id];

    // Set flags
    anim->Flags = flags & 0xFFFF;
    flags >>= 16;

    // Set frm
    uint8 cur_frm = flags & 0xFF;
    if( cur_frm > 0 )
    {
        cur_frm--;
        if( cur_frm >= anim->Frames->CntFrm )
            cur_frm = anim->Frames->CntFrm - 1;
        anim->CurSpr = cur_frm;
    }
    // flags>>=8;
}

void FOClient::AnimProcess()
{
    for( auto it = Animations.begin(), end = Animations.end(); it != end; ++it )
    {
        IfaceAnim* anim = *it;
        if( !anim || !anim->Flags )
            continue;

        if( FLAG( anim->Flags, ANIMRUN_STOP ) )
        {
            anim->Flags = 0;
            continue;
        }

        if( FLAG( anim->Flags, ANIMRUN_TO_END ) || FLAG( anim->Flags, ANIMRUN_FROM_END ) )
        {
            uint cur_tick = Timer::GameTick();
            if( cur_tick - anim->LastTick < anim->Frames->Ticks / anim->Frames->CntFrm )
                continue;

            anim->LastTick = cur_tick;
            uint end_spr = anim->Frames->CntFrm - 1;
            if( FLAG( anim->Flags, ANIMRUN_FROM_END ) )
                end_spr = 0;

            if( anim->CurSpr < end_spr )
                anim->CurSpr++;
            else if( anim->CurSpr > end_spr )
                anim->CurSpr--;
            else
            {
                if( FLAG( anim->Flags, ANIMRUN_CYCLE ) )
                {
                    if( FLAG( anim->Flags, ANIMRUN_TO_END ) )
                        anim->CurSpr = 0;
                    else
                        anim->CurSpr = end_spr;
                }
                else
                {
                    anim->Flags = 0;
                }
            }
        }
    }
}

void FOClient::AnimFree( int res_type )
{
    ResMngr.FreeResources( res_type );
    for( auto it = Animations.begin(), end = Animations.end(); it != end; ++it )
    {
        IfaceAnim* anim = *it;
        if( anim && anim->ResType == res_type )
        {
            delete anim;
            (*it) = NULL;
        }
    }
}

/************************************************************************/
/* Scripts                                                              */
/************************************************************************/

#include "ScriptPragmas.h"

bool FOClient::ReloadScripts( bool from_init /* = false */ )
{
    WriteLog( "Load scripts...\n" );

    FOMsg& msg_script = CurLang.Msg[TEXTMSG_INTERNAL];
    if( !msg_script.Count( STR_INTERNAL_SCRIPT_CONFIG ) ||
        !msg_script.Count( STR_INTERNAL_GAME_ENGINE_STAGE ) ||
        !msg_script.Count( STR_INTERNAL_GAME_ENGINE_VERSION ) ||
        !msg_script.Count( STR_INTERNAL_GAME_ENGINE_VERSION_CUSTOM ) ||
        !msg_script.Count( STR_INTERNAL_SCRIPT_ENGINE_VERSION ) ||
        !msg_script.Count( STR_INTERNAL_SCRIPT_MODULES ) ||
        !msg_script.Count( STR_INTERNAL_SCRIPT_MODULES + 1 ) )
    {
        WriteLog( "Main script section not found in MSG.\n" );
        if( !from_init )
            AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    bool old = false;
    if( !old )
        old = msg_script.GetInt( STR_INTERNAL_GAME_ENGINE_STAGE ) != FOCLASSIC_STAGE;
    if( !old )
        old = msg_script.GetInt( STR_INTERNAL_GAME_ENGINE_VERSION ) != FOCLASSIC_VERSION;
    if( !old )
        old = msg_script.GetInt( STR_INTERNAL_GAME_ENGINE_VERSION_CUSTOM ) != FOCLASSIC_VERSION_CUSTOM;
    if( !old )
        old = msg_script.GetInt( STR_INTERNAL_SCRIPT_ENGINE_VERSION ) != ANGELSCRIPT_VERSION;

    if( old )
    {
        WriteLog( "Old version of scripts.\n" );
        if( !from_init )
            AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    // Reinitialize engine
    Script::Finish();
    if( !Script::Init( false, APP_TYPE_CLIENT ) )
    {
        WriteLog( "Unable to start script engine.\n" );
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    // Bind vars and functions, see ScriptBind.cpp
    asIScriptEngine* engine = Script::GetEngine();
    if( !Script::RegisterAll( engine, APP_TYPE_CLIENT ) )
    {
        WriteLog( "Bind fail\n" );
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    // Options
    Script::SetScriptsPath( PATH_CACHE );
    Script::Define( "__CLIENT" );

    // Store dlls
    for( int i = STR_INTERNAL_SCRIPT_DLLS; ; i += 2 )
    {
        if( !msg_script.Count( i ) || !msg_script.Count( i + 1 ) )
            break;

        const char*  dll_name = msg_script.GetStr( i );
        uint         len;
        const uint8* dll_binary = msg_script.GetBinary( i + 1, len );
        if( !dll_binary )
            break;

        // Fix slashes
        char dll_name_[MAX_FOPATH];
        Str::Copy( dll_name_, dll_name );
        Str::Replacement( dll_name_, '\\', '.' );
        Str::Replacement( dll_name_, '/', '.' );
        dll_name = dll_name_;

        // Save to cache
        FileManager dll;
        if( dll.LoadStream( dll_binary, len ) )
        {
            dll.SwitchToWrite();
            dll.SaveOutBufToFile( dll_name, PATH_CACHE );
        }
    }

    // Pragmas
    StrVec pragmas;
    for( int i = STR_INTERNAL_SCRIPT_PRAGMAS; ; i += 2 )
    {
        if( !msg_script.Count( i ) || !msg_script.Count( i + 1 ) )
            break;
        pragmas.push_back( msg_script.GetStr( i ) );
        pragmas.push_back( msg_script.GetStr( i + 1 ) );
    }
    Script::CallPragmas( pragmas );

    // Load modules
    int errors = 0;
    for( int i = STR_INTERNAL_SCRIPT_MODULES; ; i += 2 )
    {
        if( !msg_script.Count( i ) || !msg_script.Count( i + 1 ) )
            break;

        const char*  module_name = msg_script.GetStr( i );
        uint         len;
        const uint8* bytecode = msg_script.GetBinary( i + 1, len );
        if( !bytecode )
            break;

        if( !Script::LoadScript( module_name, bytecode, len ) )
        {
            WriteLog( "Load script<%s> fail.\n", module_name );
            errors++;
        }
    }

    if( errors )
    {
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    // Bind functions
    errors = Script::BindImportedFunctions();

    if( errors )
    {
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    errors = Script::RebindFunctions();

    // Copy partial scripts.cfg from FOINTERNAL.MSG into ConfigFile
    Ini* scripts_cfg = new Ini();
    scripts_cfg->KeepKeysOrder = true;
    scripts_cfg->LoadStr( string( msg_script.GetStr( STR_INTERNAL_SCRIPT_CONFIG ) ) );
    ConfigFile->RemoveSection( SECTION_CLIENT_SCRIPTS_MODULES );
    ConfigFile->RemoveSection( SECTION_CLIENT_SCRIPTS_BINDS );
    Script::LoadConfigFile( scripts_cfg, ConfigFile, SECTION_CLIENT_SCRIPTS_MODULES, SECTION_CLIENT_SCRIPTS_BINDS );
    delete scripts_cfg;

    // Bind reserved functions
    ReservedFunctionsMap reserved_functions = GetClientFunctionsMap();
    if( !Script::BindReservedFunctions( SECTION_CLIENT_SCRIPTS_BINDS, "client", reserved_functions ) )
        errors++;

    if( errors )
    {
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    AnimFree( RES_SCRIPT );

    if( !Script::PrepareContext( ClientFunctions.Start, _FUNC_, "Game" ) || !Script::RunPrepared() || Script::GetReturnedBool() == false )
    {
        WriteLog( "Execute start script fail.\n" );
        AddMess( MSGBOX_GAME, MsgGame->GetStr( STR_NET_FAIL_RUN_START_SCRIPT ) );
        return false;
    }

    WriteLog( "Load scripts complete.\n" );
    return true;
}

int FOClient::ScriptGetHitProc( CritterCl* cr, int hit_location )
{
    if( !Chosen )
        return 0;
    int        use = Chosen->GetUse();
    ProtoItem* proto_item = Chosen->ItemSlotMain->Proto;
    if( !proto_item->IsWeapon() || !Chosen->ItemSlotMain->WeapIsUseAviable( use ) )
        return 0;

    if( Script::PrepareContext( ClientFunctions.ToHit, _FUNC_, "Game" ) )
    {
        Script::SetArgObject( Chosen );
        Script::SetArgObject( cr );
        Script::SetArgObject( proto_item );
        Script::SetArgUChar( MAKE_ITEM_MODE( use, hit_location ) );
        if( Script::RunPrepared() )
            return Script::GetReturnedUInt();
    }
    return 0;
}

void FOClient::DrawIfaceLayer( uint layer )
{
    if( Script::PrepareContext( ClientFunctions.RenderIface, _FUNC_, "Game" ) )
    {
        SpritesCanDraw = true;
        Script::SetArgUInt( layer );
        Script::RunPrepared();
        SpritesCanDraw = false;
    }
}

int SortCritterHx_ = 0, SortCritterHy_ = 0;
bool SortCritterByDistPred( CritterCl* cr1, CritterCl* cr2 )
{
    return DistGame( SortCritterHx_, SortCritterHy_, cr1->GetHexX(), cr1->GetHexY() ) < DistGame( SortCritterHx_, SortCritterHy_, cr2->GetHexX(), cr2->GetHexY() );
}
void SortCritterByDist( CritterCl* cr, CritVec& critters )
{
    SortCritterHx_ = cr->GetHexX();
    SortCritterHy_ = cr->GetHexY();
    std::sort( critters.begin(), critters.end(), SortCritterByDistPred );
}
void SortCritterByDist( int hx, int hy, CritVec& critters )
{
    SortCritterHx_ = hx;
    SortCritterHy_ = hy;
    std::sort( critters.begin(), critters.end(), SortCritterByDistPred );
}

#define SCRIPT_ERROR( error )          do { ScriptLastError = error; Script::LogError( _FUNC_, error ); } while( 0 )
#define SCRIPT_ERROR_RX( error, x )    do { ScriptLastError = error; Script::LogError( _FUNC_, error ); return x; } while( 0 )
#define SCRIPT_ERROR_R( error )        do { ScriptLastError = error; Script::LogError( _FUNC_, error ); return; } while( 0 )
#define SCRIPT_ERROR_R0( error )       do { ScriptLastError = error; Script::LogError( _FUNC_, error ); return 0; } while( 0 )
static string ScriptLastError;

int* FOClient::SScriptFunc::DataRef_Index( CritterClPtr& cr, uint index )
{
    static int dummy = 0;
    if( cr->IsNotValid )
        SCRIPT_ERROR_RX( "This nulltptr.", &dummy );
    if( index >= MAX_PARAMS )
        SCRIPT_ERROR_RX( "Invalid index arg.", &dummy );
    uint data_index = ( (uint) & cr - (uint) & cr->ThisPtr[0] ) / sizeof(cr->ThisPtr[0]);
    if( CritterCl::ParametersOffset[data_index] )
        index += CritterCl::ParametersMin[data_index];
    if( index < CritterCl::ParametersMin[data_index] )
        SCRIPT_ERROR_RX( "Index is less than minimum.", &dummy );
    if( index > CritterCl::ParametersMax[data_index] )
        SCRIPT_ERROR_RX( "Index is greater than maximum.", &dummy );
    return &cr->Params[index];
}

int FOClient::SScriptFunc::DataVal_Index( CritterClPtr& cr, uint index )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nulltptr." );
    if( index >= MAX_PARAMS )
        SCRIPT_ERROR_R0( "Invalid index arg." );
    uint data_index = ( (uint) & cr - (uint) & cr->ThisPtr[0] ) / sizeof(cr->ThisPtr[0]);
    if( CritterCl::ParametersOffset[data_index] )
        index += CritterCl::ParametersMin[data_index];
    if( index < CritterCl::ParametersMin[data_index] )
        SCRIPT_ERROR_R0( "Index is less than minimum." );
    if( index > CritterCl::ParametersMax[data_index] )
        SCRIPT_ERROR_R0( "Index is greater than maximum." );
    return cr->GetParam( index );
}

bool FOClient::SScriptFunc::Crit_IsChosen( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsChosen();
}

bool FOClient::SScriptFunc::Crit_IsPlayer( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsPlayer();
}

bool FOClient::SScriptFunc::Crit_IsNpc( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsNpc();
}

bool FOClient::SScriptFunc::Crit_IsLife( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsLife();
}

bool FOClient::SScriptFunc::Crit_IsKnockout( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsKnockout();
}

bool FOClient::SScriptFunc::Crit_IsDead( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsDead();
}

bool FOClient::SScriptFunc::Crit_IsFree( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsFree();
}

bool FOClient::SScriptFunc::Crit_IsBusy( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return !cr->IsFree();
}

bool FOClient::SScriptFunc::Crit_IsAnim3d( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->Anim3d != NULL;
}

bool FOClient::SScriptFunc::Crit_IsAnimAviable( CritterCl* cr, uint anim1, uint anim2 )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsAnimAviable( anim1, anim2 );
}

bool FOClient::SScriptFunc::Crit_IsAnimPlaying( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->IsAnim();
}

uint FOClient::SScriptFunc::Crit_GetAnim1( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->GetAnim1();
}

void FOClient::SScriptFunc::Crit_Animate( CritterCl* cr, uint anim1, uint anim2 )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    cr->Animate( anim1, anim2, NULL );
}

void FOClient::SScriptFunc::Crit_AnimateEx( CritterCl* cr, uint anim1, uint anim2, Item* item )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    cr->Animate( anim1, anim2, item );
}

void FOClient::SScriptFunc::Crit_ClearAnim( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    cr->ClearAnim();
}

void FOClient::SScriptFunc::Crit_Wait( CritterCl* cr, uint ms )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    cr->TickStart( ms );
}

uint FOClient::SScriptFunc::Crit_ItemsCount( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->GetItemsCount();
}

uint FOClient::SScriptFunc::Crit_ItemsWeight( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->GetItemsWeight();
}

uint FOClient::SScriptFunc::Crit_ItemsVolume( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->GetItemsVolume();
}

uint FOClient::SScriptFunc::Crit_CountItem( CritterCl* cr, uint16 proto_id )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->CountItemPid( proto_id );
}

uint FOClient::SScriptFunc::Crit_CountItemByType( CritterCl* cr, uint8 type )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->CountItemType( type );
}

Item* FOClient::SScriptFunc::Crit_GetItem( CritterCl* cr, uint16 proto_id, int slot )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    if( proto_id && slot >= 0 && slot < SLOT_GROUND )
        cr->GetItemByPidSlot( proto_id, slot );
    else if( proto_id )
        return cr->GetItemByPidInvPriority( proto_id );
    else if( slot >= 0 && slot < SLOT_GROUND )
        return cr->GetItemSlot( slot );
    return NULL;
}

uint FOClient::SScriptFunc::Crit_GetItems( CritterCl* cr, int slot, ScriptArray* items )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    ItemPtrVec items_;
    cr->GetItemsSlot( slot, items_ );
    if( items )
        Script::AppendVectorToArrayRef<Item*>( items_, items );
    return (uint)items_.size();
}

uint FOClient::SScriptFunc::Crit_GetItemsByType( CritterCl* cr, int type, ScriptArray* items )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    ItemPtrVec items_;
    cr->GetItemsType( type, items_ );
    if( items )
        Script::AppendVectorToArrayRef<Item*>( items_, items );
    return (uint)items_.size();
}

ProtoItem* FOClient::SScriptFunc::Crit_GetSlotProto( CritterCl* cr, int slot, uint8& mode )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );

    Item* item = NULL;
    switch( slot )
    {
        case SLOT_HAND1:
            item = cr->ItemSlotMain;
            break;
        case SLOT_HAND2:
            item = (cr->ItemSlotExt->GetId() ? cr->ItemSlotExt : cr->DefItemSlotHand);
            break;
        case SLOT_ARMOR:
            item = cr->ItemSlotArmor;
            break;
        default:
            item = cr->GetItemSlot( slot );
            break;
    }
    if( !item )
        return NULL;

    mode = item->Data.Mode;
    return item->Proto;
}

void FOClient::SScriptFunc::Crit_SetVisible( CritterCl* cr, bool visible )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    cr->Visible = visible;
    Self->HexMngr.RefreshMap();
}

bool FOClient::SScriptFunc::Crit_GetVisible( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->Visible;
}

void FOClient::SScriptFunc::Crit_set_ContourColor( CritterCl* cr, uint value )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    if( cr->SprDrawValid )
        cr->SprDraw->SetContour( cr->SprDraw->ContourType, value );
    cr->ContourColor = value;
}

uint FOClient::SScriptFunc::Crit_get_ContourColor( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->ContourColor;
}

uint FOClient::SScriptFunc::Crit_GetMultihex( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return cr->GetMultihex();
}

bool FOClient::SScriptFunc::Crit_IsTurnBasedTurn( CritterCl* cr )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return Self->IsTurnBased && cr->GetId() == Self->TurnBasedCurCritterId;
}

void FOClient::SScriptFunc::Crit_GetNameTextInfo( CritterCl* cr, bool& nameVisible, int& x, int& y, int& w, int& h, int& lines )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );

    cr->GetNameTextInfo( nameVisible, x, y, w, h, lines );
}

bool FOClient::SScriptFunc::Item_IsStackable( Item* item )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return item->IsStackable();
}

bool FOClient::SScriptFunc::Item_IsDeteriorable( Item* item )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return item->IsDeteriorable();
}

uint FOClient::SScriptFunc::Item_GetScriptId( Item* item )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return item->Data.ScriptId;
}

uint8 FOClient::SScriptFunc::Item_GetType( Item* item )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return item->GetType();
}

uint16 FOClient::SScriptFunc::Item_GetProtoId( Item* item )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return item->GetProtoId();
}

uint FOClient::SScriptFunc::Item_GetCount( Item* item )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    return item->GetCount();
}

bool FOClient::SScriptFunc::Item_GetMapPosition( Item* item, uint16& hx, uint16& hy )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    if( !Self->HexMngr.IsMapLoaded() )
        SCRIPT_ERROR_R0( "Map is not loaded." );
    switch( item->Accessory )
    {
        case ITEM_ACCESSORY_CRITTER:
        {
            CritterCl* cr = Self->GetCritter( item->AccCritter.Id );
            if( !cr )
                SCRIPT_ERROR_R0( "CritterCl accessory, CritterCl not found." );
            hx = cr->GetHexX();
            hy = cr->GetHexY();
            break;
        }
        case ITEM_ACCESSORY_HEX:
        {
            hx = item->AccHex.HexX;
            hy = item->AccHex.HexY;
            break;
        }
        case ITEM_ACCESSORY_CONTAINER:
        {

            if( item->GetId() == item->AccContainer.ContainerId )
                SCRIPT_ERROR_R0( "Container accessory, Crosslinks." );
            Item* cont = Self->GetItem( item->AccContainer.ContainerId );
            if( !cont )
                SCRIPT_ERROR_R0( "Container accessory, Container not found." );
            return Item_GetMapPosition( cont, hx, hy );         // Recursion
            break;
        }
        default:
            SCRIPT_ERROR_R0( "Unknown accessory." );
            return false;
    }
    return true;
}

void FOClient::SScriptFunc::Item_Animate( Item* item, uint8 from_frame, uint8 to_frame )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R( "This nullptr." );
    #pragma MESSAGE("Implement Item_Animate.")
}

Item* FOClient::SScriptFunc::Item_GetChild( Item* item, uint childIndex )
{
    if( item->IsNotValid )
        SCRIPT_ERROR_R0( "This nullptr." );
    // Need implement
    #pragma MESSAGE("Implement Item_GetChild.")
    return NULL;
}

bool FOClient::SScriptFunc::Global_get___ConsoleActive()
{
    return FOClient::ConsoleActive;
}

bool FOClient::SScriptFunc::Global_get___WorldmapActive()
{
    return FOClient::WorldmapActive;
}

bool FOClient::SScriptFunc::Global_get___WorldmapWait()
{
    return FOClient::WorldmapWait;
}

float FOClient::SScriptFunc::Global_get___WorldmapZoom()
{
    return FOClient::WorldmapZoom;
}

int FOClient::SScriptFunc::Global_get___WorldmapOffsetX()
{
    return FOClient::WorldmapOffsetX;
}

int FOClient::SScriptFunc::Global_get___WorldmapOffsetY()
{
    return FOClient::WorldmapOffsetY;
}

int FOClient::SScriptFunc::Global_get___WorldmapGroupCurX()
{
    return FOClient::WorldmapGroupCurX;
}

int FOClient::SScriptFunc::Global_get___WorldmapGroupCurY()
{
    return FOClient::WorldmapGroupCurY;
}

int FOClient::SScriptFunc::Global_get___WorldmapGroupToX()
{
    return FOClient::WorldmapGroupToX;
}

int FOClient::SScriptFunc::Global_get___WorldmapGroupToY()
{
    return FOClient::WorldmapGroupToY;
}

float FOClient::SScriptFunc::Global_get___WorldmapGroupSpeed()
{
    return FOClient::WorldmapGroupSpeed;
}

CritterCl* FOClient::SScriptFunc::Global_GetChosen()
{
    if( Self->Chosen && Self->Chosen->IsNotValid )
        return NULL;
    return Self->Chosen;
}

uint FOClient::SScriptFunc::Global_GetChosenActions( ScriptArray* actions )
{
    if( actions )
        actions->Resize( 0 );

    if( Self->Chosen )
    {
        actions->Resize( (uint)Self->ChosenAction.size() * 7 );
        for( uint i = 0, j = (uint)Self->ChosenAction.size(); i < j; i++ )
        {
            ActionEvent& act = Self->ChosenAction[i];
            *(uint*)actions->At( i * 7 + 0 ) = act.Type;
            *(uint*)actions->At( i * 7 + 1 ) = act.Param[0];
            *(uint*)actions->At( i * 7 + 2 ) = act.Param[1];
            *(uint*)actions->At( i * 7 + 3 ) = act.Param[2];
            *(uint*)actions->At( i * 7 + 4 ) = act.Param[3];
            *(uint*)actions->At( i * 7 + 5 ) = act.Param[4];
            *(uint*)actions->At( i * 7 + 6 ) = act.Param[5];
        }
        return (uint)Self->ChosenAction.size();
    }
    return 0;
}

void FOClient::SScriptFunc::Global_SetChosenActions( ScriptArray* actions )
{
    if( actions && actions->GetSize() % 7 )
        SCRIPT_ERROR_R( "Wrong action array size." );
    Self->ChosenAction.clear();

    if( Self->Chosen && actions )
    {
        Self->ChosenAction.resize( actions->GetSize() / 7 );
        for( uint i = 0, j = actions->GetSize() / 7; i < j; i++ )
        {
            ActionEvent& act = Self->ChosenAction[i];
            act.Type = *(uint*)actions->At( i * 7 + 0 );
            act.Param[0] = *(uint*)actions->At( i * 7 + 1 );
            act.Param[1] = *(uint*)actions->At( i * 7 + 2 );
            act.Param[2] = *(uint*)actions->At( i * 7 + 3 );
            act.Param[3] = *(uint*)actions->At( i * 7 + 4 );
            act.Param[4] = *(uint*)actions->At( i * 7 + 5 );
            act.Param[5] = *(uint*)actions->At( i * 7 + 6 );
        }
    }
}

Item* FOClient::SScriptFunc::Global_GetItem( uint item_id )
{
    if( !item_id )
        SCRIPT_ERROR_R0( "Item id arg is zero." );
    Item* item = Self->GetItem( item_id );
    if( !item || item->IsNotValid )
        return NULL;
    return item;
}

uint FOClient::SScriptFunc::Global_GetCrittersDistantion( CritterCl* cr1, CritterCl* cr2 )
{
    if( !Self->HexMngr.IsMapLoaded() )
        SCRIPT_ERROR_RX( "Map is not loaded.", -1 );
    if( cr1->IsNotValid )
        SCRIPT_ERROR_RX( "Critter1 arg nullptr.", -1 );
    if( cr2->IsNotValid )
        SCRIPT_ERROR_RX( "Critter2 arg nullptr.", -1 );
    return DistGame( cr1->GetHexX(), cr1->GetHexY(), cr2->GetHexX(), cr2->GetHexY() );
}

CritterCl* FOClient::SScriptFunc::Global_GetCritter( uint critter_id )
{
    if( !critter_id )
        return NULL;                // SCRIPT_ERROR_R0("CritterCl id arg is zero.");
    CritterCl* cr = Self->GetCritter( critter_id );
    if( !cr || cr->IsNotValid )
        return NULL;
    return cr;
}

uint FOClient::SScriptFunc::Global_GetCritters( uint16 hx, uint16 hy, uint radius, int find_type, ScriptArray* critters )
{
    if( hx >= Self->HexMngr.GetMaxHexX() || hy >= Self->HexMngr.GetMaxHexY() )
        SCRIPT_ERROR_R0( "Invalid hexes args." );

    CritMap& crits = Self->HexMngr.GetCritters();
    CritVec  cr_vec;
    for( auto it = crits.begin(), end = crits.end(); it != end; ++it )
    {
        CritterCl* cr = (*it).second;
        if( cr->CheckFind( find_type ) && CheckDist( hx, hy, cr->GetHexX(), cr->GetHexY(), radius ) )
            cr_vec.push_back( cr );
    }

    if( critters )
    {
        SortCritterByDist( hx, hy, cr_vec );
        Script::AppendVectorToArrayRef<CritterCl*>( cr_vec, critters );
    }
    return (uint)cr_vec.size();
}

uint FOClient::SScriptFunc::Global_GetCrittersByPids( uint16 pid, int find_type, ScriptArray* critters )
{
    CritMap& crits = Self->HexMngr.GetCritters();
    CritVec  cr_vec;
    if( !pid )
    {
        for( auto it = crits.begin(), end = crits.end(); it != end; ++it )
        {
            CritterCl* cr = (*it).second;
            if( cr->CheckFind( find_type ) )
                cr_vec.push_back( cr );
        }
    }
    else
    {
        for( auto it = crits.begin(), end = crits.end(); it != end; ++it )
        {
            CritterCl* cr = (*it).second;
            if( cr->IsNpc() && cr->Pid == pid && cr->CheckFind( find_type ) )
                cr_vec.push_back( cr );
        }
    }
    if( cr_vec.empty() )
        return 0;
    if( critters )
        Script::AppendVectorToArrayRef<CritterCl*>( cr_vec, critters );
    return (uint)cr_vec.size();
}

uint FOClient::SScriptFunc::Global_GetCrittersInPath( uint16 from_hx, uint16 from_hy, uint16 to_hx, uint16 to_hy, float angle, uint dist, int find_type, ScriptArray* critters )
{
    CritVec cr_vec;
    Self->HexMngr.TraceBullet( from_hx, from_hy, to_hx, to_hy, dist, angle, NULL, false, &cr_vec, FIND_LIFE | FIND_KO, NULL, NULL, NULL, true );
    if( critters )
        Script::AppendVectorToArrayRef<CritterCl*>( cr_vec, critters );
    return (uint)cr_vec.size();
}

uint FOClient::SScriptFunc::Global_GetCrittersInPathBlock( uint16 from_hx, uint16 from_hy, uint16 to_hx, uint16 to_hy, float angle, uint dist, int find_type, ScriptArray* critters, uint16& pre_block_hx, uint16& pre_block_hy, uint16& block_hx, uint16& block_hy )
{
    CritVec    cr_vec;
    UInt16Pair block, pre_block;
    Self->HexMngr.TraceBullet( from_hx, from_hy, to_hx, to_hy, dist, angle, NULL, false, &cr_vec, FIND_LIFE | FIND_KO, &block, &pre_block, NULL, true );
    if( critters )
        Script::AppendVectorToArrayRef<CritterCl*>( cr_vec, critters );
    pre_block_hx = pre_block.first;
    pre_block_hy = pre_block.second;
    block_hx = block.first;
    block_hy = block.second;
    return (uint)cr_vec.size();
}

void FOClient::SScriptFunc::Global_GetHexInPath( uint16 from_hx, uint16 from_hy, uint16& to_hx, uint16& to_hy, float angle, uint dist )
{
    UInt16Pair pre_block, block;
    Self->HexMngr.TraceBullet( from_hx, from_hy, to_hx, to_hy, dist, angle, NULL, false, NULL, 0, &block, &pre_block, NULL, true );
    to_hx = pre_block.first;
    to_hy = pre_block.second;
}

uint FOClient::SScriptFunc::Global_GetPathLengthHex( uint16 from_hx, uint16 from_hy, uint16 to_hx, uint16 to_hy, uint cut )
{
    if( from_hx >= Self->HexMngr.GetMaxHexX() || from_hy >= Self->HexMngr.GetMaxHexY() )
        SCRIPT_ERROR_R0( "Invalid from hexes args." );
    if( to_hx >= Self->HexMngr.GetMaxHexX() || to_hy >= Self->HexMngr.GetMaxHexY() )
        SCRIPT_ERROR_R0( "Invalid to hexes args." );

    if( cut > 0 && !Self->HexMngr.CutPath( NULL, from_hx, from_hy, to_hx, to_hy, cut ) )
        return 0;
    UInt8Vec steps;
    if( !Self->HexMngr.FindPath( NULL, from_hx, from_hy, to_hx, to_hy, steps, -1 ) )
        steps.clear();
    return (uint)steps.size();
}

uint FOClient::SScriptFunc::Global_GetPathLengthCr( CritterCl* cr, uint16 to_hx, uint16 to_hy, uint cut )
{
    if( cr->IsNotValid )
        SCRIPT_ERROR_R0( "Critter arg nullptr." );
    if( to_hx >= Self->HexMngr.GetMaxHexX() || to_hy >= Self->HexMngr.GetMaxHexY() )
        SCRIPT_ERROR_R0( "Invalid to hexes args." );

    if( cut > 0 && !Self->HexMngr.CutPath( cr, cr->GetHexX(), cr->GetHexY(), to_hx, to_hy, cut ) )
        return 0;
    UInt8Vec steps;
    if( !Self->HexMngr.FindPath( cr, cr->GetHexX(), cr->GetHexY(), to_hx, to_hy, steps, -1 ) )
        steps.clear();
    return (uint)steps.size();
}

void FOClient::SScriptFunc::Global_FlushScreen( uint from_color, uint to_color, uint ms )
{
    Self->ScreenFade( ms, from_color, to_color, true );
}

void FOClient::SScriptFunc::Global_QuakeScreen( uint noise, uint ms )
{
    Self->ScreenQuake( noise, ms );
}

bool FOClient::SScriptFunc::Global_PlaySound( ScriptString& sound_name )
{
    return SndMngr.PlaySound( sound_name.c_str() );
}

bool FOClient::SScriptFunc::Global_PlaySoundType( uint8 sound_type, uint8 sound_type_ext, uint8 sound_id, uint8 sound_id_ext )
{
    return SndMngr.PlaySoundType( sound_type, sound_type_ext, sound_id, sound_id_ext );
}

bool FOClient::SScriptFunc::Global_PlayMusic( ScriptString& music_name, uint pos, uint repeat )
{
    return SndMngr.PlayMusic( music_name.c_str(), pos, repeat );
}

void FOClient::SScriptFunc::Global_PlayVideo( ScriptString& video_name, bool can_stop )
{
    #ifndef FO_D3D
    SndMngr.StopMusic();
    Self->AddVideo( video_name.c_str(), can_stop, true );
    #endif
}

bool FOClient::SScriptFunc::Global_IsTurnBased()
{
    return Self->HexMngr.IsMapLoaded() && Self->IsTurnBased;
}

uint FOClient::SScriptFunc::Global_GetTurnBasedTime()
{
    if( Self->HexMngr.IsMapLoaded() && Self->IsTurnBased && Self->TurnBasedTime > Timer::GameTick() )
        return Self->TurnBasedTime - Timer::GameTick();
    return 0;
}

uint16 FOClient::SScriptFunc::Global_GetCurrentMapPid()
{
    if( !Self->HexMngr.IsMapLoaded() )
        return 0;
    return Self->HexMngr.GetCurPidMap();
}

uint FOClient::SScriptFunc::Global_GetMessageFilters( ScriptArray* filters )
{
    if( filters )
        Script::AppendVectorToArray( Self->MessBoxFilters, filters );
    return (uint)Self->MessBoxFilters.size();
}

void FOClient::SScriptFunc::Global_SetMessageFilters( ScriptArray* filters )
{
    Self->MessBoxFilters.clear();
    if( filters )
        Script::AssignScriptArrayInVector( Self->MessBoxFilters, filters );
}

void FOClient::SScriptFunc::Global_Message( ScriptString& msg )
{
    Self->AddMess( MSGBOX_GAME, msg.c_str() );
}

void FOClient::SScriptFunc::Global_MessageType( ScriptString& msg, int type )
{
    if( type < MSGBOX_GAME || type > MSGBOX_RADIO_HIDDEN)
        type = MSGBOX_GAME;
    Self->AddMess( type, msg.c_str() );
}

void FOClient::SScriptFunc::Global_MessageMsg( int text_msg, uint str_num )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_R( "Invalid text msg arg." );
    Self->AddMess( MSGBOX_GAME, Self->CurLang.Msg[text_msg].GetStr( str_num ) );
}

void FOClient::SScriptFunc::Global_MessageMsgType( int text_msg, uint str_num, int type )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_R( "Invalid text msg arg." );
    if( type < MSGBOX_GAME || type > MSGBOX_RADIO_HIDDEN)
        type = MSGBOX_GAME;
    Self->AddMess( type, Self->CurLang.Msg[text_msg].GetStr( str_num ) );
}

void FOClient::SScriptFunc::Global_MapMessage( ScriptString& text, uint16 hx, uint16 hy, uint ms, uint color, bool fade, int ox, int oy )
{
    FOClient::MapText t;
    t.HexX = hx;
    t.HexY = hy;
    t.Color = (color ? color : COLOR_TEXT);
    t.Fade = fade;
    t.StartTick = Timer::GameTick();
    t.Tick = ms;
    t.Text = text.c_std_str();
    t.Pos = Self->HexMngr.GetRectForText( hx, hy );
    t.EndPos = Rect( t.Pos, ox, oy );
    auto it = std::find( Self->GameMapTexts.begin(), Self->GameMapTexts.end(), t );
    //if( it != Self->GameMapTexts.end() )
    //    Self->GameMapTexts.erase( it );
    Self->GameMapTexts.push_back( t );
}

ScriptString* FOClient::SScriptFunc::Global_GetMsgStr( int text_msg, uint str_num )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_RX( "Invalid text msg arg.", new ScriptString( "" ) );
    return new ScriptString( text_msg == TEXTMSG_HOLO ? Self->GetHoloText( str_num ) : Self->CurLang.Msg[text_msg].GetStr( str_num ) );
}

ScriptString* FOClient::SScriptFunc::Global_GetMsgStrSkip( int text_msg, uint str_num, uint skip_count )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_RX( "Invalid text msg arg.", new ScriptString( "" ) );
    return new ScriptString( text_msg == TEXTMSG_HOLO ? Self->GetHoloText( str_num ) : Self->CurLang.Msg[text_msg].GetStr( str_num, skip_count ) );
}

uint FOClient::SScriptFunc::Global_GetMsgStrNumUpper( int text_msg, uint str_num )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_R0( "Invalid text msg arg." );
    return Self->CurLang.Msg[text_msg].GetStrNumUpper( str_num );
}

uint FOClient::SScriptFunc::Global_GetMsgStrNumLower( int text_msg, uint str_num )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_R0( "Invalid text msg arg." );
    return Self->CurLang.Msg[text_msg].GetStrNumLower( str_num );
}

uint FOClient::SScriptFunc::Global_GetMsgStrCount( int text_msg, uint str_num )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_R0( "Invalid text msg arg." );
    return Self->CurLang.Msg[text_msg].Count( str_num );
}

bool FOClient::SScriptFunc::Global_IsMsgStr( int text_msg, uint str_num )
{
    if( text_msg >= TEXTMSG_MAX )
        SCRIPT_ERROR_R0( "Invalid text msg arg." );
    return Self->CurLang.Msg[text_msg].Count( str_num ) > 0;
}

ScriptString* FOClient::SScriptFunc::Global_ReplaceTextStr( ScriptString& text, ScriptString& replace, ScriptString& str )
{
    size_t pos = text.c_std_str().find( replace.c_std_str(), 0 );
    if( pos == std::string::npos )
        return new ScriptString( text );
    string result = text.c_std_str();
    return new ScriptString( result.replace( pos, replace.length(), str.c_std_str() ) );
}

ScriptString* FOClient::SScriptFunc::Global_ReplaceTextInt( ScriptString& text, ScriptString& replace, int i )
{
    size_t pos = text.c_std_str().find( replace.c_std_str(), 0 );
    if( pos == std::string::npos )
        return new ScriptString( text );
    char   val[32];
    Str::Format( val, "%d", i );
    string result = text.c_std_str();
    return new ScriptString( result.replace( pos, replace.length(), val ) );
}

ScriptString* FOClient::SScriptFunc::Global_FormatTags( ScriptString& text, ScriptString* lexems )
{
    static char* buf = NULL;
    static uint  buf_len = 0;
    if( !buf )
    {
        buf = new char[MAX_FOTEXT];
        if( !buf )
            SCRIPT_ERROR_R0( "Allocation fail." );
        buf_len = MAX_FOTEXT;
    }

    if( buf_len < text.length() * 2 )
    {
        delete[] buf;
        buf = new char[text.length() * 2];
        if( !buf )
            SCRIPT_ERROR_R0( "Reallocation fail." );
        buf_len = (uint)text.length() * 2;
    }

    Str::Copy( buf, buf_len, text.c_str() );
    Self->FormatTags( buf, buf_len, Self->Chosen, NULL, lexems ? lexems->c_str() : NULL );
    return new ScriptString( buf );
}

int FOClient::SScriptFunc::Global_GetSomeValue( int var )
{
    switch( var )
    {
        case 0:
        case 1:
            break;
    }
    return 0;
}

void FOClient::SScriptFunc::Global_MoveScreen( uint16 hx, uint16 hy, uint speed )
{
    if( hx >= Self->HexMngr.GetMaxHexX() || hy >= Self->HexMngr.GetMaxHexY() )
        SCRIPT_ERROR_R( "Invalid hex args." );
    if( !Self->HexMngr.IsMapLoaded() )
        SCRIPT_ERROR_R( "Map is not loaded." );
    if( !speed )
        Self->HexMngr.FindSetCenter( hx, hy );
    else
        Self->HexMngr.ScrollToHex( hx, hy, double(speed) / 1000.0, false );
}

void FOClient::SScriptFunc::Global_LockScreenScroll( CritterCl* cr )
{
    if( cr && cr->IsNotValid )
        SCRIPT_ERROR_R( "CritterCl arg nullptr." );
    Self->HexMngr.AutoScroll.LockedCritter = (cr ? cr->GetId() : 0);
}

int FOClient::SScriptFunc::Global_GetFog( uint16 zone_x, uint16 zone_y )
{
    if( !Self->Chosen || Self->Chosen->IsNotValid )
        SCRIPT_ERROR_R0( "Chosen data not valid." );
    if( zone_x >= GameOpt.GlobalMapWidth || zone_y >= GameOpt.GlobalMapHeight )
        SCRIPT_ERROR_R0( "Invalid world map pos arg." );
    return Self->WorldmapFog.Get2Bit( zone_x, zone_y );
}

void FOClient::SScriptFunc::Global_RefreshItemsCollection( int collection )
{
    switch( collection )
    {
        case ITEM_COLLECTION_INVENTORY:
            Self->ProcessItemsCollection( ITEM_COLLECTION_INVENTORY, Self->InvContInit, Self->InvCont );
            break;
        case ITEM_COLLECTION_USE:
            Self->ProcessItemsCollection( ITEM_COLLECTION_USE, Self->InvContInit, Self->UseCont );
            break;
        case ITEM_COLLECTION_BARTER:
            Self->ProcessItemsCollection( ITEM_COLLECTION_BARTER, Self->InvContInit, Self->BarterCont1 );
            break;
        case ITEM_COLLECTION_BARTER_OFFER:
            Self->ProcessItemsCollection( ITEM_COLLECTION_BARTER_OFFER, Self->BarterCont1oInit, Self->BarterCont1o );
            break;
        case ITEM_COLLECTION_BARTER_OPPONENT:
            Self->ProcessItemsCollection( ITEM_COLLECTION_BARTER_OPPONENT, Self->BarterCont2Init, Self->BarterCont2 );
            break;
        case ITEM_COLLECTION_BARTER_OPPONENT_OFFER:
            Self->ProcessItemsCollection( ITEM_COLLECTION_BARTER_OPPONENT_OFFER, Self->BarterCont2oInit, Self->BarterCont2o );
            break;
        case ITEM_COLLECTION_PICKUP:
            Self->ProcessItemsCollection( ITEM_COLLECTION_PICKUP, Self->InvContInit, Self->PupCont1 );
            break;
        case ITEM_COLLECTION_PICKUP_FROM:
            Self->ProcessItemsCollection( ITEM_COLLECTION_PICKUP_FROM, Self->PupCont2Init, Self->PupCont2 );
            break;
        default:
            SCRIPT_ERROR( "Invalid items collection." );
            break;
    }
}

int FOClient::SScriptFunc::Global_GetScroll( int scroll_element )
{
    switch( scroll_element )
    {
        case SCROLL_MESSBOX:
            return Self->MessBoxScroll;
        case SCROLL_INVENTORY:
            return Self->InvScroll;
        case SCROLL_INVENTORY_ITEM_INFO:
            return Self->InvItemInfoScroll;
        case SCROLL_PICKUP:
            return Self->PupScroll1;
        case SCROLL_PICKUP_FROM:
            return Self->PupScroll2;
        case SCROLL_USE:
            return Self->UseScroll;
        case SCROLL_BARTER:
            return Self->BarterScroll1;
        case SCROLL_BARTER_OFFER:
            return Self->BarterScroll1o;
        case SCROLL_BARTER_OPPONENT:
            return Self->BarterScroll2;
        case SCROLL_BARTER_OPPONENT_OFFER:
            return Self->BarterScroll2o;
        case SCROLL_GLOBAL_MAP_CITIES_X:
            return Self->WorldmapTabsScrX;
        case SCROLL_GLOBAL_MAP_CITIES_Y:
            return Self->WorldmapTabsScrY;
        case SCROLL_SPLIT_VALUE:
            return Self->SplitValue;
        case SCROLL_TIMER_VALUE:
            return Self->TimerValue;
        case SCROLL_PERK:
            return Self->PerkScroll;
        case SCROLL_DIALOG_TEXT:
            return Self->DlgMainTextCur;
        case SCROLL_MAP_ZOOM_VALUE:
            return Self->LmapZoom;
        case SCROLL_CHARACTER_PERKS:
            return Self->ChaSwitchScroll[CHA_SWITCH_PERKS];
        case SCROLL_CHARACTER_KARMA:
            return Self->ChaSwitchScroll[CHA_SWITCH_KARMA];
        case SCROLL_CHARACTER_KILLS:
            return Self->ChaSwitchScroll[CHA_SWITCH_KILLS];
        case SCROLL_PIPBOY_STATUS:
            return Self->PipScroll[PIP__STATUS];
        case SCROLL_PIPBOY_STATUS_QUESTS:
            return Self->PipScroll[PIP__STATUS_QUESTS];
        case SCROLL_PIPBOY_STATUS_SCORES:
            return Self->PipScroll[PIP__STATUS_SCORES];
        case SCROLL_PIPBOY_AUTOMAPS:
            return Self->PipScroll[PIP__AUTOMAPS];
        case SCROLL_PIPBOY_ARCHIVES:
            return Self->PipScroll[PIP__ARCHIVES];
        case SCROLL_PIPBOY_ARCHIVES_INFO:
            return Self->PipScroll[PIP__ARCHIVES_INFO];
        default:
            break;
    }
    return 0;
}

int ContainerMaxScroll( int items_count, int cont_height, int item_height )
{
    int height_items = cont_height / item_height;
    if( items_count <= height_items )
        return 0;
    return items_count - height_items;
}

void FOClient::SScriptFunc::Global_SetScroll( int scroll_element, int value )
{
    int* scroll = NULL;
    int  min_value = 0;
    int  max_value = 1000000000;
    switch( scroll_element )
    {
        case SCROLL_MESSBOX:
            scroll = &Self->MessBoxScroll;
            max_value = Self->MessBoxMaxScroll;
            break;
        case SCROLL_INVENTORY:
            scroll = &Self->InvScroll;
            max_value = ContainerMaxScroll( (int)Self->InvCont.size(), Self->InvWInv.H(), Self->InvHeightItem );
            break;
        case SCROLL_INVENTORY_ITEM_INFO:
            scroll = &Self->InvItemInfoScroll;
            max_value = Self->InvItemInfoMaxScroll;
            break;
        case SCROLL_PICKUP:
            scroll = &Self->PupScroll1;
            max_value = ContainerMaxScroll( (int)Self->PupCont1.size(), Self->PupWCont1.H(), Self->PupHeightItem1 );
            break;
        case SCROLL_PICKUP_FROM:
            scroll = &Self->PupScroll2;
            max_value = ContainerMaxScroll( (int)Self->PupCont2.size(), Self->PupWCont2.H(), Self->PupHeightItem2 );
            break;
        case SCROLL_USE:
            scroll = &Self->UseScroll;
            max_value = ContainerMaxScroll( (int)Self->UseCont.size(), Self->UseWInv.H(), Self->UseHeightItem );
            break;
        case SCROLL_BARTER:
            scroll = &Self->BarterScroll1;
            max_value = ContainerMaxScroll( (int)Self->BarterCont1.size(), Self->BarterWCont1.H(), Self->BarterCont1HeightItem );
            break;
        case SCROLL_BARTER_OFFER:
            scroll = &Self->BarterScroll1o;
            max_value = ContainerMaxScroll( (int)Self->BarterCont1o.size(), Self->BarterWCont1o.H(), Self->BarterCont1oHeightItem );
            break;
        case SCROLL_BARTER_OPPONENT:
            scroll = &Self->BarterScroll2;
            max_value = ContainerMaxScroll( (int)Self->BarterCont2.size(), Self->BarterWCont2.H(), Self->BarterCont2HeightItem );
            break;
        case SCROLL_BARTER_OPPONENT_OFFER:
            scroll = &Self->BarterScroll2o;
            max_value = ContainerMaxScroll( (int)Self->BarterCont2o.size(), Self->BarterWCont2o.H(), Self->BarterCont2oHeightItem );
            break;
        case SCROLL_GLOBAL_MAP_CITIES_X:
            scroll = &Self->WorldmapTabsScrX;
            break;
        case SCROLL_GLOBAL_MAP_CITIES_Y:
            scroll = &Self->WorldmapTabsScrY;
            break;
        case SCROLL_SPLIT_VALUE:
            scroll = &Self->SplitValue;
            max_value = MAX_SPLIT_VALUE - 1;
            break;
        case SCROLL_TIMER_VALUE:
            scroll = &Self->TimerValue;
            min_value = TIMER_MIN_VALUE;
            max_value = TIMER_MAX_VALUE;
            break;
        case SCROLL_PERK:
            scroll = &Self->PerkScroll;
            max_value = (int)Self->PerkCollection.size() - 1;
            break;
        case SCROLL_DIALOG_TEXT:
            scroll = &Self->DlgMainTextCur;
            max_value = Self->DlgMainTextLinesReal - Self->DlgMainTextLinesRect;
            break;
        case SCROLL_MAP_ZOOM_VALUE:
            scroll = &Self->LmapZoom;
            min_value = 2;
            max_value = 13;
            break;
        case SCROLL_CHARACTER_PERKS:
            scroll = &Self->ChaSwitchScroll[CHA_SWITCH_PERKS];
            break;
        case SCROLL_CHARACTER_KARMA:
            scroll = &Self->ChaSwitchScroll[CHA_SWITCH_KARMA];
            break;
        case SCROLL_CHARACTER_KILLS:
            scroll = &Self->ChaSwitchScroll[CHA_SWITCH_KILLS];
            break;
        case SCROLL_PIPBOY_STATUS:
            scroll = &Self->PipScroll[PIP__STATUS];
            break;
        case SCROLL_PIPBOY_STATUS_QUESTS:
            scroll = &Self->PipScroll[PIP__STATUS_QUESTS];
            break;
        case SCROLL_PIPBOY_STATUS_SCORES:
            scroll = &Self->PipScroll[PIP__STATUS_SCORES];
            break;
        case SCROLL_PIPBOY_AUTOMAPS:
            scroll = &Self->PipScroll[PIP__AUTOMAPS];
            break;
        case SCROLL_PIPBOY_ARCHIVES:
            scroll = &Self->PipScroll[PIP__ARCHIVES];
            break;
        case SCROLL_PIPBOY_ARCHIVES_INFO:
            scroll = &Self->PipScroll[PIP__ARCHIVES_INFO];
            break;
        default:
            return;
    }
    *scroll = CLAMP( value, min_value, max_value );
}

uint FOClient::SScriptFunc::Global_GetDayTime( uint day_part )
{
    if( day_part >= 4 )
        SCRIPT_ERROR_R0( "Invalid day part arg." );
    if( Self->HexMngr.IsMapLoaded() )
        return Self->HexMngr.GetMapDayTime()[day_part];
    return 0;
}

void FOClient::SScriptFunc::Global_GetDayColor( uint day_part, uint8& r, uint8& g, uint8& b )
{
    r = g = b = 0;
    if( day_part >= 4 )
        SCRIPT_ERROR_R( "Invalid day part arg." );
    if( Self->HexMngr.IsMapLoaded() )
    {
        uint8* col = Self->HexMngr.GetMapDayColor();
        r = col[0 + day_part];
        g = col[4 + day_part];
        b = col[8 + day_part];
    }
}

ScriptString* FOClient::SScriptFunc::Global_GetLastError()
{
    return new ScriptString( ScriptLastError );
}

void FOClient::SScriptFunc::Global_Log( ScriptString& text )
{
    Script::Log( text.c_str() );
}

ProtoItem* FOClient::SScriptFunc::Global_GetProtoItem( uint16 proto_id )
{
    ProtoItem* proto_item = ItemMngr.GetProtoItem( proto_id );
    // if(!proto_item) SCRIPT_ERROR_R0("Proto item not found.");
    return proto_item;
}

uint FOClient::SScriptFunc::Global_GetDistantion( uint16 hex_x1, uint16 hex_y1, uint16 hex_x2, uint16 hex_y2 )
{
    return DistGame( hex_x1, hex_y1, hex_x2, hex_y2 );
}

uint8 FOClient::SScriptFunc::Global_GetDirection( uint16 from_hx, uint16 from_hy, uint16 to_hx, uint16 to_hy )
{
    return GetFarDir( from_hx, from_hy, to_hx, to_hy );
}

uint8 FOClient::SScriptFunc::Global_GetOffsetDir( uint16 from_hx, uint16 from_hy, uint16 to_hx, uint16 to_hy, float offset )
{
    return GetFarDir( from_hx, from_hy, to_hx, to_hy, offset );
}

uint FOClient::SScriptFunc::Global_GetFullSecond( uint16 year, uint16 month, uint16 day, uint16 hour, uint16 minute, uint16 second )
{
    if( !year )
        year = GameOpt.Year;
    else
        year = CLAMP( year, GameOpt.YearStart, GameOpt.YearStart + 130 );
    if( !month )
        month = GameOpt.Month;
    else
        month = CLAMP( month, 1, 12 );
    if( !day )
        day = GameOpt.Day;
    else
    {
        uint month_day = Timer::GameTimeMonthDay( year, month );
        day = CLAMP( day, 1, month_day );
    }
    if( hour > 23 )
        hour = 23;
    if( minute > 59 )
        minute = 59;
    if( second > 59 )
        second = 59;
    return Timer::GetFullSecond( year, month, day, hour, minute, second );
}

void FOClient::SScriptFunc::Global_GetGameTime( uint full_second, uint16& year, uint16& month, uint16& day, uint16& day_of_week, uint16& hour, uint16& minute, uint16& second )
{
    DateTime dt = Timer::GetGameTime( full_second );
    year = dt.Year;
    month = dt.Month;
    day_of_week = dt.DayOfWeek;
    day = dt.Day;
    hour = dt.Hour;
    minute = dt.Minute;
    second = dt.Second;
}

bool FOClient::SScriptFunc::Global_StrToInt( ScriptString* text, int& result )
{
    if( !text || !text->length() || !Str::IsNumber( text->c_str() ) )
        return false;
    result = Str::AtoI( text->c_str() );
    return true;
}

bool FOClient::SScriptFunc::Global_StrToFloat( ScriptString* text, float& result )
{
    if( !text || !text->length() )
        return false;
    result = (float)strtod( text->c_str(), NULL );
    return true;
}

void FOClient::SScriptFunc::Global_MoveHexByDir( uint16& hx, uint16& hy, uint8 dir, uint steps )
{
    if( !Self->HexMngr.IsMapLoaded() )
        SCRIPT_ERROR_R( "Map not loaded." );
    if( dir >= DIRS_COUNT )
        SCRIPT_ERROR_R( "Invalid dir arg." );
    if( !steps )
        SCRIPT_ERROR_R( "Steps arg is zero." );
    if( steps > 1 )
    {
        for( uint i = 0; i < steps; i++ )
            MoveHexByDir( hx, hy, dir, Self->HexMngr.GetMaxHexX(), Self->HexMngr.GetMaxHexY() );
    }
    else
    {
        MoveHexByDir( hx, hy, dir, Self->HexMngr.GetMaxHexX(), Self->HexMngr.GetMaxHexY() );
    }
}

bool FOClient::SScriptFunc::Global_AppendIfaceIni( ScriptString& ini_name )
{
    return Self->AppendIfaceIni( ini_name.c_str() );
}

ScriptString* FOClient::SScriptFunc::Global_GetIfaceIniStr( ScriptString& key )
{
    char* big_buf = Str::GetBigBuf();
    if( Self->IfaceIni.GetStr( key.c_str(), "", big_buf ) )
        return new ScriptString( big_buf );
    return new ScriptString( "" );
}

bool FOClient::SScriptFunc::Global_Load3dFile( ScriptString& fname, int path_type )
{
    char fname_[MAX_FOPATH];
    Str::Copy( fname_, FileManager::GetPath( path_type ) );
    Str::Append( fname_, fname.c_str() );
    Animation3dEntity* entity = Animation3dEntity::GetEntity( fname_ );
    return entity != NULL;
}

void FOClient::SScriptFunc::Global_WaitPing()
{
    Self->WaitPing();
}

bool FOClient::SScriptFunc::Global_LoadFont( int font_index, ScriptString& font_fname )
{
    if( font_fname.c_str()[0] == '*' )
        return SprMngr.LoadFontFO( font_index, font_fname.c_str() + 1 );
    return SprMngr.LoadFontBMF( font_index, font_fname.c_str() );
}

void FOClient::SScriptFunc::Global_SetDefaultFont( int font, uint color )
{
    SprMngr.SetDefaultFont( font, color );
}

bool FOClient::SScriptFunc::Global_SetEffect( int effect_type, int effect_subtype, ScriptString* effect_name, ScriptString* effect_defines )
{
    // Effect types
    #define EFFECT_2D_GENERIC                (0x00000001)   // Subtype can be item id, zero for all items
    #define EFFECT_2D_CRITTER                (0x00000002)   // Subtype can be critter id, zero for all critters
    #define EFFECT_2D_TILE                   (0x00000004)
    #define EFFECT_2D_ROOF                   (0x00000008)
    #define EFFECT_2D_RAIN                   (0x00000010)
    #define EFFECT_3D_SIMPLE                 (0x00000100)   // Only for OpenGL
    #define EFFECT_3D_SIMPLE_SHADOW          (0x00000200)   // Only for OpenGL
    #define EFFECT_3D_SKINNED                (0x00000400)
    #define EFFECT_3D_SKINNED_SHADOW         (0x00000800)   // Only for OpenGL
    #define EFFECT_INTERFACE_BASE            (0x00001000)
    #define EFFECT_INTERFACE_CONTOUR         (0x00002000)   // Only for OpenGL
    #define EFFECT_FONT                      (0x00010000)   // Subtype is FONT_*, -1 default for all fonts
    #define EFFECT_PRIMITIVE_GENERIC         (0x00100000)
    #define EFFECT_PRIMITIVE_LIGHT           (0x00200000)
    #define EFFECT_FLUSH_RENDER_TARGET       (0x01000000)   // Only for OpenGL
    #define EFFECT_FLUSH_RENDER_TARGET_MS    (0x02000000)   // Multisample, Only for OpenGL
    #define EFFECT_FLUSH_PRIMITIVE           (0x04000000)   // Only for OpenGL
    #define EFFECT_FLUSH_MAP                 (0x08000000)   // Only for OpenGL

    Effect* effect = NULL;
    if( effect_name && effect_name->length() )
    {
        bool use_in_2d = !(effect_type & (EFFECT_3D_SIMPLE | EFFECT_3D_SKINNED) );
        effect = GraphicLoader::LoadEffect( SprMngr.GetDevice(), effect_name->c_str(), use_in_2d, effect_defines ? effect_defines->c_str() : NULL );
        if( !effect )
            SCRIPT_ERROR_R0( "Effect not found or have some errors, see log file." );
    }

    if( effect_type & EFFECT_2D_GENERIC && effect_subtype != 0 )
    {
        ItemHex* item = Self->GetItem( (uint)effect_subtype );
        if( item )
            item->DrawEffect = (effect ? effect : Effect::Generic);
    }
    if( effect_type & EFFECT_2D_CRITTER && effect_subtype != 0 )
    {
        CritterCl* cr = Self->GetCritter( (uint)effect_subtype );
        if( cr )
            cr->DrawEffect = (effect ? effect : Effect::Critter);
    }

    if( effect_type & EFFECT_2D_GENERIC && effect_subtype == 0 )
        *Effect::Generic = (effect ? *effect : *Effect::GenericDefault);
    if( effect_type & EFFECT_2D_CRITTER && effect_subtype == 0 )
        *Effect::Critter = (effect ? *effect : *Effect::CritterDefault);
    if( effect_type & EFFECT_2D_TILE )
        *Effect::Tile = (effect ? *effect : *Effect::TileDefault);
    if( effect_type & EFFECT_2D_ROOF )
        *Effect::Roof = (effect ? *effect : *Effect::RoofDefault);
    if( effect_type & EFFECT_2D_RAIN )
        *Effect::Rain = (effect ? *effect : *Effect::RainDefault);

    if( effect_type & EFFECT_3D_SIMPLE )
        *Effect::Simple3d = (effect ? *effect : *Effect::Simple3dDefault);
    if( effect_type & EFFECT_3D_SIMPLE_SHADOW )
        *Effect::Simple3dShadow = (effect ? *effect : *Effect::Simple3dShadowDefault);
    if( effect_type & EFFECT_3D_SKINNED )
        *Effect::Skinned3d = (effect ? *effect : *Effect::Skinned3dDefault);
    if( effect_type & EFFECT_3D_SKINNED_SHADOW )
        *Effect::Skinned3dShadow = (effect ? *effect : *Effect::Skinned3dShadowDefault);

    if( effect_type & EFFECT_INTERFACE_BASE )
        *Effect::Iface = (effect ? *effect : *Effect::IfaceDefault);
    if( effect_type & EFFECT_INTERFACE_CONTOUR )
        *Effect::Contour = (effect ? *effect : *Effect::ContourDefault);

    if( effect_type & EFFECT_FONT && effect_subtype == -1 )
        *Effect::Font = (effect ? *effect : *Effect::ContourDefault);
    if( effect_type & EFFECT_FONT && effect_subtype >= 0 )
        SprMngr.SetFontEffect( effect_subtype, effect );

    if( effect_type & EFFECT_PRIMITIVE_GENERIC )
        *Effect::Primitive = (effect ? *effect : *Effect::PrimitiveDefault);
    if( effect_type & EFFECT_PRIMITIVE_LIGHT )
        *Effect::Light = (effect ? *effect : *Effect::LightDefault);

    if( effect_type & EFFECT_FLUSH_RENDER_TARGET )
        *Effect::FlushRenderTarget = (effect ? *effect : *Effect::FlushRenderTargetDefault);
    if( effect_type & EFFECT_FLUSH_RENDER_TARGET_MS )
        *Effect::FlushRenderTargetMS = (effect ? *effect : *Effect::FlushRenderTargetMSDefault);
    if( effect_type & EFFECT_FLUSH_PRIMITIVE )
        *Effect::FlushPrimitive = (effect ? *effect : *Effect::FlushPrimitiveDefault);
    if( effect_type & EFFECT_FLUSH_MAP )
        *Effect::FlushMap = (effect ? *effect : *Effect::FlushMapDefault);

    return true;
}

void FOClient::SScriptFunc::Global_RefreshMap( bool only_tiles, bool only_roof, bool only_light )
{
    if( Self->HexMngr.IsMapLoaded() )
    {
        if( only_tiles )
            Self->HexMngr.RebuildTiles();
        else if( only_roof )
            Self->HexMngr.RebuildRoof();
        else if( only_light )
            Self->HexMngr.RebuildLight();
        else
            Self->HexMngr.RefreshMap();
    }
}

void FOClient::SScriptFunc::Global_MouseClick( int x, int y, int button, int cursor )
{
    IntVec prev_events = MainWindow->MouseEvents;
    MainWindow->MouseEvents.clear();
    int    prev_x = GameOpt.MouseX;
    int    prev_y = GameOpt.MouseY;
    int    prev_cursor = Self->CurMode;
    GameOpt.MouseX = x;
    GameOpt.MouseY = y;
    Self->CurMode = cursor;
    MainWindow->MouseEvents.push_back( FL_PUSH );
    MainWindow->MouseEvents.push_back( button );
    MainWindow->MouseEvents.push_back( 0 );
    MainWindow->MouseEvents.push_back( FL_RELEASE );
    MainWindow->MouseEvents.push_back( button );
    MainWindow->MouseEvents.push_back( 0 );
    Self->ParseMouse();
    MainWindow->MouseEvents = prev_events;
    GameOpt.MouseX = prev_x;
    GameOpt.MouseY = prev_y;
    Self->CurMode = prev_cursor;
}

void FOClient::SScriptFunc::Global_KeyboardPress( uint8 key1, uint8 key2, ScriptString* key1_text, ScriptString* key2_text )
{
    IntVec prev_events = MainWindow->KeyboardEvents;
    StrVec prev_events_text = MainWindow->KeyboardEventsText;
    MainWindow->KeyboardEvents.clear();
    MainWindow->KeyboardEvents.push_back( FL_KEYDOWN );
    MainWindow->KeyboardEvents.push_back( Keyb::UnmapKey( key1 ) );
    MainWindow->KeyboardEventsText.push_back( key1_text ? key1_text->c_std_str() : "" );
    MainWindow->KeyboardEvents.push_back( FL_KEYDOWN );
    MainWindow->KeyboardEvents.push_back( Keyb::UnmapKey( key2 ) );
    MainWindow->KeyboardEventsText.push_back( key2_text ? key2_text->c_std_str() : "" );
    MainWindow->KeyboardEvents.push_back( FL_KEYUP );
    MainWindow->KeyboardEvents.push_back( Keyb::UnmapKey( key2 ) );
    MainWindow->KeyboardEventsText.push_back( key1_text ? key1_text->c_std_str() : "" );
    MainWindow->KeyboardEvents.push_back( FL_KEYUP );
    MainWindow->KeyboardEvents.push_back( Keyb::UnmapKey( key1 ) );
    MainWindow->KeyboardEventsText.push_back( key2_text ? key2_text->c_std_str() : "" );
    Self->ParseKeyboard();
    MainWindow->KeyboardEvents = prev_events;
    MainWindow->KeyboardEventsText = prev_events_text;
}

void FOClient::SScriptFunc::Global_SetRainAnimation( ScriptString* fall_anim_name, ScriptString* drop_anim_name )
{
    Self->HexMngr.SetRainAnimation( fall_anim_name ? fall_anim_name->c_str() : NULL, drop_anim_name ? drop_anim_name->c_str() : NULL );
}

void FOClient::SScriptFunc::Global_SetZoom( float zoom )
{
    if( zoom == GameOpt.SpritesZoom )
        return;

    if( zoom == 1.0f )
    {
        Self->HexMngr.ChangeZoom( 0 );
    }
    else if( zoom > GameOpt.SpritesZoom )
    {
        while( zoom > GameOpt.SpritesZoom )
        {
            float old_zoom = GameOpt.SpritesZoom;
            Self->HexMngr.ChangeZoom( 1 );
            if( GameOpt.SpritesZoom == old_zoom )
                break;
        }
    }
    else if( zoom < GameOpt.SpritesZoom )
    {
        while( zoom < GameOpt.SpritesZoom )
        {
            float old_zoom = GameOpt.SpritesZoom;
            Self->HexMngr.ChangeZoom( -1 );
            if( GameOpt.SpritesZoom == old_zoom )
                break;
        }
    }
}

uint FOClient::SScriptFunc::Global_GetTick()
{
    return Timer::FastTick();
}

void FOClient::SScriptFunc::Global_GetTime( uint16& year, uint16& month, uint16& day, uint16& day_of_week, uint16& hour, uint16& minute, uint16& second, uint16& milliseconds )
{
    DateTime dt;
    Timer::GetCurrentDateTime( dt );
    year = dt.Year;
    month = dt.Month;
    day_of_week = dt.DayOfWeek;
    day = dt.Day;
    hour = dt.Hour;
    minute = dt.Minute;
    second = dt.Second;
    milliseconds = dt.Milliseconds;
}

bool FOClient::SScriptFunc::Global_SetParameterGetBehaviour( uint index, ScriptString& func_name )
{
    if( index >= MAX_PARAMS )
        SCRIPT_ERROR_R0( "Invalid index arg." );
    CritterCl::ParamsGetScript[index] = 0;
    if( func_name.length() > 0 )
    {
        int bind_id = Script::Bind( func_name.c_str(), "int %s(CritterCl&,uint)", false );
        if( bind_id <= 0 )
            SCRIPT_ERROR_R0( "Function not found." );
        CritterCl::ParamsGetScript[index] = bind_id;
    }
    return true;
}

bool FOClient::SScriptFunc::Global_SetParameterChangeBehaviour( uint index, ScriptString& func_name )
{
    if( index >= MAX_PARAMS )
        SCRIPT_ERROR_R0( "Invalid index arg." );
    CritterCl::ParamsChangeScript[index] = 0;
    if( func_name.length() > 0 )
    {
        int bind_id = Script::Bind( func_name.c_str(), "void %s(CritterCl&,uint,int)", false );
        if( bind_id <= 0 )
            SCRIPT_ERROR_R0( "Function not found." );
        CritterCl::ParamsChangeScript[index] = bind_id;
    }
    return true;
}

void FOClient::SScriptFunc::Global_AllowSlot( uint8 index, ScriptString& ini_option )
{
    if( index <= SLOT_ARMOR || index == SLOT_GROUND )
        SCRIPT_ERROR_R( "Invalid index arg." );
    if( !ini_option.length() )
        SCRIPT_ERROR_R( "Ini string is empty." );
    CritterCl::SlotEnabled[index] = true;
    SlotExt se;
    se.Index = index;
    se.IniName = Str::Duplicate( ini_option.c_str() );
    Self->IfaceLoadRect( se.Region, se.IniName );
    Self->SlotsExt.push_back( se );
}

uint FOClient::SScriptFunc::Global_DecodeUTF8( ScriptString& text, uint& length )
{
    return Str::DecodeUTF8( text.c_str(), &length );
}

ScriptString* FOClient::SScriptFunc::Global_EncodeUTF8( uint ucs )
{
    char buf[5];
    uint len = Str::EncodeUTF8( ucs, buf );
    buf[len] = 0;
    return new ScriptString( buf );
}

void FOClient::SScriptFunc::Global_SetRegistrationParam( uint index, bool enabled )
{
    if( index >= MAX_PARAMS )
        SCRIPT_ERROR_R( "Invalid index arg." );
    CritterCl::ParamsRegEnabled[index] = enabled;
}

uint FOClient::SScriptFunc::Global_GetAngelScriptProperty( int property )
{
    asIScriptEngine* engine = Script::GetEngine();
    if( !engine )
        SCRIPT_ERROR_R0( "Can't get engine." );
    return (uint)engine->GetEngineProperty( (asEEngineProp)property );
}

bool FOClient::SScriptFunc::Global_SetAngelScriptProperty( int property, uint value )
{
    asIScriptEngine* engine = Script::GetEngine();
    if( !engine )
        SCRIPT_ERROR_R0( "Can't get engine." );
    int result = engine->SetEngineProperty( (asEEngineProp)property, value );
    if( result < 0 )
        SCRIPT_ERROR_R0( "Invalid data. Property not setted." );
    return true;
}

uint FOClient::SScriptFunc::Global_GetStrHash( ScriptString* str )
{
    if( str )
        return Str::GetHash( str->c_str() );
    return 0;
}

bool FOClient::SScriptFunc::Global_LoadDataFile( ScriptString& dat_name )
{
    if( FileManager::LoadDataFile( dat_name.c_str() ) )
    {
        ResMngr.Refresh();
        ConstantsManager::Initialize( PATH_DATA );
        return true;
    }
    return false;
}

int FOClient::SScriptFunc::Global_GetConstantValue( int const_collection, ScriptString* name )
{
    if( !ConstantsManager::IsCollectionInit( const_collection ) )
        SCRIPT_ERROR_R0( "Invalid namesFile arg." );
    if( !name || !name->length() )
        SCRIPT_ERROR_R0( "Invalid name arg." );
    return ConstantsManager::GetValue( const_collection, name->c_str() );
}

ScriptString* FOClient::SScriptFunc::Global_GetConstantName( int const_collection, int value )
{
    if( !ConstantsManager::IsCollectionInit( const_collection ) )
        SCRIPT_ERROR_R0( "Invalid namesFile arg." );
    return new ScriptString( ConstantsManager::GetName( const_collection, value ) );
}

void FOClient::SScriptFunc::Global_AddConstant( int const_collection, ScriptString* name, int value )
{
    if( !ConstantsManager::IsCollectionInit( const_collection ) )
        SCRIPT_ERROR_R( "Invalid namesFile arg." );
    if( !name || !name->length() )
        SCRIPT_ERROR_R( "Invalid name arg." );
    ConstantsManager::AddConstant( const_collection, name->c_str(), value );
}

bool FOClient::SScriptFunc::Global_LoadConstants( int const_collection, ScriptString* file_name, int path_type )
{
    if( const_collection < 0 || const_collection > 1000 )
        SCRIPT_ERROR_R0( "Invalid namesFile arg." );
    if( !file_name || !file_name->length() )
        SCRIPT_ERROR_R0( "Invalid fileName arg." );
    return ConstantsManager::AddCollection( const_collection, file_name->c_str(), path_type );
}

bool FOClient::SScriptFunc::Global_IsCritterCanWalk( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::IsCanWalk( cr_type );
}

bool FOClient::SScriptFunc::Global_IsCritterCanRun( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::IsCanRun( cr_type );
}

bool FOClient::SScriptFunc::Global_IsCritterCanRotate( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::IsCanRotate( cr_type );
}

bool FOClient::SScriptFunc::Global_IsCritterCanAim( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::IsCanAim( cr_type );
}

bool FOClient::SScriptFunc::Global_IsCritterCanArmor( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::IsCanArmor( cr_type );
}

bool FOClient::SScriptFunc::Global_IsCritterAnim1( uint cr_type, uint index )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::IsAnim1( cr_type, index );
}

int FOClient::SScriptFunc::Global_GetCritterAnimType( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::GetAnimType( cr_type );
}

uint FOClient::SScriptFunc::Global_GetCritterAlias( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_R0( "Invalid critter type arg." );
    return CritType::GetAlias( cr_type );
}

ScriptString* FOClient::SScriptFunc::Global_GetCritterTypeName( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_RX( "Invalid critter type arg.", new ScriptString( "" ) );
    return new ScriptString( CritType::GetCritType( cr_type ).Name );
}

ScriptString* FOClient::SScriptFunc::Global_GetCritterSoundName( uint cr_type )
{
    if( !CritType::IsEnabled( cr_type ) )
        SCRIPT_ERROR_RX( "Invalid critter type arg.", new ScriptString( "" ) );
    return new ScriptString( CritType::GetSoundName( cr_type ) );
}

void FOClient::SScriptFunc::Global_RunServerScript( ScriptString& func_name, int p0, int p1, int p2, ScriptString* p3, ScriptArray* p4 )
{
    UIntVec dw;
    if( p4 )
        Script::AssignScriptArrayInVector<uint>( dw, p4 );
    Self->Net_SendRunScript( false, func_name.c_str(), p0, p1, p2, p3 ? p3->c_str() : NULL, dw );
}

void FOClient::SScriptFunc::Global_RunServerScriptUnsafe( ScriptString& func_name, int p0, int p1, int p2, ScriptString* p3, ScriptArray* p4 )
{
    UIntVec dw;
    if( p4 )
        Script::AssignScriptArrayInVector<uint>( dw, p4 );
    Self->Net_SendRunScript( true, func_name.c_str(), p0, p1, p2, p3 ? p3->c_str() : NULL, dw );
}

uint FOClient::SScriptFunc::Global_LoadSprite( ScriptString& spr_name, int path_index )
{
    if( path_index >= PATH_LIST_COUNT )
        SCRIPT_ERROR_R0( "Invalid path index arg." );
    return Self->AnimLoad( spr_name.c_str(), path_index, RES_SCRIPT );
}

uint FOClient::SScriptFunc::Global_LoadSpriteHash( uint name_hash, uint8 dir )
{
    return Self->AnimLoad( name_hash, dir, RES_SCRIPT );
}

int FOClient::SScriptFunc::Global_GetSpriteWidth( uint spr_id, int spr_index )
{
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return 0;
    SpriteInfo* si = SprMngr.GetSpriteInfo( spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ) );
    if( !si )
        return 0;
    return si->Width;
}

int FOClient::SScriptFunc::Global_GetSpriteHeight( uint spr_id, int spr_index )
{
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return 0;
    SpriteInfo* si = SprMngr.GetSpriteInfo( spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ) );
    if( !si )
        return 0;
    return si->Height;
}

uint FOClient::SScriptFunc::Global_GetSpriteCount( uint spr_id )
{
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    return anim ? anim->CntFrm : 0;
}

void FOClient::SScriptFunc::Global_GetTextInfo( ScriptString& text, int w, int h, int font, int flags, int& tw, int& th, int& lines )
{
    SprMngr.GetTextInfo( w, h, text.c_str(), font, flags, tw, th, lines );
}

void FOClient::SScriptFunc::Global_DrawSprite( uint spr_id, int spr_index, int x, int y, uint color )
{
    if( !SpritesCanDraw || !spr_id )
        return;
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return;
    SprMngr.DrawSprite( spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ), x, y, color );
}

void FOClient::SScriptFunc::Global_DrawSpriteOffs( uint spr_id, int spr_index, int x, int y, uint color, bool offs )
{
    if( !SpritesCanDraw || !spr_id )
        return;
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return;
    uint spr_id_ = (spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ) );
    if( offs )
    {
        SpriteInfo* si = SprMngr.GetSpriteInfo( spr_id_ );
        if( !si )
            return;
        x += -si->Width / 2 + si->OffsX;
        y += -si->Height + si->OffsY;
    }
    SprMngr.DrawSprite( spr_id_, x, y, color );
}

void FOClient::SScriptFunc::Global_DrawSpritePattern( uint spr_id, int spr_index, int x, int y, int w, int h, int spr_width, int spr_height, uint color )
{
    if( !SpritesCanDraw || !spr_id )
        return;
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return;
    SprMngr.DrawSpritePattern( spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ), x, y, w, h, spr_width, spr_height, color );
}

void FOClient::SScriptFunc::Global_DrawSpriteSize( uint spr_id, int spr_index, int x, int y, int w, int h, bool scratch, bool center, uint color )
{
    if( !SpritesCanDraw || !spr_id )
        return;
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return;
    SprMngr.DrawSpriteSize( spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ), x, y, (float)w, (float)h, scratch, center, color );
}

void FOClient::SScriptFunc::Global_DrawSpriteSizeOffs( uint spr_id, int spr_index, int x, int y, int w, int h, bool scratch, bool center, uint color, bool offs )
{
    if( !SpritesCanDraw || !spr_id )
        return;
    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return;
    uint spr_id_ = (spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ) );
    if( offs )
    {
        SpriteInfo* si = SprMngr.GetSpriteInfo( spr_id_ );
        if( !si )
            return;
        x += si->OffsX;
        y += si->OffsY;
    }
    SprMngr.DrawSpriteSize( spr_id_, x, y, (float)w, (float)h, scratch, center, color );
}

void FOClient::SScriptFunc::Global_DrawText( ScriptString& text, int x, int y, int w, int h, uint color, int font, int flags )
{
    if( !SpritesCanDraw )
        return;
    if( !w && x < GameOpt.ScreenWidth )
        w = GameOpt.ScreenWidth - x;
    if( !h && y < GameOpt.ScreenHeight )
        h = GameOpt.ScreenHeight - y;
    Rect r = Rect( x, y, x + w, y + h );
    SprMngr.DrawStr( r, text.c_str(), flags, color, font );
}

void FOClient::SScriptFunc::Global_DrawPrimitive( int primitive_type, ScriptArray& data )
{
    if( !SpritesCanDraw )
        return;

    if( primitive_type < DRAW_PRIMITIVE_POINTLIST || primitive_type > DRAW_PRIMITIVE_TRIANGLEFAN )
        return;

    static PointVec points;
    int             size = data.GetSize() / 3;
    points.resize( size );

    for( int i = 0; i < size; i++ )
    {
        PrepPoint& pp = points[i];
        pp.PointX = *(int*)data.At( i * 3 );
        pp.PointY = *(int*)data.At( i * 3 + 1 );
        pp.PointColor = *(int*)data.At( i * 3 + 2 );
        pp.PointOffsX = NULL;
        pp.PointOffsY = NULL;
    }

    SprMngr.DrawPoints( points, primitive_type );
}

void FOClient::SScriptFunc::Global_DrawMapSprite( uint16 hx, uint16 hy, uint16 proto_id, uint spr_id, int spr_index, int ox, int oy )
{
    if( !Self->HexMngr.SpritesCanDrawMap )
        return;
    if( !Self->HexMngr.GetHexToDraw( hx, hy ) )
        return;

    AnyFrames* anim = Self->AnimGetFrames( spr_id );
    if( !anim || spr_index >= (int)anim->GetCnt() )
        return;

    ProtoItem* proto_item = ItemMngr.GetProtoItem( proto_id );
    bool       is_flat = (proto_item ? FLAG( proto_item->Flags, ITEM_FLAG_FLAT ) : false);
    bool       is_item = (proto_item ? proto_item->IsItem() : false);
    bool       is_wall = (proto_item ? proto_item->IsWall() : false);
    bool       no_light = (is_flat && !is_item);

    Field&     f = Self->HexMngr.GetField( hx, hy );
    Sprites&   tree = Self->HexMngr.GetDrawTree();
    Sprite&    spr = tree.InsertSprite( is_flat ? (is_item ? DRAW_ORDER_FLAT_ITEM : DRAW_ORDER_FLAT_SCENERY) : (is_item ? DRAW_ORDER_ITEM : DRAW_ORDER_SCENERY),
                                        hx, hy + (proto_item ? proto_item->DrawOrderOffsetHexY : 0), 0,
                                        f.ScrX + HEX_OX + ox, f.ScrY + HEX_OY + oy, spr_index < 0 ? anim->GetCurSprId() : anim->GetSprId( spr_index ), NULL, NULL, NULL, NULL, NULL, NULL );
    if( !no_light )
        spr.SetLight( Self->HexMngr.GetLightHex( 0, 0 ), Self->HexMngr.GetMaxHexX(), Self->HexMngr.GetMaxHexY() );

    if( proto_item )
    {
        if( !is_flat && !proto_item->DisableEgg )
        {
            int egg_type = 0;
            switch( proto_item->Corner )
            {
                case CORNER_SOUTH:
                    egg_type = EGG_X_OR_Y;
                    break;
                case CORNER_NORTH:
                    egg_type = EGG_X_AND_Y;
                    break;
                case CORNER_EAST_WEST:
                case CORNER_WEST:
                    egg_type = EGG_Y;
                    break;
                default:
                    egg_type = EGG_X;
                    break;                              // CORNER_NORTH_SOUTH, CORNER_EAST
            }
            spr.SetEgg( egg_type );
        }

        if( FLAG( proto_item->Flags, ITEM_FLAG_COLORIZE ) )
        {
            spr.SetAlpha( ( (uint8*)&proto_item->LightColor ) + 3 );
            spr.SetColor( proto_item->LightColor & 0xFFFFFF );
        }

        if( FLAG( proto_item->Flags, ITEM_FLAG_BAD_ITEM ) )
            spr.SetContour( CONTOUR_RED );
    }
}

void FOClient::SScriptFunc::Global_DrawCritter2d( uint crtype, uint anim1, uint anim2, uint8 dir, int l, int t, int r, int b, bool scratch, bool center, uint color )
{
    if( CritType::IsEnabled( crtype ) )
    {
        AnyFrames* anim = ResMngr.GetCrit2dAnim( crtype, anim1, anim2, dir );
        if( anim )
            SprMngr.DrawSpriteSize( anim->Ind[0], l, t, (float)(r - l), (float)(b - t), scratch, center, color ? color : COLOR_IFACE );
    }
}

Animation3dVec DrawCritter3dAnim;
UIntVec        DrawCritter3dCrType;
BoolVec        DrawCritter3dFailToLoad;
int            DrawCritter3dLayers[LAYERS3D_COUNT];
void FOClient::SScriptFunc::Global_DrawCritter3d( uint instance, uint crtype, uint anim1, uint anim2, ScriptArray* layers, ScriptArray* position, uint color )
{
    // x y
    // rx ry rz
    // sx sy sz
    // speed
    // stencil l t r b
    if( CritType::IsEnabled( crtype ) )
    {
        if( instance >= DrawCritter3dAnim.size() )
        {
            DrawCritter3dAnim.resize( instance + 1 );
            DrawCritter3dCrType.resize( instance + 1 );
            DrawCritter3dFailToLoad.resize( instance + 1 );
        }

        if( DrawCritter3dFailToLoad[instance] && DrawCritter3dCrType[instance] == crtype )
            return;

        Animation3d*& anim = DrawCritter3dAnim[instance];
        if( !anim || DrawCritter3dCrType[instance] != crtype )
        {
            if( anim )
                SprMngr.FreePure3dAnimation( anim );
            char fname[MAX_FOPATH];
            Str::Format( fname, "%s.fo3d", CritType::GetName( crtype ) );
            anim = SprMngr.LoadPure3dAnimation( fname, PATH_ART_CRITTERS );
            DrawCritter3dCrType[instance] = crtype;
            DrawCritter3dFailToLoad[instance] = false;

            if( !anim )
            {
                DrawCritter3dFailToLoad[instance] = true;
                return;
            }
            anim->EnableShadow( false );
            anim->SetTimer( false );
        }

        uint  count = (position ? position->GetSize() : 0);
        float x = (count > 0 ? *(float*)position->At( 0 ) : 0.0f);
        float y = (count > 1 ? *(float*)position->At( 1 ) : 0.0f);
        float rx = (count > 2 ? *(float*)position->At( 2 ) : 0.0f);
        float ry = (count > 3 ? *(float*)position->At( 3 ) : 0.0f);
        float rz = (count > 4 ? *(float*)position->At( 4 ) : 0.0f);
        float sx = (count > 5 ? *(float*)position->At( 5 ) : 1.0f);
        float sy = (count > 6 ? *(float*)position->At( 6 ) : 1.0f);
        float sz = (count > 7 ? *(float*)position->At( 7 ) : 1.0f);
        float speed = (count > 8 ? *(float*)position->At( 8 ) : 1.0f);
        // 9 reserved
        float stl = (count > 10 ? *(float*)position->At( 10 ) : 0.0f);
        float stt = (count > 11 ? *(float*)position->At( 11 ) : 0.0f);
        float str = (count > 12 ? *(float*)position->At( 12 ) : 0.0f);
        float stb = (count > 13 ? *(float*)position->At( 13 ) : 0.0f);

        memzero( DrawCritter3dLayers, sizeof(DrawCritter3dLayers) );
        for( uint i = 0, j = (layers ? layers->GetSize() : 0); i < j && i < LAYERS3D_COUNT; i++ )
            DrawCritter3dLayers[i] = *(int*)layers->At( i );

        anim->SetRotation( rx * PI_VALUE / 180.0f, ry * PI_VALUE / 180.0f, rz * PI_VALUE / 180.0f );
        anim->SetScale( sx, sy, sz );
        anim->SetSpeed( speed );
        anim->SetAnimation( anim1, anim2, DrawCritter3dLayers, 0 );
        RectF r = RectF( stl, stt, str, stb );
        SprMngr.Draw3d( (int)x, (int)y, 1.0f, anim, stl < str && stt < stb ? &r : NULL, color ? color : COLOR_IFACE );
    }
}

void FOClient::SScriptFunc::Global_ShowScreen( int screen, int p0, int p1, int p2 )
{
    Self->ShowScreen( screen, p0, p1, p2 );
}

void FOClient::SScriptFunc::Global_HideScreen( int screen, int p0, int p1, int p2 )
{
    if( screen )
        Self->HideScreen( screen, p0, p1, p2 );
    else
        Self->ShowScreen( CLIENT_SCREEN_NONE, p0, p1, p2 );
}

void FOClient::SScriptFunc::Global_GetHardcodedScreenPos( int screen, int& x, int& y )
{
    switch( screen )
    {
        case CLIENT_MAIN_SCREEN_LOGIN:
            x = Self->LogX;
            y = Self->LogY;
            break;
        case CLIENT_MAIN_SCREEN_REGISTRATION:
            x = Self->ChaX;
            y = Self->ChaY;
            break;
        case CLIENT_MAIN_SCREEN_GAME:
            x = Self->IntX;
            y = Self->IntY;
            break;
        case CLIENT_MAIN_SCREEN_WORLDMAP:
            x = Self->WorldmapX;
            y = Self->WorldmapY;
            break;
        case CLIENT_MAIN_SCREEN_WAIT:
            x = 0;
            y = 0;
            break;
        case CLIENT_SCREEN_INVENTORY:
            x = Self->InvX;
            y = Self->InvY;
            break;
        case CLIENT_SCREEN_PICKUP:
            x = Self->PupX;
            y = Self->PupY;
            break;
        case CLIENT_SCREEN_MINIMAP:
            x = Self->LmapX;
            y = Self->LmapY;
            break;
        case CLIENT_SCREEN_CHARACTER:
            x = Self->ChaX;
            y = Self->ChaY;
            break;
        case CLIENT_SCREEN_DIALOG:
            x = Self->DlgX;
            y = Self->DlgY;
            break;
        case CLIENT_SCREEN_BARTER:
            x = Self->DlgX;
            y = Self->DlgY;
            break;
        case CLIENT_SCREEN_PIPBOY:
            x = Self->PipX;
            y = Self->PipY;
            break;
        case CLIENT_SCREEN_FIXBOY:
            x = Self->FixX;
            y = Self->FixY;
            break;
        case CLIENT_SCREEN_MENU:
            x = Self->MoptX;
            y = Self->MoptY;
            break;
        case CLIENT_SCREEN_AIM:
            x = Self->AimX;
            y = Self->AimY;
            break;
        case CLIENT_SCREEN_SPLIT:
            x = Self->SplitX;
            y = Self->SplitY;
            break;
        case CLIENT_SCREEN_TIMER:
            x = Self->TimerX;
            y = Self->TimerY;
            break;
        case CLIENT_SCREEN_DIALOGBOX:
            x = Self->DlgboxX;
            y = Self->DlgboxY;
            break;
        case CLIENT_SCREEN_ELEVATOR:
            x = Self->ElevatorX;
            y = Self->ElevatorY;
            break;
        case CLIENT_SCREEN_SAY:
            x = Self->SayX;
            y = Self->SayY;
            break;
        case CLIENT_SCREEN_CHAR_NAME:
            x = Self->ChaNameX;
            y = Self->ChaNameY;
            break;
        case CLIENT_SCREEN_CHAR_AGE:
            x = Self->ChaAgeX;
            y = Self->ChaAgeY;
            break;
        case CLIENT_SCREEN_CHAR_SEX:
            x = Self->ChaSexX;
            y = Self->ChaSexY;
            break;
        case CLIENT_SCREEN_WM_TOWN:
            x = 0;
            y = 0;
            break;
        case CLIENT_SCREEN_INPUTBOX:
            x = Self->IboxX;
            y = Self->IboxY;
            break;
        case CLIENT_SCREEN_SKILLBOX:
            x = Self->SboxX;
            y = Self->SboxY;
            break;
        case CLIENT_SCREEN_USE:
            x = Self->UseX;
            y = Self->UseY;
            break;
        case CLIENT_SCREEN_PERK:
            x = Self->PerkX;
            y = Self->PerkY;
            break;
        case CLIENT_SCREEN_LOADSAVE:
            x = Self->SaveLoadX;
            y = Self->SaveLoadY;
            break;
        default:
            x = 0;
            y = 0;
            break;
    }
}

void FOClient::SScriptFunc::Global_DrawHardcodedScreen( int screen )
{
    switch( screen )
    {
        case CLIENT_MAIN_SCREEN_LOGIN:
            Self->LogDraw();
            break;
        case CLIENT_MAIN_SCREEN_REGISTRATION:
            Self->ChaDraw( true );
            break;
        case CLIENT_MAIN_SCREEN_CREDITS:
            Self->CreditsDraw();
            break;
        case CLIENT_MAIN_SCREEN_OPTIONS:
            break;
        case CLIENT_MAIN_SCREEN_GAME:
            Self->IntDraw();
            break;
        case CLIENT_MAIN_SCREEN_WORLDMAP:
            Self->GmapDraw();
            break;
        case CLIENT_MAIN_SCREEN_WAIT:
            Self->WaitDraw();
            break;
        case CLIENT_SCREEN_INVENTORY:
            Self->InvDraw();
            break;
        case CLIENT_SCREEN_PICKUP:
            Self->PupDraw();
            break;
        case CLIENT_SCREEN_MINIMAP:
            Self->LmapDraw();
            break;
        case CLIENT_SCREEN_DIALOG:
            Self->DlgDraw( true );
            break;
        case CLIENT_SCREEN_BARTER:
            Self->DlgDraw( false );
            break;
        case CLIENT_SCREEN_PIPBOY:
            Self->PipDraw();
            break;
        case CLIENT_SCREEN_FIXBOY:
            Self->FixDraw();
            break;
        case CLIENT_SCREEN_MENU:
            Self->MoptDraw();
            break;
        case CLIENT_SCREEN_CHARACTER:
            Self->ChaDraw( false );
            break;
        case CLIENT_SCREEN_AIM:
            Self->AimDraw();
            break;
        case CLIENT_SCREEN_SPLIT:
            Self->SplitDraw();
            break;
        case CLIENT_SCREEN_TIMER:
            Self->TimerDraw();
            break;
        case CLIENT_SCREEN_DIALOGBOX:
            Self->DlgboxDraw();
            break;
        case CLIENT_SCREEN_ELEVATOR:
            Self->ElevatorDraw();
            break;
        case CLIENT_SCREEN_SAY:
            Self->SayDraw();
            break;
        case CLIENT_SCREEN_CHAR_NAME:
            Self->ChaNameDraw();
            break;
        case CLIENT_SCREEN_CHAR_AGE:
            Self->ChaAgeDraw();
            break;
        case CLIENT_SCREEN_CHAR_SEX:
            Self->ChaSexDraw();
            break;
        case CLIENT_SCREEN_WM_TOWN:
            Self->GmapTownDraw();
            break;
        case CLIENT_SCREEN_INPUTBOX:
            Self->IboxDraw();
            break;
        case CLIENT_SCREEN_SKILLBOX:
            Self->SboxDraw();
            break;
        case CLIENT_SCREEN_USE:
            Self->UseDraw();
            break;
        case CLIENT_SCREEN_PERK:
            Self->PerkDraw();
            break;
        case CLIENT_SCREEN_WM_TOWNVIEW:
            Self->TViewDraw();
            break;
        case CLIENT_SCREEN_LOADSAVE:
            Self->SaveLoadDraw();
            break;
        default:
            break;
    }
}

bool FOClient::SScriptFunc::Global_GetHexPos( uint16 hx, uint16 hy, int& x, int& y )
{
    x = y = 0;
    if( Self->HexMngr.IsMapLoaded() && hx < Self->HexMngr.GetMaxHexX() && hy < Self->HexMngr.GetMaxHexY() )
    {
        Self->HexMngr.GetHexCurrentPosition( hx, hy, x, y );
        x += GameOpt.ScrOx + HEX_OX;
        y += GameOpt.ScrOy + HEX_OY;
        x = (int)(x / GameOpt.SpritesZoom);
        y = (int)(y / GameOpt.SpritesZoom);
        return true;
    }
    return false;
}

bool FOClient::SScriptFunc::Global_GetMonitorHex( int x, int y, uint16& hx, uint16& hy, bool ignore_interface )
{
    uint16 hx_, hy_;
    if( Self->GetCurHex( hx_, hy_, ignore_interface ) )
    {
        hx = hx_;
        hy = hy_;
        return true;
    }
    return false;
}

Item* FOClient::SScriptFunc::Global_GetMonitorItem( int x, int y, bool ignore_interface )
{
    if( !ignore_interface && Self->IsCurInInterface() )
        return NULL;

    ItemHex*   item;
    CritterCl* cr;
    Self->HexMngr.GetSmthPixel( x, y, item, cr );
    return item;
}

CritterCl* FOClient::SScriptFunc::Global_GetMonitorCritter( int x, int y, bool ignore_interface )
{
    if( !ignore_interface && Self->IsCurInInterface() )
        return NULL;

    ItemHex*   item;
    CritterCl* cr;
    Self->HexMngr.GetSmthPixel( x, y, item, cr );
    return cr;
}

uint16 FOClient::SScriptFunc::Global_GetMapWidth()
{
    if( !Self->HexMngr.IsMapLoaded() )
        SCRIPT_ERROR_R0( "Map is not loaded." );
    return Self->HexMngr.GetMaxHexX();
}

uint16 FOClient::SScriptFunc::Global_GetMapHeight()
{
    if( !Self->HexMngr.IsMapLoaded() )
        SCRIPT_ERROR_R0( "Map is not loaded." );
    return Self->HexMngr.GetMaxHexY();
}

int FOClient::SScriptFunc::Global_GetCurrentCursor()
{
    return Self->CurMode;
}

int FOClient::SScriptFunc::Global_GetLastCursor()
{
    return Self->CurModeLast;
}

void FOClient::SScriptFunc::Global_ChangeCursor( int cursor )
{
    Self->SetCurMode( cursor );
}

bool FOClient::SScriptFunc::Global_SaveScreenshot()
{
    return Self->SaveScreenshot();
}

bool FOClient::SScriptFunc::Global_SaveLogFile()
{
    return Self->SaveLogFile();
}
