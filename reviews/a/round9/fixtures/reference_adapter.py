from pathlib import Path
import os
b=Path(os.environ.get('REVIEW_OUT','reports/round7'));b.mkdir(parents=True,exist_ok=True)
source=Path(os.environ.get('CHECKERBOARD_SRC','reports/round5/checkerboard-src'));src=source/'CheckerBoard.c'
def fn(name,path=src):
 s=path.read_text();at=s.index(name);at=s.rfind('\n',0,at)+1;i=s.index('{',at);level=1;j=i+1
 while level:
  level+= (s[j]=='{')-(s[j]=='}');j+=1
 return s[at:j]
prefix='\n#include "checkers.hpp"\n#include "cb_api.h"\n#include <vector>\n#include <cstring>\n#include <iostream>\n#include <iomanip>\n#include <algorithm>\n#include <cassert>\nusing Board8x8=int[8][8];\n#define CB_CHANGECOLOR(c) ((c)^3)\nstruct pos{uint32_t bm,bk,wm,wk;};\nstruct entry{CBmove move;};\nstruct PDNgame{std::vector<entry> moves;int movesindex=0;char FEN[256];int gametype=21;};\nint get_startcolor(int){return CB_BLACK;}\nvoid InitCheckerBoard(Board8x8 a){cbt::Board::start().to_board8(a);}\nvoid FENtoboard8(Board8x8 a,const char* fen,int* col,int){auto b=*cbt::Board::from_fen(fen);b.to_board8(a);*col=b.stm==cbt::Color::Black?CB_BLACK:CB_WHITE;}\nvoid boardtobitboard(Board8x8 a,pos* p){auto b=cbt::Board::from_board8(a,cbt::Color::Black);*p={b.black&~b.kings,b.black&b.kings,b.white&~b.kings,b.white&b.kings};}\nvoid domove(CBmove& m,Board8x8 a){a[m.from.x][m.from.y]=0;a[m.to.x][m.to.y]=m.newpiece;for(int i=0;i<m.jumps;++i)a[m.del[i].x][m.del[i].y]=0;}\nvoid undomove(CBmove& m,Board8x8 a){a[m.to.x][m.to.y]=0;a[m.from.x][m.from.y]=m.oldpiece;for(int i=0;i<m.jumps;++i)a[m.del[i].x][m.del[i].y]=m.delpiece[i];}\nint coorstonumber(int x,int y,int){return cbt::xy_square(x,y)+1;}\nvoid board8toFEN(Board8x8 a,std::string& s,int c,int){s=cbt::Board::from_board8(a,c==CB_BLACK?cbt::Color::Black:cbt::Color::White).fen();}\nconstexpr int ENGINECOMMAND_REPLY_SIZE=1024;\nstd::string command;\nint enginecommand(const char* c,char*){command=c;return 1;}\n'
prefix+='\n'+fn('bool is_conversion_move',source/'utility.c')+'\n'+fn('void detect_nonconversion_draws',source/'utility.c')+'\n'+fn('void send_game_history',source/'utility.c')+'\n'+fn('double maxtime_for_non_incremental_tc')
prefix=prefix.replace('struct entry{CBmove move;};','struct gamebody_entry{CBmove move;char comment[2048]{},analysis[2048]{};};using entry=gamebody_entry;')
prefix=prefix.replace('char FEN[256];int gametype=21;','char FEN[256];char resultstring[32]{};int gametype=21;')
stub=r"""
#include <ctime>
#include <cstdarg>
using DWORD=unsigned long;using LPVOID=void*;using WPARAM=uintptr_t;
#define TRUE true
#define FALSE false
#define SND_FILENAME 1
#define SND_ASYNC 2
#define CLK_TCK CLOCKS_PER_SEC
#define MAKELPARAM(a,b) 0
constexpr int MAXMOVES=128,MAX_PATH=260,COMMENTLENGTH=2048,TB_CHANGEBITMAP=0,MOVESPLAY=0;
enum {NORMAL,ENGINEMATCH,AUTOPLAY,ENGINEGAME,OBSERVEGAME,ANALYZEGAME,ANALYZEPDN};
int CBstate=ENGINEMATCH,cbcolor=CB_BLACK,currentengine=1,enginethreadpriority=0,usersetpriority=0,abortcalculation=0,playnow=0;
int hwnd=0,tbwnd=0,hSearchThread=0;const char* CBdirectory="private";
bool gameover=false,reset_move_history=false,addcomment=false;
struct Options{bool use_incremental_time=false,handicap_enable=false,exact_time=false,userbook=false,early_game_adjudication=false,sound=false;double initial_time=0,time_increment=0,handicap_mult=1;int level=0;}cboptions;
struct Time{bool clock_paused=false;double black_time_remaining=0,white_time_remaining=0;clock_t starttime=0;}time_ctrl;
PDNgame cbgame;Board8x8 cbboard8;CBmove cbmove;int game_result=CB_UNKNOWN;char statusbar_txt[4096]{};
struct UB{pos position;CBmove move;};UB userbook[1];int userbooknum=0;
void SetThreadPriority(int,int){}void SetCurrentDirectory(const char*){}void PostMessage(int,int,WPARAM,int){}
void setenginebusy(bool){}void setenginestarting(bool){}void setanimationbusy(bool){}
void start_clock(){time_ctrl.starttime=clock();}
void cblog(const char*,...){}void PlaySound(const char*,void*,int){}void start_animation_thread(){}
void emlog_filename(char* s){std::strcpy(s,"private");}void enginename(char* s){std::strcpy(s,"fixture");}
void writefile(const char*,const char*,const char*,...){}
int gametype(){return 21;}double timelevel_to_time(int){return .5;}
void format_time_args(double,double,uint32_t*,uint32_t*){}double maxtime_for_incremental_tc(double x){return x;}
int get_movelist_from_engine(Board8x8,int,CBmove*,int*,int*){return 1;}
CBmove cb_of(cbt::Board b,cbt::Move m){
 int a[8][8];b.to_board8(a);CBmove x{};cbt::square_xy(m.from,x.from.x,x.from.y);cbt::square_xy(m.to,x.to.x,x.to.y);x.oldpiece=a[x.from.x][x.from.y];x.newpiece=x.oldpiece;
 x.jumps=m.njumps;int n=0;for(int sq=0;sq<32;++sq)if(m.captures&cbt::bit(sq)){cbt::square_xy(sq,x.del[n].x,x.del[n].y);x.delpiece[n]=a[x.del[n].x][x.del[n].y];++n;}return x;
}
int getmovelist(int c,CBmove* list,Board8x8 a,int* capture){auto b=cbt::Board::from_board8(a,c==CB_BLACK?cbt::Color::Black:cbt::Color::White);auto ms=b.legal_moves();*capture=!ms.empty()&&ms[0].is_capture();for(size_t i=0;i<ms.size();++i)list[i]=cb_of(b,ms[i]);return int(ms.size());}
void move4tonotation(CBmove m,char* s){std::sprintf(s,"%d-%d",cbt::xy_square(m.from.x,m.from.y)+1,cbt::xy_square(m.to.x,m.to.y)+1);}
void move_to_pdn_english(int,CBmove*,CBmove* m,char* s,int){move4tonotation(*m,s);}
void addmovetogame(CBmove m,const char*){gamebody_entry e{};e.move=m;cbgame.moves.push_back(e);cbgame.movesindex++;}
int returned=CB_UNKNOWN;
int fake_getmove(int a[8][8],int c,double,char* s,int*,uint32_t,uint32_t,CBmove*){auto b=cbt::Board::from_board8(a,c==CB_BLACK?cbt::Color::Black:cbt::Color::White);auto m=b.legal_moves()[0];b.after(m).to_board8(a);std::strcpy(s,"fixture result");return returned;}
auto getmove=fake_getmove;
enum PDN_RESULT{UNKNOWN_RES,WHITE_WIN_RES,BLACK_WIN_RES,DRAW_RES};
struct emstats_t{int unknowns=0,wins=0,losses=0,draws=0,blackwins=0,blacklosses=0;bool engine1_plays_black(int n){return n%2==1;}};
int maxmovecount=200;void emprogress_filename(char* s){std::strcpy(s,"private");}
"""
main=r"""
int main(){
 std::cout<<"early\treturn\t2piece_end\t2piece_PDN\t24piece_end\t24piece_PDN\n";
 for(bool early:{false,true})for(int r:{CB_DRAW,CB_WIN,CB_LOSS,CB_UNKNOWN}){
  std::cout<<early<<"\t"<<r;
  for(const char* fen:{"B:WK32:BK1",cbt::Board::start().fen().c_str()}){
   auto b=*cbt::Board::from_fen(fen);b.to_board8(cbboard8);cbgame=PDNgame{};std::strcpy(cbgame.FEN,fen);cboptions=Options{};cboptions.early_game_adjudication=early;time_ctrl=Time{};gameover=false;returned=r;currentengine=1;cbcolor=CB_BLACK;reset_move_history=true;
   SearchThreadFunc(nullptr);std::string result="not written";if(gameover){emstats_t st;update_match_stats(game_result,1,1,&st);result=cbgame.resultstring;}
   bool want=r==CB_DRAW||(early&&r!=CB_UNKNOWN);assert(gameover==want);assert(!want||result==(r==CB_DRAW?"1/2-1/2":r==CB_WIN?"1-0":"0-1"));std::cout<<"\t"<<gameover<<"\t"<<result;
  }std::cout<<"\n";
 }
}
"""
# Avoid the lifetime of a temporary c_str in the initializer list.
main=main.replace('for(const char* fen:{"B:WK32:BK1",cbt::Board::start().fen().c_str()})','for(const std::string fenstr:{std::string("B:WK32:BK1"),cbt::Board::start().fen()})').replace('auto b=*cbt::Board::from_fen(fen);','const char* fen=fenstr.c_str();auto b=*cbt::Board::from_fen(fen);')
main=main.replace(r'24piece_PDN\n',r'24piece_PDN\tWhite2piece_end\tWhite2piece_PDN\n')
main=main.replace('cbt::Board::start().fen()})','cbt::Board::start().fen(),std::string("W:WK32:BK1")})')
main=main.replace('cbcolor=CB_BLACK;reset_move_history=true;','cbcolor=b.stm==cbt::Color::Black?CB_BLACK:CB_WHITE;reset_move_history=true;')
main=main.replace('game_result,1,1,&st','game_result,1,cbcolor==CB_BLACK?1:2,&st')
main=main.replace('r==CB_WIN?"1-0":"0-1"','(r==CB_WIN)==(cbcolor==CB_BLACK)?"1-0":"0-1"')
(b/'reference_adapter.cpp').write_text(prefix+stub+'\n'+fn('DWORD SearchThreadFunc')+'\n'+fn('const char *pdn_result_to_string')+'\n'+fn('int update_match_stats')+'\n'+main)
