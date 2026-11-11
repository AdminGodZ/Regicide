#include "book.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "egdb/egdb.hpp"
#include "checkers.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <map>
#include <set>
#include <deque>
#include <unordered_map>
#include <unordered_set>
using namespace regicide;
using Sig=std::tuple<int,int,Bitboard>;
Sig sig(Move m){return {m.from,m.to,m.captures};}
std::vector<std::string> split(std::string s,char c='\t'){std::vector<std::string>f;std::stringstream ss(s);for(std::string x;std::getline(ss,x,c);)f.push_back(x);return f;}
std::string trim(std::string s){auto a=s.find_first_not_of(" \r\n");return a==s.npos?"":s.substr(a,s.find_last_not_of(" \r\n")-a+1);}
std::vector<Book::Entry> entries(std::string path){std::ifstream f(path,std::ios::binary);unsigned h[4];f.read((char*)h,16);if(h[1]==1)f.seekg(12);std::vector<Book::Entry>e(h[2]);f.read((char*)e.data(),e.size()*16);return e;}
int main(){
 const std::string base="reports/round10/", lab=base+"booklab-src/experiments/booklab/";
 auto book=std::make_shared<Book>();Book old;if(!book->load(lab+"wp11f/book_r11f.bin")||!old.load(lab+"wp11d/book_r11d.bin"))return 2;
 auto es=entries(lab+"wp11f/book_r11f.bin"),os=entries(lab+"wp11d/book_r11d.bin");
 std::unordered_map<Key,Position>positions;std::deque<Position>q;std::map<Key,std::pair<Position,std::string>>pins;
 auto add=[&](Position p){if(positions.emplace(p.key(),p).second)q.push_back(p);};
 for(auto name:{"wp11f/decisions.txt","wp11f/pins_r11f_new.txt"}){
  std::ifstream f(lab+name);for(std::string l;std::getline(f,l);){if(l.empty()||l[0]=='#')continue;auto x=split(l,'|');Position p;if(x.size()<3||!p.set_fen(trim(x[0])))return 3;add(p);if(std::string(name).find("pins")!=std::string::npos)pins[p.key()]={p,trim(x[1])};}
 }
 Position changed;if(!changed.set_fen("B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14"))return 4;
 std::map<Key,std::pair<Position,std::string>>targets=pins;targets[changed.key()]={changed,"8-11"};
 std::vector<std::pair<std::string,Position>>ballots;std::ifstream bf(base+"ballots.tsv");
 for(std::string l;std::getline(bf,l);){auto f=split(l);Position p;std::stringstream ss(f[1]);for(std::string t;ss>>t;){Move m;if(!parse_move(p,t,m))return 5;p.make_move(m);}ballots.push_back({f[0],p});add(p);}add(Position{});
 std::map<Key,std::map<Sig,Move>>oppSeen[2];std::set<Key>oldLine,other;std::map<Key,std::array<int,3>>provenance;
 std::ifstream rf(base+"records.tsv");long games=0,plies=0;std::ofstream cand(base+"draw-candidates.tsv");std::unordered_set<Key>candseen;
 if(!egdb::init("/home/admingod/regicide-data/egdb",16))return 6;
 for(std::string l;std::getline(rf,l);){
  auto f=split(l);if(f.size()<6)return 7;Color ours=f[2]=="B"?BLACK:WHITE;Position p;if(!f[4].empty()&&!p.set_fen(f[4]))return 8;
  cbt::Board cb=*cbt::Board::from_fen(p.fen());std::vector<Position>hist;std::vector<std::string>tokens;std::stringstream ss(f[5]);for(std::string t;ss>>t;)tokens.push_back(t);
  std::set<int>searched;if(f.size()>6)for(auto x:split(f[6],','))if(!x.empty())searched.insert(std::stoi(x));
  bool through=false;for(int i=0;i<(int)tokens.size();i++){Move m;if(!parse_move(p,tokens[i],m)){std::cerr<<"ERR "<<f[1]<<":"<<i+1<<" "<<tokens[i]<<"\n";return 9;}
   auto rm=cbt::parse_move(cb,tokens[i]);if(!rm)return 10;
   add(p);if(p.side_to_move()!=ours)oppSeen[ours][p.key()][sig(m)]=m;
   if(i==16&&p==changed)through=true;
   if(ours==BLACK&&f[4].empty()&&p.piece_count()>8&&p.reversible_plies()==0&&old.probe(p)){
    if(through&&tokens[16]=="6-10"&&i>16)oldLine.insert(p.key());
    else if(!(through&&i>16))other.insert(p.key());
   }
   if((f[1]=="w14c6-kingsrow-fresh"||f[1]=="w14c6-kingsrow-kept")&&targets.contains(p.key())){
    auto [tp,tm]=targets[p.key()];Move wanted;if(!parse_move(tp,tm,wanted))return 11;
    if(m==wanted){bool draw=f[3]=="1/2-1/2";bool win=f[3]==(ours==BLACK?"1-0":"0-1");provenance[p.key()][draw?1:win?0:2]++;}
   }
   if(p.piece_count()<=8&&p.side_to_move()==ours&&searched.contains(i+1)&&!candseen.contains(p.key())){
    auto w=egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move());
    if(w==egdb::Wld::Draw){candseen.insert(p.key());std::string prefix;for(int j=0;j<i;j++)prefix+=(j?" ":"")+tokens[j];cand<<f[0]<<":"<<i+1<<"\t"<<prefix<<"\t"<<p.fen()<<"\t1000000\n";}
   }
   hist.push_back(p);p.make_move(m);cb=cb.after(*rm);if(p.fen()!=cb.fen())return 12;++plies;
  }++games;
 }
 // Recover FENs also for base-book branches absent from records: all legal one-ply children of every known book position.
 while(!q.empty()){Position p=q.front();q.pop_front();if(book->lookup(p).empty()&&old.lookup(p).empty())continue;MoveList ms;generate_moves(p,ms);for(auto m:ms){Position c=p;c.make_move(m);if(!book->lookup(c).empty()||!old.lookup(c).empty())add(c);}}
 std::ifstream af(base+"avoid-fens.txt");for(std::string l;std::getline(af,l);){Position p;if(p.set_fen(l))add(p);}long missing=0,bad=0,forced=0,multi=0;std::map<Key,int>nf;std::ofstream legal(base+"tables/book-legality.tsv");legal<<"kind\tkey\tFEN\tmove\tlegal\n";
 auto check=[&](const Book::Entry&e,const char*kind){auto it=positions.find(e.key);if(it==positions.end()){missing++;legal<<kind<<'\t'<<e.key<<"\tUNKNOWN\t-\t0\n";return;}auto m=Book::to_move(it->second,e);bool ok=bool(m);if(m){auto cb=*cbt::Board::from_fen(it->second.fen());auto rm=cbt::parse_move(cb,move_to_string(*m));std::uint32_t caps=0;for(Bitboard bb=m->captures;bb;){int sq=pop_lsb(bb);caps|=cbt::bit(to_pdn(sq)-1);}ok=bool(rm)&&rm->captures==caps;}if(!ok){bad++;legal<<kind<<'\t'<<e.key<<'\t'<<it->second.fen()<<"\t-\t0\n";}if(e.depth==255&&e.score==32767){forced++;nf[e.key]++;}};
 for(auto&e:es)check(e,"entry");for(auto&e:book->avoid_entries())check(e,"avoid");for(auto[k,n]:nf)multi+=n>1;
 std::map<Key,std::set<Sig>>forces;std::ifstream df(lab+"wp11f/decisions.txt");for(std::string l;std::getline(df,l);){if(l.empty()||l[0]=='#')continue;auto f=split(l,'|');if(trim(f[2]).find("force")!=0)continue;Position p;p.set_fen(trim(f[0]));Move m;if(!parse_move(p,trim(f[1]),m))return 14;forces[p.key()].insert(sig(m));auto lookup=book->lookup(p);if(lookup.size()!=1||Book::to_move(p,lookup[0])!=m)return 15;}forced=forces.size();for(auto [k,v]:forces)multi+=v.size()!=1;
 using EKey=std::tuple<Key,int,int,int>;std::map<EKey,Book::Entry>before,after;for(auto e:os)before[{e.key,e.from,e.to,e.disc}]=e;for(auto e:es)after[{e.key,e.from,e.to,e.disc}]=e;
 std::ofstream diff(base+"tables/book-diff.tsv");diff<<"change\tFEN\tmove\tscore\tdepth\tcount\n";int added=0,deleted=0,modified=0;
 auto out=[&](const char*k,Book::Entry e){auto p=positions.at(e.key);auto m=Book::to_move(p,e);diff<<k<<'\t'<<p.fen()<<'\t'<<(m?move_to_string(*m):"-")<<'\t'<<e.score<<'\t'<<int(e.depth)<<'\t'<<e.count<<'\n';};
 for(auto[k,e]:after){if(!before.contains(k)){++added;out("added",e);}else if(std::memcmp(&e,&before.at(k),16)){++modified;out("modified",e);}}
 for(auto[k,e]:before)if(!after.contains(k)){++deleted;out("removed",e);}
 std::ofstream pv(base+"tables/pin-provenance.tsv");pv<<"FEN\tmove\tsource_W\tsource_D\tsource_L\tengine_move\tdepth\tnodes\tscore\n";
 auto eng=std::make_unique<Engine>();eng->set_threads(1);eng->set_hash_mb(1);eng->set_book(book,0);eng->set_avoid(book);int samplebad=0;
 for(auto[k,t]:targets){eng->new_game();eng->set_position(t.first);Limits lim;lim.depth=1;auto r=eng->go(lim);Move m;if(!parse_move(t.first,t.second,m))return 13;samplebad+=r.best!=m||r.depth!=0||r.nodes!=0;auto v=provenance[k];pv<<t.first.fen()<<'\t'<<t.second<<'\t'<<v[0]<<'\t'<<v[1]<<'\t'<<v[2]<<'\t'<<move_to_string(r.best)<<'\t'<<r.depth<<'\t'<<r.nodes<<'\t'<<r.score<<'\n';}
 // Complete graph: our move is mode-3 book choice; every legal opponent move, or only observed opponent edges.
 std::map<Key,std::set<std::string>>reach[3],anyturn[3];long visits[3]{};
 for(int arm=0;arm<3;arm++)for(Color ours:{BLACK,WHITE})for(auto [name,start]:ballots){
  const Book&bk=arm==0?old:*book;std::vector<Position>todo{start};std::set<std::pair<Key,int>>seen;
  while(!todo.empty()){
   Position p=todo.back();todo.pop_back();if(p.piece_count()<=8)continue;if(targets.contains(p.key())||!old.lookup(p).empty())anyturn[arm][p.key()].insert(name+"/"+(ours==BLACK?"B":"W"));bool own=p.side_to_move()==ours;if(own&&p.reversible_plies()!=0)continue;
   if(!seen.insert({p.key(),p.reversible_plies()}).second)continue;++visits[arm];
   if(own){auto m=bk.probe(p,0,0,&bk);if(!m)continue;reach[arm][p.key()].insert(name+"/"+(ours==BLACK?"B":"W"));Position c=p;c.make_move(*m);todo.push_back(c);}
   else{MoveList ms;generate_moves(p,ms);for(auto m:ms){if(arm==2&&!oppSeen[ours][p.key()].contains(sig(m)))continue;Position c=p;c.make_move(m);if(!bk.lookup(c).empty())todo.push_back(c);}}
  }
 }
 std::ofstream reachout(base+"tables/target-reach.tsv");reachout<<"FEN\tmove\tall_legal_opponents\tobserved_opponents\tany_turn_all_legal\tany_turn_observed\n";
 auto joined=[](const std::set<std::string>&s){std::string r;for(auto x:s)r+=(r.empty()?"":";")+x;return r;};
 for(auto[k,t]:targets)reachout<<t.first.fen()<<'\t'<<t.second<<'\t'<<joined(reach[1][k])<<'\t'<<joined(reach[2][k])<<'\t'<<joined(anyturn[1][k])<<'\t'<<joined(anyturn[2][k])<<'\n';
 std::ofstream un(base+"tables/old-line-reach.tsv");un<<"FEN\trecorded_old_only\tr11d_ballots\tr11f_ballots\tobserved_opponents\tany_turn_all_legal\tany_turn_observed\n";int oldonly=0,still=0;
 for(auto k:oldLine)if(!other.contains(k)){oldonly++;still+=!reach[1][k].empty();un<<positions.at(k).fen()<<"\t1\t"<<joined(reach[0][k])<<'\t'<<joined(reach[1][k])<<'\t'<<joined(reach[2][k])<<'\t'<<joined(anyturn[1][k])<<'\t'<<joined(anyturn[2][k])<<'\n';}
 std::ofstream lost(base+"tables/all-unreachable.tsv");lost<<"FEN\tr11d_origins\n";int lostkeys=0;for(auto [k,v]:reach[0])if(!v.empty()&&reach[1][k].empty()){lostkeys++;lost<<positions.at(k).fen()<<'\t'<<joined(v)<<'\n';}std::cout<<"all previously reachable now unreachable "<<lostkeys<<"\n";
 std::cout<<"records "<<games<<" plies "<<plies<<" distinct FENs "<<positions.size()<<" drawn searched candidates "<<candseen.size()<<"\nentries "<<es.size()<<" avoids "<<book->avoid_size()<<" missing FEN "<<missing<<" illegal "<<bad<<" forced "<<forced<<" multi forced "<<multi<<"\ndiff added "<<added<<" removed "<<deleted<<" modified "<<modified<<" avoid identical "<<(old.avoid_size()==book->avoid_size()&&std::equal(old.avoid_entries().begin(),old.avoid_entries().end(),book->avoid_entries().begin(),[](auto a,auto b){return !std::memcmp(&a,&b,16);}))<<"\nengine samples "<<targets.size()<<" bad "<<samplebad<<"\nold recorded keys "<<oldLine.size()<<" other "<<other.size()<<" old-only "<<oldonly<<" reachable r11f "<<still<<"\nreach visits "<<visits[0]<<","<<visits[1]<<","<<visits[2]<<"\n";
 return missing||bad||multi||samplebad;
}
