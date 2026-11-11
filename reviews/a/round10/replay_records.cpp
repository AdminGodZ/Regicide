#include "checkers.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
int main(){
 std::string line;
 while(std::getline(std::cin,line)){
  auto a=line.find('\t'),z=line.find('\t',a+1);auto id=line.substr(0,a),fen=line.substr(a+1,z-a-1);auto b=fen.empty()?std::optional(cbt::Board::start()):cbt::Board::from_fen(fen);
  if(!b){std::cout<<"ERR\t"<<id<<"\tFEN\nEND\t"<<id<<"\n"<<std::flush;continue;}
  std::vector<cbt::Board> window{*b};int rev=0,p=0;std::istringstream ms(line.substr(z+1));std::string tok;
  while(ms>>tok){
   auto m=cbt::parse_move(*b,tok);if(!m){std::cout<<"ERR\t"<<id<<"\tmove "<<p+1<<" "<<tok<<"\n";break;}
   auto before=*b;bool conv=b->is_conversion(*m);*b=b->after(*m);++p;
   if(conv){rev=0;window.assign(1,*b);}else{++rev;window.push_back(*b);}
   int repetitions=std::count(window.begin(),window.end(),*b);int replies=b->legal_moves().size();std::string result="-";
   if(!replies)result=before.stm==cbt::Color::Black?"1-0":"0-1";else if(repetitions>=3||rev>=79)result="1/2-1/2";
   std::cout<<id<<"\t"<<p<<"\t"<<(before.stm==cbt::Color::Black?"B":"W")<<"\t"<<cbt::move_to_pdn(*m)<<"\t"<<b->fen()<<"\t"<<conv<<"\t"<<replies<<"\t"<<rev<<"\t"<<repetitions<<"\t"<<result<<"\t"<<int(m->from)+1<<","<<int(m->to)+1<<","<<m->captures<<"\n";
  }std::cout<<"END\t"<<id<<"\n"<<std::flush;
 }
}
