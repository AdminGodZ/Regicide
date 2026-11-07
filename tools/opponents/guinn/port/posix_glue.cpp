// POSIX stand-ins for the pieces of GuiNN Checkers 2.06 that live in Windows-only files
// (guiWindows.cpp, registry.cpp, kr_db.cpp), for the CheckerBoard-API dylib build.

#include <cstring>
#include <string>

#include "engine.h"
#include "kr_db.h"
#include "registry.h"

// Search info goes to the CheckerBoard status string (what guiWindows.cpp does in DLL mode).
void DisplayText(const char* text)
{
	if (checkerBoard.bActive && checkerBoard.infoString) {
		std::strncpy(checkerBoard.infoString, text, 1023);
		checkerBoard.infoString[1023] = 0;
	}
}

// No registry: settings are not persisted, and "not found" keeps the compiled-in defaults
// (128 MB hash, book on, built-in endgame db). Use enginecommand "set ..." to change them.
void reg_set_int(char*, int) {}
int reg_get_int(char*, int*) { return 1; }
void reg_set_string(char*, char*) {}
int reg_get_string(char*, char*, int) { return 1; }
void save_hashsize(int) {}
void save_book_setting(int) {}
void save_dbpath(char*) {}
void save_enable_wld(int) {}
void save_dbmbytes(int) {}
void save_max_dbpieces(int) {}
int get_hashsize(int*) { return 1; }
int get_book_setting(int*) { return 1; }
int get_dbpath(char*, int) { return 1; }
int get_enable_wld(int*) { return 1; }
int get_dbmbytes(int*) { return 1; }
int get_max_dbpieces(int*) { return 1; }

// The KingsRow egdb driver (egdb64.dll) is not available. Instead load GuiNN's own 4-piece
// win/loss/draw database (database.jef) once, as the standalone GuiNN program does.
void init_egdb(char msg[1024])
{
	if (checkerBoard.did_egdb_init && !checkerBoard.request_egdb_init)
		return;
	checkerBoard.did_egdb_init = true;
	checkerBoard.request_egdb_init = false;
	if (engine.dbInfo.loaded && engine.dbInfo.type == dbType::WIN_LOSS_DRAW)
		close_gui_databases(engine.dbInfo);
	if (checkerBoard.enable_wld) {
		InitializeGuiDatabases(engine.dbInfo);
		if (msg)
			std::snprintf(msg, 1024, "%s", engine.dbInfo.loaded ? "4-piece database loaded" : "database.jef not found");
	}
}

void check_wld_dir(const char*, char* reply)
{
	std::strcpy(reply, "no KingsRow egdb driver in this build");
}

// enginecommand "staticevaluation <fen>" (CheckersGUI::get_eval_string on Windows).
void cbport_eval_string(const Board& board, std::string& evalstr)
{
	EvalNetInfo& netInfo = engine.searchThreadData.stack->netInfo;
	netInfo.netIdx = (int)CheckersNet::GetGamePhase(board);
	engine.evalNets[netInfo.netIdx]->ComputeFirstLayerValues(board, engine.searchThreadData.nnValues, netInfo.firstLayerValues);
	int eval = board.EvaluateBoard(0, engine.searchThreadData, netInfo, 100);
	if (board.sideToMove == WHITE)
		eval = -eval; // + is good for black
	evalstr = "eval " + std::to_string(eval) + " (+ good for black)";
}
