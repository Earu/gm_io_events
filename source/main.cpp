#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Lua/Interface.h>
#include <eiface.h>
#include <dbg.h>
#include "filewatch.hpp"

std::string GetGamePath(GarrysMod::Lua::ILuaBase *LUA) 
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

GMOD_MODULE_OPEN()
{
	std::string game_dir = GetGamePath(LUA);
	std::string watch_dir = game_dir.append("/lua/minge/config.json");

	filewatch::FileWatch<std::string>(watch_dir, [](const std::string &path, const filewatch::Event event_type) {
		Msg(path.c_str());
		switch (event_type) {
			case filewatch::Event::added:
				Msg(" ADD\n");
				break;
			case filewatch::Event::modified:
				Msg(" MODIFY\n");
				break;
			case filewatch::Event::removed:
				Msg(" REMOVE\n");
				break;
			case filewatch::Event::renamed_new:
				Msg(" RENAME NEW\n");
				break;
			case filewatch::Event::renamed_old:
				Msg(" RENAME OLD\n");
				break;
			default:
				Msg(" UNKNOWN\n");
				break;
		}
	});

	return 0;
}

GMOD_MODULE_CLOSE( )
{
	return 0;
}
