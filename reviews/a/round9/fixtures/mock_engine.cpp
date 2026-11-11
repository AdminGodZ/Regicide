#include "checkers.hpp"
#include "cb_api.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
static std::string logfile;
static int claim=CB_UNKNOWN;
extern "C" int enginecommand(const char* cmd,char* reply){
 if(std::strncmp(cmd,"set log ",8)==0)logfile=cmd+8;
 if(std::strncmp(cmd,"set claim ",10)==0)claim=std::stoi(cmd+10);
 if(!logfile.empty())std::ofstream(logfile,std::ios::app)<<"CMD "<<cmd<<"\n";
 std::strcpy(reply,std::strcmp(cmd,"name")==0?"Review fixture":"ok");return 1;
}
extern "C" int getmove(int a[8][8],int color,double mt,char* str,int*,int info,int more,CBmove*){
 auto b=cbt::Board::from_board8(a,color==CB_BLACK?cbt::Color::Black:cbt::Color::White);
 if(!logfile.empty())std::ofstream(logfile,std::ios::app)<<"CALL "<<b.fen()<<" maxtime "<<mt<<" info "<<info<<" moreinfo "<<more<<"\n";
 auto moves=b.legal_moves();if(moves.empty())return CB_LOSS;
 auto m=moves.front();
 if((b.black|b.white)==b.kings){
  const char* wanted=b.stm==cbt::Color::Black?(b.black==cbt::bit(0)?"1-5":"5-1"):(b.white==cbt::bit(31)?"32-28":"28-32");
  if(auto p=cbt::parse_move(b,wanted))m=*p;
 }
 b.after(m).to_board8(a);std::strcpy(str,"fixture");return claim;
}
