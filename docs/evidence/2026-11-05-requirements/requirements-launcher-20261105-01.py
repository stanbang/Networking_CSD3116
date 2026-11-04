
from pathlib import Path
import subprocess,os,re,time,signal,sys,shutil
from types import SimpleNamespace
repo=Path('/mnt/c/Users/stanl/Documents/GitHub/Networking_CSD3116');os.chdir(repo)
sys.path.insert(0,str(repo/'tests'))
from p3_gui import window_for,command,snapshots,watch_snapshot_progress,assert_fresh_progress,assert_live_state
out=repo/'build/evidence/requirements-launcher-20261105-01';out.mkdir(exist_ok=False)
stream=(out/'launcher.log').open('x');session=None;pidfd=None
launcher=subprocess.Popen(['bash','tools/play_local.sh','--duration','45'],stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
try:
 deadline=time.monotonic()+30
 while True:
  text=(out/'launcher.log').read_text()
  match=re.search(r'Local session started\. Logs: (\S+) server=(\d+) client1=(\d+) client2=(\d+)',text)
  if match:break
  assert launcher.poll() is None,'Automatic launcher exited during startup'
  assert time.monotonic()<deadline,'Automatic launcher startup deadline'
  time.sleep(.1)
 session=repo/match.group(1)
 pids=[int(match.group(3)),int(match.group(4))]
 pidfd=os.pidfd_open(pids[0])
 windows=[window_for(SimpleNamespace(pid=pid)) for pid in pids]
 assert len(set(windows))==2
 for window in windows:
  command('xdotool','windowraise',window)
  command('xdotool','windowfocus','--sync',window)
  command('xdotool','keydown','Up','Right','space')
  time.sleep(1.2)
  command('xdotool','keyup','Up','Right','space')
 progress=watch_snapshot_progress(session,11)
 snapshots(out,windows,'launcher')
 assert_fresh_progress(progress)
 assert_live_state(session)
 assert launcher.poll() is None
 print('PASS automatic launcher: server first, two admitted distinct GUI clients, real key input, fresh state >10s',flush=True)
 signal.pidfd_send_signal(pidfd,signal.SIGTERM)
 code=launcher.wait(timeout=10)
 assert code==0,'Managed session did not exit cleanly after first client left: '+str(code)
 text=(session/'server.log').read_text()
 match=re.search(r'SERVER_SUMMARY (.*)',text);assert match
 counts={k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',match.group(1))}
 assert counts['joins']==2 and counts['peak']==2 and counts['timeouts']==0 and counts['invalid']==0
 assert counts['moves']>0 and counts['shots']>0 and counts['snapshots']>0
 assert 'SERVER_LEAVE identity=1 ' in text,'No explicit first client Leave'
 print('SERVER_SUMMARY '+match.group(1),flush=True)
 print('PASS client Leave received; automatic session shutdown exit0',flush=True)
finally:
 if launcher.poll() is None:
  launcher.terminate()
  try:launcher.wait(timeout=8)
  except subprocess.TimeoutExpired:
   os.killpg(launcher.pid,signal.SIGKILL);launcher.wait();print('FAIL forced launcher cleanup',flush=True)
 if pidfd is not None:os.close(pidfd)
 stream.close()
 if session is not None:
  for name in ('server.log','client1.log','client2.log','audit.log'):
   shutil.copyfile(session/name,out/name)
