#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Lua/Interface.h>
#include <eiface.h>
#include <dbg.h>
#include <filewatch.hpp>

const std::string GetGamePath(GarrysMod::Lua::ILuaBase* LUA) 
{
	SourceSDK::FactoryLoader engine_loader("engine");
	IVEngineServer* engine_server = engine_loader.GetInterface<IVEngineServer>(INTERFACEVERSION_VENGINESERVER);
	if (engine_server == nullptr)
		LUA->ThrowError("Failed to load required IVEngineServer interface");

	std::string game_dir;
	game_dir.resize(1024);
	engine_server->GetGameDir(&game_dir[0], static_cast<int32_t>(game_dir.size()));
	game_dir.resize(std::strlen(game_dir.c_str()));

	return game_dir;
}

void HookRun(GarrysMod::Lua::ILuaBase* LUA, const char* path, const char* event_type)
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "hook");
			LUA->GetField(-1, "Run");
				LUA->PushString("FileChange");
				LUA->PushString(path);
				LUA->PushString(event_type);
			LUA->PCall(3, 0, 0);
	//LUA->Pop();
}

filewatch::FileWatch* watcher = nullptr;
GMOD_MODULE_OPEN()
{
	std::string game_dir = GetGamePath(LUA);

	watcher = new filewatch::FileWatch(game_dir, [LUA](const std::string& path, const filewatch::Event event_type) {
		const char* type;
		switch (event_type) {
			case filewatch::Event::added:
				type = "ADD";
				break;
			case filewatch::Event::modified:
				type = "MODIFY";
				break;
			case filewatch::Event::removed:
				type = "REMOVE";
				break;
			case filewatch::Event::renamed_new:
				type = "RENAME_NEW";
				break;
			case filewatch::Event::renamed_old:
				type = "RENAME_OLD";
				break;
			default:
				type = "UNKNOWN";
				break;
		}

		HookRun(LUA, path.c_str(), type);
	});

	return 0;
}

GMOD_MODULE_CLOSE()
{
	if (watcher != nullptr) {
		watcher->~FileWatch();
	}

	return 0;
}
