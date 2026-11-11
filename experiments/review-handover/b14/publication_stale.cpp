#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstring>
#include <string>
static bool armed=false;static std::size_t fail_size=0;static int refused=0;
void*operator new(std::size_t n){if(armed){++refused;throw std::bad_alloc();}if(auto p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}
extern "C" int enginecommand(const char*,char*);
int main(){char r[1024];enginecommand("name",r);std::string path="/tmp/regicide-review-r1.tQRfyf/round14/master/src/weights/net.bin",c="set evalfile "+path;if(enginecommand(c.c_str(),r)!=1)return 2;enginecommand("get book",r);std::printf("before get book: %s\n",r);fail_size=path.size()+1;armed=true;int rc=enginecommand("set book 0",r);armed=false;std::printf("set book 0: rc=%d reply=%s refused=%d\n",rc,r,refused);enginecommand("get book",r);std::printf("after allocation recovers get book: %s\n",r);return rc==1&&refused>0&&std::strcmp(r,"3")==0?0:3;}
