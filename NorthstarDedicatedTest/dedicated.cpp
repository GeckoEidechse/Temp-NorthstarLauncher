#include "pch.h"
#include "dedicated.h"
#include "hookutils.h"
#include "tier0.h"
#include "gameutils.h"
#include "serverauthentication.h"

bool IsDedicated()
{
	return CommandLine()->CheckParm("-dedicated");
}

bool DisableDedicatedWindowCreation()
{
	return !CommandLine()->CheckParm("-windoweddedi");
}

// CDedidcatedExports defs
struct CDedicatedExports; // forward declare

typedef void (*DedicatedSys_PrintfType)(CDedicatedExports* dedicated, char* msg);
typedef void (*DedicatedRunServerType)(CDedicatedExports* dedicated);

// would've liked to just do this as a class but have not been able to get it to work
struct CDedicatedExports
{
	void* vtable; // because it's easier, we just set this to &this, since CDedicatedExports has no props we care about other than funcs

	char unused[56];

	DedicatedSys_PrintfType Sys_Printf;
	DedicatedRunServerType RunServer;
};

void Sys_Printf(CDedicatedExports* dedicated, char* msg)
{
	spdlog::info("[DEDICATED PRINT] {}", msg);
}

typedef bool (*CEngine__FrameType)(void* engineSelf);
typedef void(*CHostState__InitType)(CHostState* self);

void RunServer(CDedicatedExports* dedicated)
{
	Sys_Printf(dedicated, (char*)"CDedicatedExports::RunServer(): starting");

	// init hoststate, if we don't do this, we get a crash later on
	CHostState__InitType CHostState__Init = (CHostState__InitType)((char*)GetModuleHandleA("engine.dll") + 0x16E110);
	CHostState__Init(g_pHostState);

	// set host state to allow us to enter CHostState::FrameUpdate, with the state HS_NEW_GAME
	g_pHostState->m_iNextState = HostState_t::HS_NEW_GAME;
	strncpy(g_pHostState->m_levelName, CommandLine()->ParmValue("+map", "mp_lobby"), sizeof(g_pHostState->m_levelName)); // set map to load into

	spdlog::info(CommandLine()->GetCmdLine());

	while (g_pEngine->m_nQuitting == EngineQuitState::QUIT_NOTQUITTING)
	{
		g_pEngine->Frame();

		SetConsoleTitleA(fmt::format("Titanfall 2 dedicated server - {} {}/{} players", g_pHostState->m_levelName, g_ServerAuthenticationManager->m_additionalPlayerData.size(), "0").c_str());
		Sleep(50);
	}
}

typedef bool(*IsGameActiveWindowType)();
IsGameActiveWindowType IsGameActiveWindow;
bool IsGameActiveWindowHook()
{
	return true;
}

void InitialiseDedicated(HMODULE engineAddress)
{
	if (!IsDedicated())
		return;

	spdlog::info("InitialiseDedicated");

	{
		// Host_Init
		// prevent a particle init that relies on client dll

		char* ptr = (char*)engineAddress + 0x156799;
		TempReadWrite rw(ptr);

		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
	}

	{
		// CModAppSystemGroup::Create
		// force the engine into dedicated mode by changing the first comparison to IsServerOnly to an assignment
		char* ptr = (char*)engineAddress + 0x1C4EBD;
		TempReadWrite rw(ptr);
		
		// cmp => mov
		*(ptr + 1) = (char)0xC6;
		*(ptr + 2) = (char)0x87;
		
		// 00 => 01
		*((char*)ptr + 7) = (char)0x01;
	}

	{
		// Some init that i'm not sure of that crashes
		char* ptr = (char*)engineAddress + 0x156A63;
		TempReadWrite rw(ptr);

		// nop the call to it
		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
	}

	{
		// runframeserver
		char* ptr = (char*)engineAddress + 0x159819;
		TempReadWrite rw(ptr);

		// nop some access violations
		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
		*(ptr + 5) = (char)0x90;
		*(ptr + 6) = (char)0x90;
		*(ptr + 7) = (char)0x90;
		*(ptr + 8) = (char)0x90;
		*(ptr + 9) = (char)0x90;
		*(ptr + 10) = (char)0x90;
		*(ptr + 11) = (char)0x90;
		*(ptr + 12) = (char)0x90;
		*(ptr + 13) = (char)0x90;
		*(ptr + 14) = (char)0x90;
		*(ptr + 15) = (char)0x90;
		*(ptr + 16) = (char)0x90;
	}

	{
		// HostState_State_NewGame
		char* ptr = (char*)engineAddress + 0x156B4C;
		TempReadWrite rw(ptr);

		// nop some access violations
		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
		*(ptr + 5) = (char)0x90;
		*(ptr + 6) = (char)0x90;

		// previously patched these, took me a couple weeks to figure out they were the issue
		// removing these will mess up register state when this function is over, so we'll write HS_RUN to the wrong address
		// so uhh, don't do that
		//*(ptr + 7) = (char)0x90;
		//*(ptr + 8) = (char)0x90;
		//*(ptr + 9) = (char)0x90;
		//*(ptr + 10) = (char)0x90;
		//*(ptr + 11) = (char)0x90;
		//*(ptr + 12) = (char)0x90;
		//*(ptr + 13) = (char)0x90;
		//*(ptr + 14) = (char)0x90;

		*(ptr + 15) = (char)0x90;
		*(ptr + 16) = (char)0x90;
		*(ptr + 17) = (char)0x90;
		*(ptr + 18) = (char)0x90;
		*(ptr + 19) = (char)0x90;
		*(ptr + 20) = (char)0x90;
		*(ptr + 21) = (char)0x90;
		*(ptr + 22) = (char)0x90;
		*(ptr + 23) = (char)0x90;
	}

	{
		// HostState_State_NewGame
		char* ptr = (char*)engineAddress + 0xB934C;
		TempReadWrite rw(ptr);

		// nop an access violation
		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
		*(ptr + 5) = (char)0x90;
		*(ptr + 6) = (char)0x90;
		*(ptr + 7) = (char)0x90;
		*(ptr + 8) = (char)0x90;
	}

	{
		// CEngineAPI::Connect
		char* ptr = (char*)engineAddress + 0x1C4D7D;
		TempReadWrite rw(ptr);

		// remove call to Shader_Connect
		*ptr = 0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
	}

	// currently does not work, crashes stuff, likely gotta keep this here
	//{
	//	// CEngineAPI::Connect
	//	char* ptr = (char*)engineAddress + 0x1C4E07;
	//	TempReadWrite rw(ptr);
	//	
	//	// remove calls to register ui rpak asset types
	//	*ptr = 0x90;
	//	*(ptr + 1) = (char)0x90;
	//	*(ptr + 2) = (char)0x90;
	//	*(ptr + 3) = (char)0x90;
	//	*(ptr + 4) = (char)0x90;
	//}

	// not sure if this should be done, not loading ui at least is good, but should everything be gone?
	{
		// Host_Init
		char* ptr = (char*)engineAddress + 0x15653B;
		TempReadWrite rw(ptr);

		// change the number of rpaks to load from 6 to 1, so we only load common.rpak
		*(ptr + 1) = (char)0x01;
	}

	{
		// Host_Init
		char* ptr = (char*)engineAddress + 0x156595;
		TempReadWrite rw(ptr);

		// remove call to ui loading stuff
		*ptr = 0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
	}

	{
		// some function that gets called from RunFrameServer
		char* ptr = (char*)engineAddress + 0x15A0BB;
		TempReadWrite rw(ptr);

		// nop a function that makes requests to stryder, this will eventually access violation if left alone and isn't necessary anyway
		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
	}

	{
		// RunFrameServer
		char* ptr = (char*)engineAddress + 0x159BF3;
		TempReadWrite rw(ptr);

		// nop a function that access violations
		*ptr = (char)0x90;
		*(ptr + 1) = (char)0x90;
		*(ptr + 2) = (char)0x90;
		*(ptr + 3) = (char)0x90;
		*(ptr + 4) = (char)0x90;
	}

	// stuff that disables renderer/window creation: unstable atm
	if (DisableDedicatedWindowCreation())
	{
		{
			// CEngineAPI::Init
			char* ptr = (char*)engineAddress + 0x1C60CE;
			TempReadWrite rw(ptr);

			// remove call to something or other that reads video settings
			*ptr = (char)0x90;
			*(ptr + 1) = (char)0x90;
			*(ptr + 2) = (char)0x90;
			*(ptr + 3) = (char)0x90;
			*(ptr + 4) = (char)0x90;
		}

		{
			// some inputsystem bullshit
			char* ptr = (char*)engineAddress + 0x1CEE28;
			TempReadWrite rw(ptr);

			// nop an accessviolation: temp because we still create game window atm
			*ptr = (char)0x90;
			*(ptr + 1) = (char)0x90;
			*(ptr + 2) = (char)0x90;
		}

		//{
		//	// CEngineAPI::ModInit
		//	char* ptr = (char*)engineAddress + 0x1C67D1;
		//	TempReadWrite rw(ptr);
		//
		//	// prevent game window from being created
		//	*ptr = (char)0x90;
		//	*(ptr + 1) = (char)0x90;
		//	*(ptr + 2) = (char)0x90;
		//	*(ptr + 3) = (char)0x90;
		//	*(ptr + 4) = (char)0x90;
		//
		//	*(ptr + 7) = (char)0xEB; // jnz => jmp
		//}
		
		// note: this is a different way of nopping window creation, i'm assuming there are like a shitload of inits here we shouldn't skip
		// i know at the very least it registers datatables which are important
		{
			// IVideoMode::CreateGameWindow
			char* ptr = (char*)engineAddress + 0x1CD0ED;
			TempReadWrite rw(ptr);

			// prevent game window from being created
			*ptr = (char)0x90;
			*(ptr + 1) = (char)0x90;
			*(ptr + 2) = (char)0x90;
			*(ptr + 3) = (char)0x90;
			*(ptr + 4) = (char)0x90;
		}

		{
			// IVideoMode::CreateGameWindow
			char* ptr = (char*)engineAddress + 0x1CD146;
			TempReadWrite rw(ptr);

			// prevent game from calling a matsystem function that will crash here
			//*ptr = (char)0x90;
			//*(ptr + 1) = (char)0x90;
			//*(ptr + 2) = (char)0x90;
			//*(ptr + 3) = (char)0x90;
			//*(ptr + 4) = (char)0x90;
		}

		{
			// IVideoMode::CreateGameWindow
			char* ptr = (char*)engineAddress + 0x1CD160;
			TempReadWrite rw(ptr);

			// prevent game from complaining about window not being created
			*ptr = (char)0x90;
			*(ptr + 1) = (char)0x90;
		}

		CommandLine()->AppendParm("-noshaderapi", 0);
	}

	CDedicatedExports* dedicatedExports = new CDedicatedExports;
	dedicatedExports->vtable = dedicatedExports;
	dedicatedExports->Sys_Printf = Sys_Printf;
	dedicatedExports->RunServer = RunServer;

	CDedicatedExports** exports = (CDedicatedExports**)((char*)engineAddress + 0x13F0B668);
	*exports = dedicatedExports;

	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)engineAddress + 0x1CDC80, &IsGameActiveWindowHook, reinterpret_cast<LPVOID*>(&IsGameActiveWindow));
	
	// extra potential patches:
	// nop engine.dll+1c67d1 and +1c67d8 to skip videomode creategamewindow
	// also look into launcher.dll+d381, seems to cause renderthread to get made
	// this crashes HARD if no window which makes sense tbh
	// also look into materialsystem + 5B344 since it seems to be the base of all the renderthread stuff

	// big note: datatable gets registered in window creation
	// make sure it still gets registered

	// add cmdline args that are good for dedi
	CommandLine()->AppendParm("-nomenuvid", 0);
	CommandLine()->AppendParm("-nosound", 0);
	CommandLine()->AppendParm("+host_preload_shaders", "0");
}

typedef void(*Tier0_InitOriginType)();
Tier0_InitOriginType Tier0_InitOrigin;
void Tier0_InitOriginHook()
{
	// disable origin on dedicated
	// for any big ea lawyers, this can't be used to play the game without origin, game will throw a fit if you try to do anything without an origin id as a client
	// for dedi it's fine though, game doesn't care if origin is disabled as long as there's only a server

	if (!IsDedicated())
		Tier0_InitOrigin();
}

void InitialiseDedicatedOrigin(HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, GetProcAddress(GetModuleHandleA("tier0.dll"), "Tier0_InitOrigin"), &Tier0_InitOriginHook, reinterpret_cast<LPVOID*>(&Tier0_InitOrigin));
}