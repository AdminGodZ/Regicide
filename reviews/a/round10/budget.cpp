#include "search.hpp"
#include "egdb/egdb.hpp"
#include "checkers.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace regicide;
std::vector<std::string> fields(std::string l){std::vector<std::string>f;std::stringstream s(l);for(std::string x;std::getline(s,x,'\t');)f.push_back(x);return f;}
int main(int argc,char**argv){
 if(!egdb::init("/home/admingod/regicide-data/egdb",16))return 2;
 std::cerr<<"DB max WLD="<<egdb::max_pieces()<<" MTC="<<egdb::max_mtc_pieces()<<"\n";
 auto e=std::make_unique<Engine>();e->set_threads(1);e->set_hash_mb(128);e->set_egdb_pieces(8);e->set_book(nullptr);e->set_avoid(nullptr);
 for(std::string l;std::getline(std::cin,l);){
  auto f=fields(l);if(f.size()!=4)return 3;Position p;cbt::Board cb=cbt::Board::start();std::vector<Position>h;std::stringstream ss(f[1]);
  for(std::string tok;ss>>tok;){
   auto rm=cbt::parse_move(cb,tok);if(!rm){std::cerr<<f[0]<<" bad reference "<<tok<<"\n";return 4;}
   Move m;if(!parse_move(p,tok,m)){std::cerr<<f[0]<<" bad engine "<<tok<<"\n";return 5;}
   h.push_back(p);p.make_move(m);cb=cb.after(*rm);if(p.fen()!=cb.fen())return 6;
  }
  if(p.fen()!=f[2]){std::cerr<<f[0]<<" FEN mismatch "<<p.fen()<<" expected "<<f[2]<<"\n";return 7;}
  e->new_game();e->set_position(p,h);Limits lim;lim.nodes=std::stoull(f[3]);lim.use_book=false;
  auto r=e->go(lim);auto rm=cbt::parse_move(cb,move_to_string(r.best));if(!rm){std::cerr<<"illegal best\n";return 8;}
  std::cout<<f[0]<<'\t'<<p.fen()<<'\t'<<p.reversible_plies()<<'\t'<<int(egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move()))<<'\t'<<f[3]<<'\t'<<move_to_string(r.best)<<'\t'<<r.score<<'\t'<<r.depth<<'\t'<<r.nodes<<'\t'<<r.partial<<'\t'<<r.completed_score<<'\t'<<r.time_ms<<std::endl;
 }
}
