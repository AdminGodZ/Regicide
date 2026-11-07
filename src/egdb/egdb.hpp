#pragma once
// Endgame database for 8x8 English checkers (win/loss/draw, plus moves-to-conversion).
//
// Values are exact game-theoretic results under the rules in types.hpp (compulsory captures,
// completed multi-jumps, promotion ends the move, short kings, no legal move = loss); positions
// that can only repeat are draws. Every position with up to max_pieces() pieces can be probed,
// including positions where the side to move has a capture: those are not stored and are
// resolved inside probe() by a capture-only search into smaller slices.
//
// Data lives in one memory-mapped file per material slice (see store.hpp). After init() the
// probes are lock-free, allocation-free and safe to call from any number of threads.

#include <cstddef>
#include <cstdint>
#include <string>

#include "types.hpp"

namespace regicide::egdb {

enum class Wld : std::int8_t { Unknown = 0, Win = 1, Draw = 2, Loss = 3 };  // from side to move's view

// Maps every complete slice file found in `dir`. If the files fit in `cache_mb` MiB they are also
// pre-faulted into RAM; otherwise pages are loaded on demand by the OS. Call before probing. Once a
// directory has loaded, later calls keep it and return true; after a failure another directory may be tried.
bool init(const std::string& dir, std::size_t cache_mb);

// Largest piece count N such that every position with <= N pieces can be probed; 0 if none.
int max_pieces();

Wld probe(Bitboard black, Bitboard white, Bitboard kings, Color stm);

// Moves-to-conversion for decided positions: the number of plies, under optimal play (winner
// minimises, loser maximises), before a *converting* move is played, where a converting move is a
// capture, a promotion, or a man move that advances that side's leading man to a new rank.
// 0 means a converting move is played right now (e.g. the side to move has a capture).
// Following MTC-decreasing moves from a won position always realises the win.
// Returns MTC_DRAW for draws and MTC_UNKNOWN when not available.
inline constexpr int MTC_DRAW = -1;
inline constexpr int MTC_UNKNOWN = -2;
int probe_mtc(Bitboard black, Bitboard white, Bitboard kings, Color stm);
int max_mtc_pieces();

}  // namespace regicide::egdb
