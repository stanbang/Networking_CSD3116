
from pathlib import Path
import subprocess,os,re,time,sys
repo=Path('/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116');os.chdir(repo)
sys.path.insert(0,str(repo/'tests'))
from p1_network import wait_line,stop
from verify_audit import verify
out=repo/'build/evidence/requirements-auth-20261105-02';out.mkdir(exist_ok=False)
bundle=(repo/'build/local-security-dir').read_text().strip()
children=[];streams=[];env=dict(os.environ)
for k in ('LD_PRELOAD','CYCLONEDDS_URI','LP_NUM_THREADS'):env.pop(k,None)
def start(name,kind,args):
 path=out/(name+'.log');stream=path.open('x');streams.append(stream)
 child=subprocess.Popen([str(repo/'build/release'/kind),'--security-dir',bundle,*args],stdout=stream,stderr=subprocess.STDOUT,env=env);children.append(child)
 return child,path
try:
 server,serverlog=start('server','server',['--duration','40','--audit-file',str(out/'audit.log')])
 wait_line(serverlog,'SERVER_READY ',server)
 legitimate,path=start('legitimate','client',['--identity','1','--headless','--duration','30'])
 wait_line(path,r'CLIENT_JOIN .*status=accepted',legitimate)
 for identity,fault in [(6,'wrong-ca'),(3,'no-cert')]:
  bad,badlog=start(fault,'client',['--identity',str(identity),'--headless','--duration','3','--fault',fault])
  code=bad.wait(timeout=12)
  assert 'status=accepted' not in badlog.read_text(),fault+' accepted'
  assert code!=0,fault+' did not report failed admission'
  assert legitimate.poll() is None and server.poll() is None
  print('PASS '+fault+': no admission, client exit='+str(code)+', legitimate peer remains alive',flush=True)
 assert stop(legitimate)==0
 assert stop(server)==0
 text=serverlog.read_text();summary=re.search(r'SERVER_SUMMARY (.*)',text).group(1)
 counts={k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',summary)}
 assert counts['joins']==1 and counts['peak']==1 and counts['leaves']==1 and counts['timeouts']==0
 audit=(out/'audit.log').read_text()
 assert 'DDS_AUTH_WARNING' in audit,'No authentication rejection audit evidence'
 assert 'DDS_UNAUTHENTICATED_DISCOVERY' in audit,'No absent-certificate discovery audit evidence'
 anchor=re.search(r'AUDIT_ROOT hash=([0-9a-f]{64}) records=(\d+)',text)
 verify(out/'audit.log',anchor.group(1),int(anchor.group(2)))
 print('SERVER_SUMMARY '+summary,flush=True)
 print('PASS rejected peers used no slot; authentication and unauthenticated-discovery audit evidence verified',flush=True)
finally:
 for child in reversed(children):
  if child.poll() is None:stop(child)
 for stream in streams:stream.close()
