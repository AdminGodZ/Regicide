#include "master-src/src/cb_api.cpp"
#include "nnue.hpp"
#include "checkers.hpp"
#include <random>
#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <memory>
#include <numeric>
using namespace regicide;
namespace nn=regicide::nnue;
std::mt19937_64 rng(0xA9C0DE64);
long positions=0,edges=0,acc_checks=0,lanes=0,board_checks=0,path_checks=0,siblings=0;
long quiets=0,singlecap=0,multicap=0,promotions=0,kingmoves=0,circuits=0,capture_promotions=0,captured_kings=0;
long colors[2]{},buckets[24]{},changes[24][24]{};
[[noreturn]]void fail(const std::string& why,const Position&p,const Move&m={}){
 std::cerr<<"FAIL "<<why<<" root "<<p.fen()<<" move "<<move_to_string(m)<<"\n";std::exit(1);
}
std::string sig(const Move&m){return std::to_string(to_pdn(m.from))+","+std::to_string(to_pdn(m.to))+","+std::to_string([&]{std::uint32_t v=0;for(Bitboard b=m.captures;b;){int q=pop_lsb(b);v|=cbt::bit(to_pdn(q)-1);}return v;}());}
std::string sig(const cbt::Move&m){return std::to_string(m.from+1)+","+std::to_string(m.to+1)+","+std::to_string(m.captures);}
bool same(const Position&a,const Position&b){return a==b&&a.key()==b.key()&&a.reversible_plies()==b.reversible_plies();}
void checkacc(const nn::Network&net,nn::AccumulatorStack&stack,const Position&p,int ply){
 nn::Accumulator full;net.refresh(full,p);
 if(!net.both_views()){Position other;other.set(p.pieces(BLACK),p.pieces(WHITE),p.kings(),~p.side_to_move());stack.get(net,other,ply);}
 const auto&inc=stack.get(net,p,ply);
 for(Color c:{BLACK,WHITE}){
  for(int i=0;i<net.arch().ft;i++){++lanes;if(inc.v[c][i]!=full.v[c][i])fail("accumulator lane "+std::to_string(i),p);}
 }
 if(net.evaluate(inc,p.side_to_move(),p.piece_count())!=net.evaluate(full,p.side_to_move(),p.piece_count()))fail("incremental score",p);
 ++acc_checks;
}
void checkpath(const cbt::Board&b,const Move&m,const CBmove&out,const int orig[8][8],const Position&p){
 int fx,fy,tx,ty;cbt::square_xy(to_pdn(m.from)-1,fx,fy);cbt::square_xy(to_pdn(m.to)-1,tx,ty);
 if(out.from.x!=fx||out.from.y!=fy||out.to.x!=tx||out.to.y!=ty||out.oldpiece!=orig[fx][fy]||out.jumps!=popcount(m.captures))fail("CB endpoints/piece/jumps",p,m);
 std::map<int,int> want,got;
 for(Bitboard cap=m.captures;cap;){int q=pop_lsb(cap);int x,y;cbt::square_xy(to_pdn(q)-1,x,y);want[to_pdn(q)-1]=orig[x][y];}
 for(int i=0;i<out.jumps;i++){int q=cbt::xy_square(out.del[i].x,out.del[i].y);if(q<0||got.contains(q))fail("CB capture coordinate",p,m);got[q]=out.delpiece[i];}
 if(got!=want)fail("CB victims",p,m);
 if(out.path[0].x||out.path[0].y)fail("CB unused path[0]",p,m);
 for(int i=std::max(1,out.jumps);i<12;i++)if(out.path[i].x||out.path[i].y)fail("CB unused path",p,m);
 for(int i=out.jumps;i<12;i++)if(out.del[i].x||out.del[i].y||out.delpiece[i])fail("CB unused victim",p,m);
 std::vector<std::pair<int,int>> route{{fx,fy}};
 for(int i=1;i<out.jumps;i++)route.push_back({out.path[i].x,out.path[i].y});
 route.push_back({tx,ty});std::uint32_t caps=0;auto state=b;int cur=to_pdn(m.from)-1;
 const bool king=b.kings&cbt::bit(cur);auto own=b.stm;
 for(size_t i=1;i<route.size();i++){
  auto [x0,y0]=route[i-1];auto [x,y]=route[i];int dest=cbt::xy_square(x,y);
  int span=m.is_capture()?2:1;
  if(dest<0||std::abs(x-x0)!=span||std::abs(y-y0)!=span||(!king&&((own==cbt::Color::Black?y-y0:y0-y)!=span)))fail("CB path direction",p,m);
  auto& ours=own==cbt::Color::Black?state.black:state.white;
  auto& theirs=own==cbt::Color::Black?state.white:state.black;
  ours&=~cbt::bit(cur);state.kings&=~cbt::bit(cur);
  if(state.occupied()&cbt::bit(dest))fail("CB occupied landing",p,m);
  if(m.is_capture()){
   int over=cbt::xy_square((x+x0)/2,(y+y0)/2);
   if(over<0||!(theirs&cbt::bit(over)))fail("CB path missing victim",p,m);
   theirs&=~cbt::bit(over);state.kings&=~cbt::bit(over);caps|=cbt::bit(over);
  }
  ours|=cbt::bit(dest);if(king)state.kings|=cbt::bit(dest);
  bool crown=!king&&(own==cbt::Color::Black?y==7:y==0);
  if(crown&&i+1<route.size())fail("CB continues after promotion",p,m);
  if(crown)state.kings|=cbt::bit(dest);
  cur=dest;
 }
 std::uint32_t expected=0;for(auto [q,code]:want)expected|=cbt::bit(q);
 if(caps!=expected)fail("CB path captured set",p,m);++path_checks;
}
void node(const nn::Network&net,nn::AccumulatorStack&stack,const Position&p,const cbt::Board&b,int ply){
 ++positions;++colors[p.side_to_move()];++buckets[nn::bucket(p.piece_count(),net.arch().buckets)];checkacc(net,stack,p,ply);
 Board8x8 in{};b.to_board8(in);auto read=read_board(in,p.side_to_move()==WHITE?1:2);
 if(!(read==p)||read.key()!=p.key())fail("read_board",p);
 Board8x8 round{};
 for(Color c:{BLACK,WHITE})for(Bitboard bits=read.pieces(c);bits;){int q=pop_lsb(bits);auto xy=square_to_coors(q);round[xy.x][xy.y]=piece_code(c,read.kings()&bit(q));}
 if(std::memcmp(in,round,sizeof in))fail("board round trip",p);++board_checks;
 MoveList ms;generate_moves(p,ms);auto refs=b.legal_moves();std::map<std::string,cbt::Move> by;
 for(auto m:refs)by.emplace(sig(m),m);std::set<std::string> actual;
 for(auto m:ms)actual.insert(sig(m));
 std::set<std::string> expected;for(auto [k,v]:by)expected.insert(k);
 if(actual!=expected||actual.size()!=ms.size())fail("move set",p);
 bool forced=std::any_of(refs.begin(),refs.end(),[](auto m){return m.is_capture();});
 if(std::any_of(ms.begin(),ms.end(),[&](auto m){return m.is_capture()!=forced;}))fail("compulsory capture",p);
 for(auto m:ms){
  ++edges;auto rm=by.at(sig(m));auto ref=b.after(rm);Position child=p;auto old=p;auto afterkey=p.key_after(m);child.make_move(m);
  Position fresh;if(!fresh.set_fen(ref.fen()))fail("scratch FEN",p,m);
  fresh.set_reversible_plies(b.is_conversion(rm)?0:p.reversible_plies()+1);
  if(!same(child,fresh)||afterkey!=fresh.key()||!same(p,old))fail("make/key_after/copy",p,m);
  stack.push(ply+1,p,m);checkacc(net,stack,child,ply+1);++siblings;
  CBmove out;std::memset(&out,0xA5,sizeof out);Board8x8 moved;std::memcpy(moved,in,sizeof in);write_move(moved,p,m,&out);checkpath(b,m,out,in,p);
  auto result=cbt::Board::from_board8(moved,cbt::opposite(b.stm));
  if(!(result==ref))fail("write_move board",p,m);
  int tx,ty;cbt::square_xy(rm.to,tx,ty);Board8x8 ref8{};ref.to_board8(ref8);
  if(out.newpiece!=ref8[tx][ty]||std::memcmp(moved,ref8,sizeof moved))fail("CB promotion/board",p,m);
  const int ncap=popcount(m.captures);quiets+=ncap==0;singlecap+=ncap==1;multicap+=ncap>1;promotions+=b.promotes(rm);capture_promotions+=b.promotes(rm)&&ncap>0;captured_kings+=popcount(m.captures&p.kings());kingmoves+=(p.kings()&bit(m.from))!=0;circuits+=m.from==m.to;
  ++changes[nn::bucket(p.piece_count(),net.arch().buckets)][nn::bucket(child.piece_count(),net.arch().buckets)];
 }
 checkacc(net,stack,p,ply);
}
cbt::Board layout(int n){
 cbt::Board b;b.stm=rng()&1?cbt::Color::White:cbt::Color::Black;
 std::array<int,32>a;std::iota(a.begin(),a.end(),0);std::shuffle(a.begin(),a.end(),rng);
 int nb=std::uniform_int_distribution<int>(std::max(1,n-12),std::min(12,n-1))(rng);
 for(int i=0;i<n;i++){int q=a[i];bool black=i<nb;(black?b.black:b.white)|=cbt::bit(q);if((rng()&1)||(black?q>=28:q<4))b.kings|=cbt::bit(q);}
 return b;
}
int main(){
 auto ev=Evaluator::builtin();const auto&net=*ev->network();std::cout<<"seed 0xA9C0DE64 net "<<net.arch().describe()<<"\n";
 auto stack=std::make_unique<nn::AccumulatorStack>();
 for(int game=0;game<160;game++){
  cbt::Board b=cbt::Board::start();Position p;int ply=0;stack->reset(net,p);std::vector<cbt::Board> seen{b};int rev=0;
  for(int turn=0;turn<200;turn++){
   if(ply==96){ply=0;stack->reset(net,p);}
   node(net,*stack,p,b,ply);auto moves=b.legal_moves();if(moves.empty())break;auto rm=moves[rng()%moves.size()];Move m;
   if(!parse_move(p,cbt::move_to_pdn(rm),m))fail("reference path parse",p);
   auto old=p;stack->push(++ply,p,m);p.make_move(m);bool cv=b.is_conversion(rm);b=b.after(rm);
   if(cv){rev=0;seen={b};}else{++rev;seen.push_back(b);}
   if(rev>=79||std::count(seen.begin(),seen.end(),b)>=3){node(net,*stack,p,b,ply);break;}
  }
 }
 for(int i=0;i<4000;i++){auto b=layout(2+i%23);Position p;p.set_fen(b.fen());p.set_reversible_plies(rng()%79);stack->reset(net,p);node(net,*stack,p,b,0);}
 std::cout<<"positions "<<positions<<" edges "<<edges<<" accumulator checks "<<acc_checks<<" active lanes "<<lanes<<" board round trips "<<board_checks<<" CB move/path checks "<<path_checks<<" sibling writes "<<siblings<<"\n";
 std::cout<<"quiet "<<quiets<<" single captures "<<singlecap<<" multi captures "<<multicap<<" promotions "<<promotions<<" king moves "<<kingmoves<<" circuits "<<circuits<<" capture promotions "<<capture_promotions<<" captured kings "<<captured_kings<<" colors "<<colors[BLACK]<<"/"<<colors[WHITE]<<"\n";
 for(int i=0;i<net.arch().buckets;i++){std::cout<<"bucket "<<i<<" positions "<<buckets[i]<<" changes";for(int j=0;j<net.arch().buckets;j++)if(changes[i][j])std::cout<<" "<<i<<"->"<<j<<":"<<changes[i][j];std::cout<<"\n";}
}
