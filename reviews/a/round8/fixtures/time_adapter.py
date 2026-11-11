from pathlib import Path
b=Path('reports/round8');s=(b/'reference_adapter.cpp').read_text();s=s[:s.rfind('int main(){')]
s=s.replace('using DWORD=unsigned long;','clock_t fixture_ticks=0;int fixture_call=0;\nusing DWORD=unsigned long;')
s=s.replace('void start_clock(){time_ctrl.starttime=clock();}','void start_clock(){time_ctrl.starttime=fixture_ticks;}')
a=s.index('int fake_getmove(');z=s.index('\n',a)
s=s[:a]+r'''int fake_getmove(int a[8][8],int c,double mt,char* status,int*,uint32_t info,uint32_t more,CBmove*){std::cout<<cboptions.exact_time<<"\t"<<++fixture_call<<"\t"<<(c==CB_BLACK?"B":"W")<<"\t"<<std::setprecision(8)<<mt<<"\t"<<info<<"\t"<<more<<"\n";auto b=cbt::Board::from_board8(a,c==CB_BLACK?cbt::Color::Black:cbt::Color::White);auto m=b.legal_moves()[0];b.after(m).to_board8(a);std::strcpy(status,"fixture");fixture_ticks+=CLOCKS_PER_SEC/10;return CB_UNKNOWN;}'''+s[z:]
at=s.index('DWORD SearchThreadFunc')
s=s[:at]+'clock_t review_clock(){return fixture_ticks;}\n#define clock review_clock\n'+s[at:]
s+=r'''
int main(){
 std::cout<<"GUI_exact_option\tglobal_call\tmover\tmaxtime\tinfo\tmoreinfo\n";
 for(bool exact:{false,true}){
  fixture_ticks=0;fixture_call=0;cboptions=Options{};cboptions.exact_time=exact;time_ctrl=Time{};cbgame=PDNgame{};gameover=false;
  auto b=cbt::Board::start();
  for(const char* tok:{"9-13","21-17","5-9"}){auto m=*cbt::parse_move(b,tok);addmovetogame(cb_of(b,m),tok);b=b.after(m);}
  b.to_board8(cbboard8);cbcolor=CB_WHITE;
  for(int n=1;n<=6;n++){
   currentengine=cbcolor==CB_BLACK?1:2;reset_move_history=n<=2;
   SearchThreadFunc(nullptr);assert(!gameover);domove(cbgame.moves.back().move,cbboard8);cbcolor=CB_CHANGECOLOR(cbcolor);
  }
 }
}
'''
(b/'time_adapter.cpp').write_text(s)
