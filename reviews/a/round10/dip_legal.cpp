#include "checkers.hpp"
#include <iostream>
#include <sstream>
#include <vector>
int main(){for(std::string l;std::getline(std::cin,l);){std::vector<std::string>f;std::stringstream ss(l);for(std::string x;std::getline(ss,x,'\t');)f.push_back(x);auto b=cbt::play_moves(cbt::Board::start(),f[1]);if(!b||b->fen()!=f[2])return 1;auto ms=b->legal_moves();std::cout<<f[0]<<'\t'<<b->fen()<<'\t'<<ms.size()<<'\t';for(auto m:ms)std::cout<<cbt::move_to_pdn(m)<<' ';std::cout<<'\n';}}
