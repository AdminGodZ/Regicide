#include "search.hpp"
#include "egdb/egdb.hpp"
#include "checkers.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace regicide;
Position rp(const cbt::Board& b){ Position p; if(!p.set_fen(b.fen()))std::abort();return p; }
int main(){
 std::string db;std::getline(std::ifstream("reports/round6/db-path.txt"),db);
 if(!egdb::init(db,16))return 2;
 Engine e;e.set_hash_mb(1);e.set_evaluator(Evaluator::hce());e.set_egdb_pieces(3);e.set_egdb_mtc_pieces(3);
 Limits l;l.depth=1;l.nodes=100;l.use_book=false;
 int checked=0;
 for(int a=1;a<=32;a++)for(int b=a+1;b<=32;b++)for(int w=1;w<=32;w++){
  if(w==a||w==b)continue;
  auto R=*cbt::Board::from_fen("B:WK"+std::to_string(w)+":BK"+std::to_string(a)+",K"+std::to_string(b));
  auto p=rp(R);if(egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),BLACK)!=egdb::Wld::Win)continue;
  e.set_position(p);auto fresh=e.go(l);if(fresh.score<SCORE_KNOWN_WIN||e.claim(fresh)!=Engine::Claim::Win)continue;
  auto mc=cbt::parse_move(R,move_to_string(fresh.best));if(!mc||R.is_conversion(*mc))continue;auto C=R.after(*mc);
  for(auto ms:C.legal_moves()){
   if(C.is_conversion(ms))continue;auto S=C.after(ms);
   for(auto mw:S.legal_moves()){
    if(S.is_conversion(mw))continue;auto W=S.after(mw);
    for(auto mr:W.legal_moves()){
     if(W.is_conversion(mr)||!(W.after(mr)==R))continue;
     std::vector<Position> hist{rp(S),rp(W),p,rp(C),rp(S),rp(W)};
     e.set_position(p,hist);auto r=e.go(l);++checked;
     if(move_to_string(r.best)!=cbt::move_to_pdn(*mc)||e.claim(r)!=Engine::Claim::Win)continue;
     std::cout<<"root "<<R.fen()<<" score "<<r.score<<" claim Win best "<<move_to_string(r.best)<<" nodes "<<r.nodes<<"\n";
     std::cout<<"set gamehist "<<S.fen()<<" "<<cbt::move_to_pdn(mw)<<" "<<cbt::move_to_pdn(mr)<<" "<<cbt::move_to_pdn(*mc)<<" "<<cbt::move_to_pdn(ms)<<" "<<cbt::move_to_pdn(mw)<<" "<<cbt::move_to_pdn(mr)<<"\n";
     std::cout<<"played "<<cbt::move_to_pdn(*mc)<<" reply "<<cbt::move_to_pdn(ms)<<" makes "<<S.fen()<<" third occurrence at ply 8; checked "<<checked<<" cycles\n";
     return 0;
    }
   }
  }
 }
 std::cout<<"no counterexample in "<<checked<<" cycles\n";return 1;
}
