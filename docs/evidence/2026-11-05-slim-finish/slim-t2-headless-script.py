
from pathlib import Path
import subprocess, time, re, os, sys, json, shutil
root=Path('/home/stanley/.local/state/dds-asteroids/slim-check-20261105-01'); clone=root/'source'
sys.path.insert(0,str(clone/'tests'))
from p1_network import wait_line,stop
from verify_audit import verify
out=root/'headless';out.mkdir(exist_ok=False)
children=[];streams=[];env=dict(os.environ)
for key in ('LD_PRELOAD','LP_NUM_THREADS','CYCLONEDDS_URI'): env.pop(key,None)
bundle='/home/stanley/.local/state/dds-asteroids/credentials-integration-20261104-01'
def start(name,kind,args):
 path=out/(name+'.log');stream=path.open('x');streams.append(stream)
 child=subprocess.Popen([str(clone/'build/release'/kind),'--security-dir',bundle,*args],cwd=clone,env=env,stdout=stream,stderr=subprocess.STDOUT);children.append(child)
 return child,path
try:
 server,serverlog=start('server','server',['--duration','150','--audit-file',str(out/'audit.log')])
 wait_line(serverlog,r'SERVER_READY ',server)
 clients=[];ids=[]
 for identity in (1,2):
  child,path=start('client'+str(identity),'client',['--identity',str(identity),'--headless','--bot','circle','--duration','125'])
  match=wait_line(path,r'CLIENT_JOIN .* id=([1-5]) status=accepted',child)
  ids.append(int(match.group(1)));clients.append((child,path))
 assert len(set(ids))==2
 joined=time.monotonic()
 for child,path in clients: assert child.wait(timeout=140)==0, 'Client exit failed'
 elapsed=time.monotonic()-joined
 assert elapsed>=120, 'Less than two minutes of concurrent admitted runtime'
 assert stop(server)==0
 text=serverlog.read_text();summary=re.search(r'SERVER_SUMMARY (.*)',text).group(1)
 counts={k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',summary)}
 assert counts['joins']==2 and counts['leaves']==2 and counts['timeouts']==0 and counts['invalid']==0
 assert counts['moves']>0 and counts['shots']>0 and counts['inputs']>0
 for child,path in clients:
  ticks=[int(x) for x in re.findall(r'CLIENT_SNAPSHOT tick=(\d+)',path.read_text())]
  assert len(ticks)>=100 and ticks[-1]-ticks[0]>=7000, 'Own-state stream did not cover soak'
 anchor=re.search(r'AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)',text)
 verify(out/'audit.log',anchor.group(1),int(anchor.group(2)))
 print('PASS two-client headless elapsed_after_both_admitted_seconds='+str(round(elapsed,3)),flush=True)
 print('SERVER_SUMMARY '+summary,flush=True)
 print('PASS distinct identities, sustained own-state streams, explicit leaves and audit chain',flush=True)
finally:
 for child in reversed(children):
  if child.poll() is None: stop(child)
 for stream in streams:stream.close()
 shutil.copytree(out,Path('/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116/build/evidence/slim-t2-headless'))
