#include "search.hpp"
#include "checkers.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>
using namespace regicide;
Position rp(const cbt::Board& b){Position p;if(!p.set_fen(b.fen()))std::abort();return p;}
int main(){
 Engine e;e.set_hash_mb(1);std::mt19937 rng(7007);
 long roots=0,candidates=0,draws=0,conversions=0,terminal=0;
 auto check=[&](const std::vector<cbt::Board>& path){
  const auto& b=path.back();std::vector<Position> hist;for(size_t i=0;i+1<path.size();i++)hist.push_back(rp(path[i]));
  e.set_position(rp(b),hist);int rev=0;size_t start=0;
  for(size_t i=1;i<path.size();i++){
   if(path[i-1].black==path[i].black&&path[i-1].white==path[i].white&&path[i-1].kings==path[i].kings){rev++;continue;}
   bool conv=false;for(auto m:path[i-1].legal_moves())if(path[i-1].after(m)==path[i]){conv=path[i-1].is_conversion(m);break;}
   if(conv){rev=0;start=i;}else rev++;
  }
  ++roots;
  for(auto m:b.legal_moves()){
   auto c=b.after(m);bool cv=b.is_conversion(m);int cr=cv?0:rev+1;
   int reps=cv?1:1+std::count(path.begin()+start,path.end(),c);
   bool term=c.legal_moves().empty();bool want=!term&&(cr>=79||reps>=3);
   SearchResult r{};if(!parse_move(rp(b),cbt::move_to_pdn(m),r.best))return false;r.score=0;
   bool got=e.claim(r)==Engine::Claim::Draw;
   ++candidates;draws+=got;conversions+=cv;terminal+=term;
   if(got!=want){std::cout<<"MISMATCH "<<b.fen()<<" move "<<cbt::move_to_pdn(m)<<" rev "<<rev<<" reps "<<reps<<" got "<<got<<" want "<<want<<"\n";return false;}
  }
  return true;
 };
 for(auto file:{"reports/round6/evidence/gamehist77.txt","reports/round6/evidence/gamehist9-78.txt"}){
  std::ifstream f(file);std::string tok;f>>tok>>tok>>tok;auto b=*cbt::Board::from_fen(tok);std::vector<cbt::Board> path{b};
  if(!check(path))return 1;
  while(f>>tok){auto m=cbt::parse_move(path.back(),tok);if(!m)return 2;path.push_back(path.back().after(*m));if(!check(path))return 1;}
 }
 for(int game=0;game<300;game++){
  cbt::Board b=game%3==0?cbt::Board::start():*cbt::Board::from_fen(game%3==1?"B:WK2,K8,K12,K18,K20,K28,K29:BK13,K30":"B:WK32:BK1");
  std::vector<cbt::Board> path{b};
  for(int ply=0;ply<100;ply++){
   if(!check(path))return 1;auto moves=path.back().legal_moves();if(moves.empty())break;
   auto m=moves[rng()%moves.size()];path.push_back(path.back().after(m));
   if(std::count(path.begin(),path.end(),path.back())>=3)break;
  }
 }
 std::cout<<"matched referee: roots "<<roots<<" legal candidates "<<candidates<<" draw claims "<<draws<<" converting moves "<<conversions<<" terminal moves "<<terminal<<"\n";
}
