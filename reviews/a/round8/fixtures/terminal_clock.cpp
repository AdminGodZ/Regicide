#include "search.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
#include "checkers.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <algorithm>
using namespace regicide;
std::mt19937 rng(42);
std::vector<cbt::Board> back;
bool buildback(const cbt::Board& cur,int n){
 if(!n)return true;auto actor=cbt::opposite(cur.stm);std::vector<cbt::Board> cand;
 for(int sq=0;sq<32;++sq)if(cur.pieces(actor)&cbt::bit(sq)){
  int x,y;cbt::square_xy(sq,x,y);
  for(int dx:{-1,1})for(int dy:{-1,1}){int to=cbt::xy_square(x+dx,y+dy);if(to<0||(cur.occupied()&cbt::bit(to)))continue;
   auto p=cur;p.stm=actor;auto& bits=actor==cbt::Color::Black?p.black:p.white;bits^=cbt::bit(sq)|cbt::bit(to);p.kings=p.black|p.white;
   auto m=cbt::parse_move(p,std::to_string(to+1)+"-"+std::to_string(sq+1));if(!m||m->is_capture()||!(p.after(*m)==cur))continue;
   if(std::find(back.begin(),back.end(),p)!=back.end())continue;cand.push_back(p);
  }
 }
 std::shuffle(cand.begin(),cand.end(),rng);for(auto p:cand){back.push_back(p);if(buildback(p,n-1))return true;back.pop_back();}return false;
}

int main(){
 auto b=*cbt::Board::from_fen("B:WK10:BK1,K6");back={b};if(!buildback(b,78))return 3;std::reverse(back.begin(),back.end());
 std::vector<Position> hist;std::string gh="set gamehist "+back.front().fen();
 for(size_t i=0;i+1<back.size();i++){
  Position p;p.set_fen(back[i].fen());hist.push_back(p);
  auto ms=back[i].legal_moves();auto it=std::find_if(ms.begin(),ms.end(),[&](auto m){return back[i].after(m)==back[i+1];});
  if(it==ms.end()||back[i].is_conversion(*it))return 4;gh+=" "+cbt::move_to_pdn(*it);
 }
 std::ofstream("reports/round8/evidence/terminal-history78.txt")<<gh<<"\n";
 Engine e;e.set_hash_mb(1);e.set_evaluator(Evaluator::hce());e.set_egdb_pieces(0);Position p;p.set_fen(b.fen());e.set_position(p,hist);Limits l;l.depth=2;l.nodes=20000;l.use_book=false;auto r=e.go(l);
 auto m=cbt::parse_move(b,move_to_string(r.best));if(!m)return 5;
 std::cout<<"root "<<b.fen()<<" reversible 78 move "<<move_to_string(r.best)<<" score "<<r.score<<" claim "<<int(e.claim(r))<<" child replies "<<b.after(*m).legal_moves().size()<<" nodes "<<r.nodes<<"\n";
 std::cout<<gh<<"\n";
}
