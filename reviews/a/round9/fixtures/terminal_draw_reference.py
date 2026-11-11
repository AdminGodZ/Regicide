from pathlib import Path
b=Path('reports/round9');s=(b/'reference_adapter.cpp').read_text();s=s[:s.rfind('int main(){')]+r'''
int main(){
 auto b=*cbt::Board::from_fen("B:W10:B6");b.to_board8(cbboard8);cbgame=PDNgame{};std::strcpy(cbgame.FEN,"B:W10:B6");cboptions=Options{};cboptions.early_game_adjudication=true;time_ctrl=Time{};gameover=false;returned=CB_DRAW;currentengine=1;cbcolor=CB_BLACK;reset_move_history=true;maxmovecount=200;
 SearchThreadFunc(nullptr);emstats_t st;if(gameover)update_match_stats(game_result,1,1,&st);
 std::cout<<"6x15 with CB_DRAW: gameover "<<gameover<<" PDN "<<cbgame.resultstring<<"\n";
 assert(gameover&&std::string(cbgame.resultstring)=="1/2-1/2");
}
'''
(b/'terminal_draw_reference.cpp').write_text(s)
