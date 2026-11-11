import ctypes as C,pathlib,sys,csv
lib=C.CDLL(str(pathlib.Path(sys.argv[1]).resolve()));lib.enginecommand.argtypes=[C.c_char_p,C.c_char_p]
commands=['name','get protocolversion','get gametype','get hashsize','get book','get threads','get searchthreads','get cpus','get dbpath','get dbpieces','get dbmbytes','get allscores','get solve','get enable_wld','get max_dbpieces','get enable_mtc','get mtcpath','get bookfile','get movelist B:W1:B']
if len(sys.argv)>2:commands+=['set hashsize 64','get hashsize','set book 2','get book','set book 3','get book','set searchthreads 1','get threads','set allscores 1','set dbmbytes 16','get dbmbytes','set dbpath '+sys.argv[2],'get dbpath','get dbpieces']
w=csv.writer(sys.stdout,delimiter='\t',lineterminator='\n');w.writerow(['command','handled','reply'])
for cmd in commands:
 out=C.create_string_buffer(1024);rc=lib.enginecommand(cmd.encode(),out)
 w.writerow([cmd,rc,out.value.decode().replace('\n','\\n')or '(empty)'])
