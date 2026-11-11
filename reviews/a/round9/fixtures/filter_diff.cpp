#ifdef BEFORE
#include "master-src/src/search.cpp"
#else
#include "claim-src/src/search.cpp"
#endif
#include "checkers.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <random>
#include <numeric>
using namespace regicide;
// Test-only read access to the actual root list retained by Engine::go; no search/filter code is replaced.
struct WorkersTag{using type=std::vector<std::unique_ptr<Worker>> Engine::*;friend type access(WorkersTag);};
template<class Tag,typename Tag::type M>struct Reveal{friend typename Tag::type access(Tag){return M;}};
template struct Reveal<WorkersTag,&Engine::workers_>;
using Signature=std::tuple<int,int,Bitboard>;
Signature sig(Move m){return {m.from,m.to,m.captures};}
long roots=0,drawroots=0,allmoves=0,kept=0,removed=0,empty=0;long childstates[4]{};
std::mt19937_64 rng(0xA9F17E);
int code(egdb::Wld w){return w==egdb::Wld::Win?0:w==egdb::Wld::Draw?1:w==egdb::Wld::Loss?2:3;}
int main(){
 std::string db;std::getline(std::ifstream("reports/round6/db-path.txt"),db);if(!egdb::init(db,16))return 2;
 auto e=std::make_unique<Engine>();e->set_hash_mb(1);e->set_egdb_pieces(3);e->set_stop_poll([]{return true;});
 auto check=[&](const Position&p,bool special=false){
  ++roots;auto wld=egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move());if(wld!=egdb::Wld::Draw)return true;++drawroots;
  MoveList ms;generate_moves(p,ms);auto cb=*cbt::Board::from_fen(p.fen());auto refs=cb.legal_moves();
  std::set<Signature>want,got;for(auto rm:refs){
   auto child=cb.after(rm);Position q;q.set_fen(child.fen());auto r=egdb::probe(q.pieces(BLACK),q.pieces(WHITE),q.kings(),q.side_to_move());++childstates[code(r)];
   Move m;if(!parse_move(p,cbt::move_to_pdn(rm),m))return false;
   if(r!=egdb::Wld::Win)want.insert(sig(m));
  }
  if(want.empty()){++empty;std::cout<<"no holding move at "<<p.fen()<<"\n";return false;}
  e->set_position(p);Limits l;l.nodes=1;l.use_book=false;auto r=e->go(l);
  const auto& workers=e.get()->*access(WorkersTag{});if(workers.size()!=1)return false;
  for(auto&rm:workers[0]->roots)got.insert(sig(rm.move));
  allmoves+=ms.size();kept+=got.size();removed+=ms.size()-got.size();
  if(got!=want||!got.contains(sig(r.best))){std::cout<<"FILTER mismatch "<<p.fen()<<" expected "<<want.size()<<" got "<<got.size()<<" fallback "<<move_to_string(r.best)<<"\n";return false;}
  if(special){
   Position c=p;c.make_move(r.best);std::cout<<"stopped "<<p.fen()<<" -> "<<move_to_string(r.best)<<" child WLD "<<code(egdb::probe(c.pieces(BLACK),c.pieces(WHITE),c.kings(),c.side_to_move()))<<" kept "<<kept<<"/"<<allmoves<<"\n";
   for(int mode=0;mode<2;mode++){
    Limits a;a.nodes=1;a.use_book=false;if(mode==0)a.infinite=true;else a.searchmoves={ms[0]};
    e->set_position(p);e->go(a);auto&rr=(e.get()->*access(WorkersTag{}))[0]->roots;
    if(rr.size()!=(mode==0?ms.size():1))return false;
   }
  }
  return true;
 };
 Position special;special.set_fen("B:WK30:BK25,K26");if(!check(special,true))return 1;
 // Every non-overlapping two-piece board, both men/king types and both sides, excluding uncrowned men on crown rows.
 for(int a=0;a<32;a++)for(int b=0;b<32;b++)if(a!=b)for(int kind=0;kind<4;kind++)for(Color side:{BLACK,WHITE}){
  bool bk=kind&1,wk=kind&2;if((!bk&&(bit(a)&promo_row(BLACK)))||(!wk&&(bit(b)&promo_row(WHITE))))continue;
  Position p;p.set(bit(a),bit(b),(bk?bit(a):0)|(wk?bit(b):0),side);if(!check(p))return 1;
 }
 // All 2v1 king layouts, both material colours and both sides.
 for(int a=0;a<32;a++)for(int b=a+1;b<32;b++)for(int c=0;c<32;c++)if(c!=a&&c!=b)for(int swap=0;swap<2;swap++)for(Color side:{BLACK,WHITE}){
  Bitboard two=bit(a)|bit(b),one=bit(c);Position p;p.set(swap?one:two,swap?two:one,two|one,side);if(!check(p))return 1;
 }
 for(int i=0;i<4000;i++){
  std::array<int,32> sq;std::iota(sq.begin(),sq.end(),0);std::shuffle(sq.begin(),sq.end(),rng);
  Bitboard black=bit(sq[0]),white=bit(sq[1]),kings=0;if(i&1)black|=bit(sq[2]);else white|=bit(sq[2]);
  for(int j=0;j<3;j++){Color owner=black&bit(sq[j])?BLACK:WHITE;if((rng()&1)||(bit(sq[j])&promo_row(owner)))kings|=bit(sq[j]);}
  Position p;p.set(black,white,kings,Color(i&1));if(!check(p))return 1;
 }
 std::cout<<"seed 0xA9F17E positions "<<roots<<" drawn roots "<<drawroots<<" legal moves "<<allmoves<<" retained "<<kept<<" removed "<<removed<<" empty "<<empty<<" child Win/Draw/Loss/Unknown "<<childstates[0]<<"/"<<childstates[1]<<"/"<<childstates[2]<<"/"<<childstates[3]<<"\n";
}
