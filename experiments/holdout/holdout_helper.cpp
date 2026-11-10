// Replays with two independent rules implementations, evaluates statically, and checks bounded book contact.
// The Python builder passes 13 as the internal action budget so decision states
// at depths 0..12 are checked; a certificate may include the forced action 13.
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include "book.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "openings.hpp"
#include "search.hpp"
using namespace regicide;
struct BoardKey  {
    Bitboard b,w,k;
    unsigned char stm;
    bool operator==(const BoardKey&)const=default;
};
BoardKey key(const Position&p) {
    return {
        p.pieces(BLACK),p.pieces(WHITE),p.kings(),(unsigned char)p.side_to_move()
    };
}
struct KeyHash {
    size_t operator()(const BoardKey&a)const {
        std::uint64_t h=(std::uint64_t(a.b)<<32)|a.w;
        h^=std::uint64_t(a.k)*0x9e3779b97f4a7c15ULL;
        h^=a.stm;
        h^=h>>30;
        h*=0xbf58476d1ce4e5b9ULL;
        return h^(h>>27);
    }
};
struct State {
    BoardKey b;
    unsigned char rem,ours;
    bool reversible;
    bool operator==(const State&)const=default;
};
struct StateHash {
    size_t operator()(const State&s)const {
        return KeyHash{}(s.b)^(size_t(s.rem)*0x9e3779b9u)^(size_t(s.ours)<<22)^(size_t(s.reversible)<<29);
    }
};
struct Target  {
    Position p;
    Move move;
    int mb,mw,pb,pw;
};
std::string trim(std::string s) {
    auto a=s.find_first_not_of(" \t\r\n");
    if(a==std::string::npos)return {
    };
    return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);
}
std::vector<std::string> split(const std::string&s,char c) {
    std::vector<std::string>v;
    std::stringstream in(s);
    for(std::string x;std::getline(in,x,c);)v.push_back(x);
    return v;
}
void verify_board(const Position&p,const cbt::Board&r) {
    std::string err;
    auto b=cbt::Board::from_fen(p.fen(),&err);
    if(!b||!(*b==r))throw std::runtime_error("engine/referee board mismatch: "+p.fen());
}
std::vector<Position> replay(const std::string&text) {
    Position p;
    auto r=cbt::Board::start();
    std::vector<Position>path {
        p
    };
    std::istringstream in(text);
    for(std::string t;in>>t;) {
        Move m;
        std::string err;
        auto rm=cbt::parse_move(r,t,&err);
        if(!parse_move(p,t,m)||!rm)throw std::runtime_error("replay rejected "+t+": "+err);
        if(to_pdn(m.from)!=rm->from+1||to_pdn(m.to)!=rm->to+1)throw std::runtime_error("move identity mismatch");
        Bitboard pdncap=0;
        for(Bitboard b=m.captures;b;)pdncap|=Bitboard(1)<<(to_pdn(pop_lsb(b))-1);
        if(pdncap!=rm->captures)throw std::runtime_error("capture identity mismatch");
        p.make_move(m);
        r=r.after(*rm);
        verify_board(p,r);
        path.push_back(p);
    }
    if(r.legal_moves().empty())throw std::runtime_error("terminal opening");
    return path;
}
// Empty-board piece distances are a lower bound: capture chains may stop anywhere,
// ignore blocking pieces, and may reuse squares. Real moves are a subset of this graph.
int pdist[2][64][64];
bool use_pruning=true;
void init_distances() {
    for(Color c:{BLACK,WHITE}) {
        bool edges[64][64]= {
        };
        for(int from=0;from<64;++from) {
            bool king=from>=32;
            int sq=from%32;
            for(int d=dir_begin(c,king);d<dir_end(c,king);++d) {
                int to=NEIGHBOR[sq][d];
                if(to>=0) {
                    bool k=king||(bit(to)&promo_row(c));
                    edges[from][to+(k?32:0)]=true;
                }
            }
            bool seen[32]= {
            };
            std::vector<int>stack {
                sq
            };
            seen[sq]=true;
            while(!stack.empty()) {
                int cur=stack.back();
                stack.pop_back();
                for(int d=dir_begin(c,king);d<dir_end(c,king);++d) {
                    int over=NEIGHBOR[cur][d];
                    if(over<0)continue;
                    int to=NEIGHBOR[over][d];
                    if(to<0)continue;
                    bool k=king||(bit(to)&promo_row(c));
                    edges[from][to+(k?32:0)]=true;
                    if(!seen[to]&&!(k&&!king)) {
                        seen[to]=true;
                        stack.push_back(to);
                    }
                }
            }
        }
        for(int a=0;a<64;++a)for(int b=0;b<64;++b)pdist[c][a][b]=a==b?0:edges[a][b]?1:1000;
        for(int k=0;k<64;++k)for(int a=0;a<64;++a)for(int b=0;b<64;++b)pdist[c][a][b]=std::min(pdist[c][a][b],pdist[c][a][k]+pdist[c][k][b]);
    }
}
int move_lb(const Position&p,const Position&t,Color c) {
    int sum=0;
    for(int king=0;king<2;++king) {
        Bitboard tb=(king?t.kings(c):t.men(c))&~(king?p.kings(c):p.men(c));
        for(;tb;) {
            int sq=pop_lsb(tb),best=1000;
            for(Bitboard b=p.men(c);b;) {
                int from=pop_lsb(b);
                best=std::min(best,pdist[c][from][sq+32*king]);
            }
            for(Bitboard b=p.kings(c);b;) {
                int from=pop_lsb(b);
                best=std::min(best,pdist[c][32+from][sq+32*king]);
            }
            sum+=best;
        }
    }
    return std::max(sum,int(popcount(p.pieces(~c))>popcount(t.pieces(~c))));
}
int goal_rank(const Position&p,const std::vector<Target>&targets,const std::vector<int>&pot) {
    int best=1000000;
    for(int i:pot) {
        auto&t=targets[i].p;
        int v=64*(move_lb(p,t,BLACK)+move_lb(p,t,WHITE))+popcount(p.pieces(BLACK)^t.pieces(BLACK))+popcount(p.pieces(WHITE)^t.pieces(WHITE));
        best=std::min(best,v);
    }
    return best;
}
bool possible(const Position&p,const Target&t,int rem,Color ours) {
    if(!use_pruning)return rem>0;
    int d=rem-1;
    if((int(p.side_to_move())^(d&1))!=int(ours))--d;
    if(d<0)return false;
    int turns[2]= {
        (d+(p.side_to_move()==BLACK))/2,(d+(p.side_to_move()==WHITE))/2
    };
    if(move_lb(p,t.p,BLACK)>turns[BLACK]||move_lb(p,t.p,WHITE)>turns[WHITE])return false;
    for(Color c:{BLACK,WHITE}) {
        if(popcount(t.p.pieces(c))>popcount(p.pieces(c))||popcount(t.p.men(c))>popcount(p.men(c)))return false;
        if(popcount(t.p.pieces(c)&~p.pieces(c))>turns[c]||popcount(t.p.men(c)&~p.men(c))>turns[c]||popcount(t.p.kings(c)&~p.kings(c))>turns[c])return false;
        for(int r=0;r<8;++r) {
            Bitboard mask=c==BLACK?(~Bitboard(0)<<(4*r)):(r==7?~Bitboard(0):((Bitboard(1)<<(4*(r+1)))-1));
            if(popcount(t.p.men(c)&mask)>popcount(p.men(c)&mask))return false;
        }
    }
    return true;
}
enum Result {
    CLEAN,HIT,UNKNOWN
};
struct Reach {
    const Book&book;
    const std::vector<Target>&targets;
    Color ours;
    bool legal;
    long cap,nodes=0;
    std::unordered_set<State,StateHash>&clean;
    std::vector<Move>path,witness;
    int hit_target=-1;
    Result dfs(const Position&p,int rem,const std::vector<int>&potential) {
        if(rem<=0)return CLEAN;
        State s {
            key(p),(unsigned char)rem,(unsigned char)ours,p.reversible_plies()>0
        };
        if(clean.contains(s))return CLEAN;
        if(nodes>=cap)return UNKNOWN;
        ++nodes;
        std::vector<int>keep;
        for(int i:potential)if(possible(p,targets[i],rem,ours))keep.push_back(i);
        if(keep.empty()) {
            clean.insert(s);
            return CLEAN;
        }
        std::optional<Move>bm;
        if(p.side_to_move()==ours&&p.reversible_plies()==0)bm=book.probe(p);
        if(bm)for(int i:keep)if(key(p)==key(targets[i].p)&&*bm==targets[i].move) {
            witness=path;
            witness.push_back(*bm);
            hit_target=i;
            return HIT;
        }
        MoveList moves;
        if(p.side_to_move()==ours) {
            if(bm)moves.push(*bm);
            else if(!legal) {
                clean.insert(s);
                return CLEAN;
            }
            else  {
                generate_moves(p,moves);
                MoveList allowed;
                for(const Move& m:moves)if(!book.avoids(p,m))allowed.push(m);
                if(!allowed.empty()) {
                    moves.clear();
                    for(const Move& m:allowed)moves.push(m);
                }
            }
        }
        else generate_moves(p,moves);
        if(rem==1) {
            clean.insert(s);
            return CLEAN;
        }
        std::vector<std::pair<int,Move>>ranked;
        for(const Move&m:moves) {
            Position child=p;
            child.make_move(m);
            if(pdist[p.side_to_move()][m.from+((p.kings()&bit(m.from))?32:0)][m.to+((child.kings()&bit(m.to))?32:0)]>1)throw std::runtime_error("piece-distance bound excluded a legal move");
            ranked.push_back({use_pruning?goal_rank(child,targets,keep):0,m});
        }
        std::stable_sort(ranked.begin(),ranked.end(),[](const auto&a,const auto&b){return a.first<b.first;});
        bool unknown=false;
        for(auto&item:ranked) {
            const Move&m=item.second;
            Position child=p;
            child.make_move(m);
            path.push_back(m);
            auto z=dfs(child,rem-1,keep);
            path.pop_back();
            if(z==HIT)return HIT;
            if(z==UNKNOWN) {
                unknown=true;
                if(nodes>cap)return UNKNOWN;
            }
        }
        if(unknown)return UNKNOWN;
        clean.insert(s);
        return CLEAN;
    }
};
std::string witness_text(const Position&root,const std::vector<Move>&moves) {
    auto p=root;
    std::string s;
    for(auto&m:moves) {
        if(!s.empty())s+=' ';
        s+=move_text(p,m);
        p.make_move(m);
    }
    return s;
}
int main(int argc,char**argv) {
    try {
        init_distances();
        std::map<std::string,std::string>arg;
        for(int i=1;i+1<argc;i+=2)arg[argv[i]]=argv[i+1];
        if(arg.contains("--prune"))use_pruning=arg["--prune"]!="0";
        if(arg.contains("--validate")) {
            std::string err;
            auto rows=match::load_openings(arg["--validate"],err);
            if(!err.empty())throw std::runtime_error(err);
            std::set<std::string>seen;
            for(auto&o:rows) {
                auto path=replay(o.moves);
                if(!o.fen.empty())throw std::runtime_error("validator expects move-only panels");
                if(!seen.insert(path.back().fen()).second)throw std::runtime_error("duplicate final board");
                verify_board(path.back(),o.start);
            }
            std::cout<<rows.size()<<" openings passed both parsers and unique-final-board check\n";
            return 0;
        }
        Book book;
        if(!book.load(arg["--book"]))throw std::runtime_error("cannot load candidate book");
        std::vector<Target>targets;
        std::set<std::string>all_decisions;
        std::ifstream df(arg["--decisions"]);
        for(std::string l;std::getline(df,l);) {
            if(trim(l).empty()||trim(l)[0]=='#')continue;
            auto f=split(l,'|');
            if(f.size()<4)throw std::runtime_error("bad decision row");
            Position p;
            Move m;
            if(!p.set_fen(trim(f[0]))||!parse_move(p,trim(f[1]),m))throw std::runtime_error("bad decision");
            all_decisions.insert(p.fen());
            if(trim(f[2])=="force") {
                auto bm=book.probe(p);
                if(!bm||!(*bm==m))throw std::runtime_error("book/decision mismatch");
                targets.push_back({p,m,popcount(p.men(BLACK)),popcount(p.men(WHITE)),popcount(p.pieces(BLACK)),popcount(p.pieces(WHITE))});
            }
        }
        // This mode replays contact certificates; it performs no tree enumeration or search.
        if(arg.contains("--witness-input")) {
            std::ifstream wi(arg["--witness-input"]);
            std::ofstream wo(arg["--out"]);
            wo<<"id\tcolour\tplies_after_opening\tfirst_own_book_miss\tdecision_fen\tforced_move\n";
            for(std::string line;std::getline(wi,line);) {
                auto f=split(line,'\t');
                if(f.size()!=4)throw std::runtime_error("bad witness query");
                auto history=replay(f[1]);
                auto p=history.back();
                std::string err;
                auto rb=cbt::Board::from_fen(p.fen(),&err);
                if(!rb)throw std::runtime_error(err);
                Color ours=f[2]=="B"?BLACK:WHITE;
                int n=0,miss=0;
                std::string last_fen,last_move;
                bool final_force=false;
                std::istringstream tail(f[3]);
                for(std::string t;tail>>t;) {
                    ++n;
                    if(n>13)throw std::runtime_error("witness exceeds inclusive twelve-ply position horizon");
                    int occurrences=0;
                    for(auto&h:history)occurrences+=key(h)==key(p);
                    if(occurrences>=3||p.reversible_plies()>=NO_PROGRESS_PLIES)throw std::runtime_error("witness continues after a draw");
                    Move m;
                    if(!parse_move(p,t,m))throw std::runtime_error("bad witness engine move");
                    auto rm=cbt::parse_move(*rb,t,&err);
                    if(!rm)throw std::runtime_error("bad witness referee move: "+err);
                    std::optional<Move>bm;
                    if(p.side_to_move()==ours&&p.reversible_plies()==0)bm=book.probe(p);
                    if(p.side_to_move()==ours) {
                        if(bm&&!(m==*bm))throw std::runtime_error("witness contradicts own book");
                        if(!bm) {
                            if(!miss)miss=n;
                            MoveList lm;
                            generate_moves(p,lm);
                            bool unavoided=false;
                            for(auto&a:lm)unavoided|=!book.avoids(p,a);
                            if(unavoided&&book.avoids(p,m))throw std::runtime_error("witness uses avoided own move");
                        }
                    }
                    final_force=false;
                    if(bm)for(auto&tgt:targets)if(key(tgt.p)==key(p)&&tgt.move==m) {
                        final_force=true;
                        last_fen=p.fen();
                        last_move=move_text(p,m);
                        break;
                    }
                    p.make_move(m);
                    *rb=rb->after(*rm);
                    verify_board(p,*rb);
                    history.push_back(p);
                }
                if(!n||!final_force)throw std::runtime_error("witness does not end with a forced decision move");
                wo<<f[0]<<'\t'<<f[2]<<'\t'<<n<<'\t'<<miss<<'\t'<<last_fen<<'\t'<<last_move<<'\n';
            }
            return 0;
        }
        int bound=std::stoi(arg["--plies"]);
        long cap=std::stol(arg["--cap"]);
        bool legal=arg["--miss-policy"]=="legal";
        bool doc=arg["--do-c"]!="0";
        std::unordered_set<State,StateHash>clean;
        std::ifstream in(arg["--input"]);
        std::ofstream out(arg["--out"]);
        out<<"id\tplies\tfinal_fen\teval_stm\teval_black\trule_b\tc_black\tc_white\tnodes_black\tnodes_white\twitness_black\twitness_white\n";
        auto ev=Evaluator::builtin();
        for(std::string l;std::getline(in,l);) {
            auto tab=l.find('\t');
            if(tab==std::string::npos)continue;
            auto id=l.substr(0,tab);
            auto path=replay(l.substr(tab+1));
            auto p=path.back();
            bool b=false;
            for(auto&q:path)b|=all_decisions.contains(q.fen());
            std::string states[2]= {
                "not_run","not_run"
            }
            ,wit[2];
            long n[2]= {
                0,0
            };
            if(doc)for(Color c:{BLACK,WHITE}) {
            if(arg.contains("--colour") && arg["--colour"] != (c==BLACK?"B":"W"))continue;
                std::vector<int>pot;
                for(int j=0;j<int(targets.size());++j)if(targets[j].p.side_to_move()==c)pot.push_back(j);
                Reach reach {
                    book,targets,c,legal,cap,0,clean
                };
                auto z=reach.dfs(p,bound,pot);
                states[c]=z==HIT?"touched":z==CLEAN?"clean":"unknown";
                n[c]=reach.nodes;
                if(z==HIT)wit[c]=witness_text(p,reach.witness);
            }
            int score=(*ev)(p);
            out<<id<<'\t'<<path.size()-1<<'\t'<<p.fen()<<'\t'<<score<<'\t'<<(p.side_to_move()==BLACK?score:-score)<<'\t'<<b<<'\t'<<states[0]<<'\t'<<states[1]<<'\t'<<n[0]<<'\t'<<n[1]<<'\t'<<wit[0]<<'\t'<<wit[1]<<'\n';
        }
        return 0;
    }
    catch(const std::exception&e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
