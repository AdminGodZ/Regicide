# Opponent options: what each opponent DLL answers to our recipe's option lines

Host: cbhost.exe from master fdd154e. Each engine in one fresh process; the lines in the order of bl_run_w.sh,
each followed by "get <name>". Raw replies below; "reply 1" = accepted, "reply 0" = rejected.

## Cake 1.89f

```
set dbpath C:\regicide-match\db\kingsrow_wld8 -> reply 1
get dbpath -> reply 1 C:\\regicide-match\\db\\kingsrow_wld8
set enable_wld 1 -> reply 0 Enable WLD set to 1
get enable_wld -> reply 1 1
set dbmbytes 512 -> reply 1
get dbmbytes -> reply 1 512
set max_dbpieces 8 -> reply 0 max DB pieces to use set to 8\nYou must restart Cake for the\nchange to take effect
get max_dbpieces -> reply 1 8
set hashsize 128 -> reply 1 hash size set to 128
get hashsize -> reply 1 128
set book 3 -> reply 1 Book set to 3
get book -> reply 1 3
book read-back: 'reply 1 3' (ok)
```

## KingsRow 1.20

```
set dbpath C:\regicide-match\db\kingsrow_wld8 -> reply 1 dbpath set to C:\\regicide-match\\db\\kingsrow_wld8
get dbpath -> reply 1 C:\\regicide-match\\db\\kingsrow_wld8
set enable_wld 1 -> reply 1 enable_wld set to 1
get enable_wld -> reply 1 1
set dbmbytes 512 -> reply 1 dbmbytes set to 512
get dbmbytes -> reply 1 512
set max_dbpieces 8 -> reply 1 max_dbpieces set to 8
get max_dbpieces -> reply 1 8
set hashsize 128 -> reply 1
get hashsize -> reply 1 128
set searchthreads 1 -> reply 1 searchthreads set to 1
get searchthreads -> reply 1 1
set book 3 -> reply 1 opening book set to best
get book -> reply 1 3
book read-back: 'reply 1 3' (ok)
```

## GuiNN 2.06

```
set dbpath C:\regicide-match\db\kingsrow_wld8 -> reply 1 dbpath set to C:\\regicide-match\\db\\kingsrow_wld8
get dbpath -> reply 1 C:\\regicide-match\\db\\kingsrow_wld8
set enable_wld 1 -> reply 1 enable_wld set to 1
get enable_wld -> reply 1 1
set dbmbytes 512 -> reply 1 dbmbytes set to 512
get dbmbytes -> reply 1 512
set max_dbpieces 8 -> reply 1 max_dbpieces set to 8
get max_dbpieces -> reply 1 8
set hashsize 128 -> reply 1
get hashsize -> reply 1 128
set book 3 -> reply 1 book set to 3
get book -> reply 1 3
book read-back: 'reply 1 3' (ok)
```

## Reading

- **KingsRow 1.20 and GuiNN 2.06** accept every line (`reply 1`) and read back the values: they take effect at once,
  in the running process.
- **Cake 1.89f** accepts dbpath, dbmbytes, hashsize and book; it answers **`reply 0`** to `set enable_wld 1` ("Enable
  WLD set to 1") and to `set max_dbpieces 8` ("You must restart Cake for the change to take effect"), although `get`
  reads back the new values. Cake keeps its settings in the registry (`HKCU\Software\Martin Fierz\Cake`) and reads them
  when it is loaded; the values stored there now are ours (Maxpieces 8, Enable WLD 1, DBpath
  C:\regicide-match\db\kingsrow_wld8, Hashsize 128, DBcachesize 512, Use Book 3). Every Cake process of our runs is a
  new load (cbhost loads the DLL at start), so it started with these stored values; the two rejected lines only mean
  "takes effect at the next load".
- **For a real CheckerBoard match**: set these in CheckerBoard's engine options for each engine; for Cake the database
  settings (max pieces, WLD) take effect only after Cake is loaded again (restart CheckerBoard, or select the engine
  anew), as its own reply says. KingsRow's `searchthreads 1` is our setting for one thread per engine; CheckerBoard
  sends whatever its own options hold.
