#include <fstream>
#include <iostream>
#include "search.hpp"
#include "movegen.hpp"
using namespace regicide;
int main(){Position p;p.set_fen("B:W21,K22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,K5,9");std::vector<Position>hist;for(auto t:{"9-13","22-18"}){Move m;if(!parse_move(p,t,m))return 2;hist.push_back(p);p.make_move(m);}Move old;if(!parse_move(p,"5-9",old))return 3;auto book=std::make_shared<Book>();if(!book->load("king-fixture/avoid.bin"))return 4;Engine e;e.set_hash_mb(1);e.set_threads(1);e.set_evaluator(Evaluator::hce());e.set_book(book);e.set_avoid(book);e.set_position(p,hist);Limits l;l.depth=1;auto r=e.go(l);std::cout<<p.fen()<<" rev="<<p.reversible_plies()<<" old="<<move_text(p,old)<<" excluded="<<book->avoids(p,old)<<" selected="<<move_text(p,r.best)<<" nodes="<<r.nodes<<'\n';return r.best==old||p.reversible_plies()!=1;}
