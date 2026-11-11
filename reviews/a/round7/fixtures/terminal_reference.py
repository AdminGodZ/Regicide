from pathlib import Path
b=Path('reports/round7')
s=(b/'reference_adapter.cpp').read_text()
s=s[:s.rfind('int main(){')]+r'''
int main(){
 const char* f="B:W10:B6";auto b=*cbt::Board::from_fen(f);b.to_board8(cbboard8);cbgame=PDNgame{};std::strcpy(cbgame.FEN,f);cboptions=Options{};cboptions.early_game_adjudication=true;time_ctrl=Time{};gameover=false;returned=CB_WIN;currentengine=1;cbcolor=CB_BLACK;reset_move_history=true;maxmovecount=1;
 SearchThreadFunc(nullptr);emstats_t st;if(gameover)update_match_stats(game_result,1,1,&st);
 std::cout<<"terminal 6x15, win return, maxmovecount 1: gameover "<<gameover<<" PDN "<<cbgame.resultstring<<"\n";assert(gameover&&std::string(cbgame.resultstring)=="1-0");
}
'''
(b/'terminal_reference.cpp').write_text(s)
