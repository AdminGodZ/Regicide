#include "master-src/src/search.cpp"
#include <random>
#include <iostream>
#include <set>
using namespace regicide;
std::mt19937_64 rng(0xA9D157);
[[noreturn]] void fail(const char*s){std::cerr<<"FAIL "<<s<<"\n";std::exit(1);}
int main(){
 TranspositionTable t;t.resize(1);long distance=0,roundtrips=0,prefs=0;
 // The helpers are the actual ones compiled from search.cpp, not a second implementation.
 for(int sign:{-1,1})for(int base:{SCORE_WIN,SCORE_DB_WIN})for(int d:{1,7,63,127,1000}){
  if(base==SCORE_WIN&&d>=1000)continue;
  for(int p=0;p<128;p+=7)for(int q=0;q<128;q+=5){
   int s=sign*(base-d-p);int stored=score_to_tt(s,p);Key k=rng();t.store(k,stored,17,12,BOUND_EXACT,{});
   TTData out;if(!t.probe(k,out)||score_from_tt(out.score,q)!=sign*(base-d-q))fail("distance conversion");++distance;
  }
 }
 for(int i=0;i<20000;i++){
  Key k=rng();int score=int(rng()%60001)-30000,eval=int(rng()%29999)-14999,depth=int(rng()%136)-8;
  Bound bound=Bound(1+rng()%3);TTMove m{std::uint16_t(rng())};t.store(k,score,eval,depth,bound,m);TTData d;
  if(!t.probe(k,d)||d.score!=score||d.eval!=eval||d.depth!=depth||d.bound!=bound||d.move.v!=m.v)fail("pack round trip");++roundtrips;
 }
 for(int deep=3;deep<=120;deep+=3)for(int diff:{1,2,3,9})for(Bound b:{BOUND_UPPER,BOUND_LOWER,BOUND_EXACT}){
  t.clear();Key k=1;t.store(k,111,7,deep,BOUND_EXACT,{17});t.store(k,222,8,deep-diff,b,{});
  TTData d;if(!t.probe(k,d))fail("same key missing");
  bool keep=b!=BOUND_EXACT&&diff>2;
  if(d.score!=(keep?111:222)||d.move.v!=17||d.depth!=(keep?deep:deep-diff)||d.bound!=(keep?BOUND_EXACT:b))fail("depth/exact preference");
  ++prefs;t.new_search();t.store(k,333,9,-8,BOUND_UPPER,{});if(!t.probe(k,d)||d.score!=333)fail("old generation replaced");++prefs;
 }
 t.resize(0); // one four-entry bucket, so replacement is completely observed.
 for(int k=1;k<=4;k++)t.store(k,10*k,0,4*k,BOUND_EXACT,{});
 t.store(5,50,0,20,BOUND_EXACT,{});TTData d;if(t.probe(1,d)||!t.probe(5,d))fail("shallow victim");
 t.clear();for(int k=1;k<=4;k++)t.store(k,10*k,0,12,BOUND_EXACT,{});
 t.new_search();for(int k=1;k<=3;k++)t.store(k,10*k,0,5,BOUND_EXACT,{});
 t.store(5,50,0,1,BOUND_EXACT,{});if(t.probe(4,d)||!t.probe(5,d))fail("age victim");

 long cutoff_cases=0;
 {
  Engine engine;engine.set_hash_mb(1);auto worker=std::make_unique<Worker>(engine,0);TimeControl tc;tc.start=Clock::now();tc.node_limit=20000;
  Position parent;MoveList ml;generate_moves(parent,ml);Move first=ml[0];Position child=parent;child.make_move(first);
  for(auto [bound,score]:{std::pair{BOUND_EXACT,333},std::pair{BOUND_LOWER,333},std::pair{BOUND_UPPER,-333}}){
   worker->tt.clear();worker->setup(parent,{},std::vector<Move>(ml.begin(),ml.end()));worker->tc=&tc;worker->keys[0]=parent.key();worker->prev_to[0]=32;worker->prev_to[1]=first.to;worker->nn.push(1,parent,first);
   worker->tt.store(child.key(),score,0,8,bound,{});Position at=child;
   int got=worker->search(at,0,1,2,1,false,1);if(got!=score||worker->nodes!=1)fail("bound cutoff");
   ++cutoff_cases;
  }
  worker->tt.clear();worker->setup(parent,{},std::vector<Move>(ml.begin(),ml.end()));worker->tc=&tc;worker->keys[0]=parent.key();worker->prev_to[0]=32;worker->prev_to[1]=first.to;worker->nn.push(1,parent,first);
  worker->tt.store(child.key(),333,0,8,BOUND_EXACT,{});Position at=child;
  worker->search(at,-50,50,2,1,false,1);if(worker->nodes<=1)fail("PV should not take TT cutoff");++cutoff_cases;
 }
 auto e=std::make_unique<Engine>();e->set_hash_mb(1);e->set_egdb_pieces(0);
 auto& hash=const_cast<TranspositionTable&>(e->hash());Position p;long searches=0,poisons=0,pvplies=0,lookups=0;
 for(int i=0;i<96;i++){
  MoveList legal;generate_moves(p,legal);if(legal.empty()){p=Position();generate_moves(p,legal);}
  e->new_game();e->set_position(p);
  auto poison=[&](const Position& at){
   MoveList ms;generate_moves(at,ms);int empty=0;while(empty<32&&(at.occupied()&bit(empty)))++empty;
   if(empty==32)return;TTMove bad=TTMove::make(empty,(empty+4)%32,0);if(find_move(ms,bad)!=-1)fail("bad fixture move");++lookups;
   hash.store(at.key()^rev_key(at.reversible_plies(),false),0,0,5,BOUND_UPPER,bad);++poisons;
  };
  poison(p);for(auto m:legal){Position c=p;c.make_move(m);poison(c);}
  Limits l;l.depth=3;l.nodes=2000;l.use_book=false;auto r=e->go(l);++searches;
  if(std::find(legal.begin(),legal.end(),r.best)==legal.end())fail("illegal returned move");
  Position cur=p;
  for(auto m:r.pv){MoveList ms;generate_moves(cur,ms);if(std::find(ms.begin(),ms.end(),m)==ms.end())fail("illegal PV move");cur.make_move(m);++pvplies;}
  p.make_move(legal[rng()%legal.size()]);
 }
 std::cout<<"seed 0xA9D157 distance cases "<<distance<<" field roundtrips "<<roundtrips<<" same-key/generation preference cases "<<prefs<<" collision replacement cases 2 cutoff-window cases "<<cutoff_cases<<"\n";
 std::cout<<"poisoned full searches "<<searches<<" illegal references "<<poisons<<" rejected move lookups "<<lookups<<" legal PV plies "<<pvplies<<" all returned/PV moves legal\n";
}
