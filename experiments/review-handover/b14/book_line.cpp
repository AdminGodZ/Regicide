#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "book.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
using namespace regicide;
std::vector<std::string> split(const std::string&s){std::vector<std::string>v;std::stringstream ss(s);for(std::string x;std::getline(ss,x,'\t');)v.push_back(x);return v;}
int main(int ac,char**av){if(ac<3)return 2;auto book=Book::builtin();if(!egdb::init("/Users/timheld/regicide-data/egdb",64))return 3;std::string mode=av[1];std::map<Key,std::vector<std::pair<std::string,Position>>> targets;
 if(mode=="pool"){std::ifstream t(av[3]);for(std::string l;std::getline(t,l);){auto f=split(l);Position p;if(!p.set_fen(f[1]))return 4;targets[p.key()].push_back({f[0],p});}}
 std::ifstream input(av[2]);std::cout<<"game\tply\tfen\trev\tplayed\tbook\teligible\tdb\texcluded\ttarget\n";
 for(std::string l;std::getline(input,l);){auto f=split(l);Position p;if(!f[1].empty()&&!p.set_fen(f[1]))return 5;Color reg=f[2]=="B"?BLACK:WHITE;int nopen=std::stoi(f[3]),ply=0;std::istringstream ms(f[4]);
  for(std::string text;ms>>text;++ply){Move m;if(!parse_move(p,text,m)){std::cerr<<"illegal "<<f[0]<<' '<<ply<<' '<<text<<'\n';return 6;}bool use=mode=="own"?p.side_to_move()==reg&&ply>=nopen:targets.contains(p.key());
   if(use){int db=p.piece_count()<=7?int(egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move())):0;bool el=p.reversible_plies()==0&&db==0;auto bm=book->probe(p,0,0,book.get());std::string ids="-";if(mode=="pool"){ids="";for(auto&[id,q]:targets[p.key()])if(q==p){if(!ids.empty())ids+=',';ids+=id;}if(ids.empty()){p.make_move(m);continue;}}
    std::cout<<f[0]<<'\t'<<ply+1<<'\t'<<p.fen()<<'\t'<<p.reversible_plies()<<'\t'<<move_text(p,m)<<'\t'<<(bm?move_text(p,*bm):"-")<<'\t'<<el<<'\t'<<db<<'\t'<<book->avoids(p,m)<<'\t'<<ids<<'\n';
   }p.make_move(m);
  }
 }return 0;}
