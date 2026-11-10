// No search. TSV stdin: id TAB B|W TAB dealt_plies TAB initial_reversible|-1 TAB FEN|- TAB moves
// Output: id, exposed|unexposed|unknown|error, first eligible changed ply (1-based),
// first effective probe-change ply, reason, FEN. Validate the WHOLE record first.
// Build/link with current regicide_core, regicide_egdb and tools/cbt_common.
#include <cstring>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "book.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "checkers.hpp"
using namespace regicide;
std::vector<std::string> split(const std::string&s,char c) {
    std::vector<std::string> v; std::stringstream f(s);
    for(std::string x;std::getline(f,x,c);)v.push_back(x);
    if(!s.empty()&&s.back()==c)v.emplace_back();return v;
}
bool equal(const std::vector<Book::Entry>&a,const std::vector<Book::Entry>&b) {
    return a.size()==b.size() && (a.empty()||!std::memcmp(a.data(),b.data(),a.size()*sizeof(a[0])));
}
int main(int argc,char**argv) {
    try {
        if(argc<3)throw std::runtime_error("exposure_helper OLD_BOOK NEW_BOOK [--egdb DIR] [--db-pieces N] [--book-off]");
        Book oldb,newb; if(!oldb.load(argv[1])||!newb.load(argv[2]))throw std::runtime_error("cannot load book");
        if(!equal(oldb.avoid_entries(),newb.avoid_entries()))throw std::runtime_error("avoid lists differ; separate root-avoid classification required");
        int dbpieces=8,no_progress=40,max_plies=1000; bool bookon=true,loaded=false;std::string played_book;
        for(int i=3;i<argc;++i) {
            std::string a=argv[i];
            if(a=="--book-off")bookon=false;
            else if(a=="--played-book"&&i+1<argc) {played_book=argv[++i];if(played_book!="old"&&played_book!="new")throw std::runtime_error("--played-book old|new");}
            else if(a=="--db-pieces"&&i+1<argc)dbpieces=std::stoi(argv[++i]);
            else if(a=="--no-progress"&&i+1<argc)no_progress=std::stoi(argv[++i]);
            else if(a=="--max-plies"&&i+1<argc)max_plies=std::stoi(argv[++i]);
            else if(a=="--egdb"&&i+1<argc) { loaded=egdb::init(argv[++i],16);if(!loaded)throw std::runtime_error("cannot load database"); }
            else throw std::runtime_error("bad argument "+a);
        }
        std::cout<<"id\tclassification\tfirst_ply\tfirst_probe_change_ply\tearliest_unknown_ply\treason\tfen\n";
        for(std::string line;std::getline(std::cin,line);) {
            std::string id="?";
            try {
                auto f=split(line,'\t'); if(f.size()!=6)throw std::runtime_error("expected six TSV fields");id=f[0];
                if(f[1]!="B"&&f[1]!="W")throw std::runtime_error("side must be B or W");
                const Color ours=f[1]=="B"?BLACK:WHITE;
                int dealt=std::stoi(f[2]),initial=std::stoi(f[3]);
                if(initial< -1||initial>65535||dealt<0)throw std::runtime_error("bad history metadata");
                Position p;if(f[4]!="-"&&!f[4].empty()&&!p.set_fen(f[4]))throw std::runtime_error("bad FEN");
                std::string err;auto ref=cbt::Board::from_fen(p.fen(),&err);if(!ref)throw std::runtime_error(err);
                p.set_reversible_plies(std::max(0,initial));
                std::vector<Position> states;std::vector<bool> known;std::vector<Move> played;
                std::vector<cbt::Board> window;int nonconv=0;bool ended=false;
                bool history_known=initial>=0;
                std::istringstream in(f[5]);
                for(std::string t;in>>t;) {
                    const int i=int(states.size());
                    if(i>=dealt) {
                        if(i==dealt)window.push_back(*ref);
                        if(ended||i-dealt>=max_plies)throw std::runtime_error("moves after runner draw/length termination");
                    }
                    Move m;auto rm=cbt::parse_move(*ref,t,&err);
                    if(!parse_move(p,t,m)||!rm)throw std::runtime_error("illegal/ambiguous move "+t+" "+err);
                    Bitboard caps=0;for(Bitboard q=m.captures;q;)caps|=Bitboard(1)<<(to_pdn(pop_lsb(q))-1);
                    if(to_pdn(m.from)!=rm->from+1||to_pdn(m.to)!=rm->to+1||caps!=rm->captures)throw std::runtime_error("move identity mismatch");
                    states.push_back(p);known.push_back(history_known);played.push_back(m);
                    if(m.is_capture()||!(p.kings()&bit(m.from)))history_known=true;
                    bool conversion=ref->is_conversion(*rm);
                    p.make_move(m);ref=ref->after(*rm);
                    if(i>=dealt) {
                        if(conversion){nonconv=0;window.assign(1,*ref);}
                        else {
                            ++nonconv;window.push_back(*ref);
                            ended=std::count(window.begin(),window.end(),*ref)>=3 || (no_progress>0&&nonconv>=2*no_progress-1);
                        }
                    }
                    auto check=cbt::Board::from_fen(p.fen());if(!check||!(*check==*ref))throw std::runtime_error("board mismatch");
                }
                if(dealt>int(states.size()))throw std::runtime_error("opening longer than game");
                int first=-1,effective=-1,unknown=-1;std::string fen,why;
                for(int i=dealt;bookon&&i<int(states.size());++i) {
                    const auto&q=states[i];
                    if(q.side_to_move()!=ours||q.reversible_plies()!=0||equal(oldb.lookup(q),newb.lookup(q)))continue;
                    if(!known[i]) {if(unknown<0)unknown=i+1;continue;}
                    if(dbpieces>0&&q.piece_count()<=dbpieces) {
                        if(!loaded||q.piece_count()>egdb::max_pieces()) {if(unknown<0)unknown=i+1;continue;}
                        auto v=egdb::probe(q.pieces(BLACK),q.pieces(WHITE),q.kings(),q.side_to_move());
                        if(v==egdb::Wld::Win||v==egdb::Wld::Loss)continue;
                    }
                    auto a=oldb.empty()?std::optional<Move>{}:oldb.probe(q),b=newb.empty()?std::optional<Move>{}:newb.probe(q);
                    auto actual=played_book=="old"?a:played_book=="new"?b:std::optional<Move>{};
                    if(actual&&*actual!=played[i])throw std::runtime_error("played move disagrees with "+played_book+" deterministic book at ply "+std::to_string(i+1));
                    if(first<0) {first=i+1;fen=q.fen();why="eligible changed entries";}
                    if(a!=b&&effective<0)effective=i+1;
                }
                std::cout<<id<<'\t'<<(first>=0?"exposed":unknown>=0?"unknown":"unexposed")<<'\t'<<first<<'\t'<<effective<<'\t'<<unknown
                         <<'\t'<<(first>=0?why:unknown>=0?"missing root history/database value":"no eligible changed root")<<'\t'<<fen<<'\n';
            }catch(const std::exception&e){std::cout<<id<<"\terror\t-1\t-1\t-1\t"<<e.what()<<"\t\n";}
        }
        return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}
}
