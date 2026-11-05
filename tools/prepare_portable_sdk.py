"""Package installed Ubuntu24.04 x86-64 runtime SDK; developer packaging only.
Team/Codex2026-11-05. Never copies user keys, profiles or account data."""
from pathlib import Path
import subprocess,shutil,sys,sysconfig,re,json,os,hashlib
root=Path(__file__).resolve().parents[1];sdk=root/'portable/linux-x86_64'
assert subprocess.check_output(['uname','-m'],text=True).strip()=='x86_64'
assert not sdk.exists(),'Use a fresh SDK destination; never overwrite a verified package'
sdk.mkdir(parents=True);(sdk/'lib').mkdir();(sdk/'bin').mkdir();(sdk/'include').mkdir();(sdk/'licenses').mkdir()
# Public build headers, including Ubuntu's multiarch OpenSSL configuration.
for name in ('dds','GLFW','GL','KHR','openssl'):
 shutil.copytree(Path('/usr/include')/name,sdk/'include'/name)
for p in Path('/usr/include/x86_64-linux-gnu/openssl').iterdir():
 if p.is_file():shutil.copy2(p,sdk/'include/openssl'/p.name)
for name in ('sqlite3.h','sqlite3ext.h'):shutil.copy2(Path('/usr/include')/name,sdk/'include'/name)
listing=subprocess.check_output(['ldconfig','-p'],text=True);libraries={}
for line in listing.splitlines():
 if '=>' in line and 'x86-64' in line:
  libraries.setdefault(line.strip().split()[0],Path(line.split('=>',1)[1].strip()))
base={'libc.so.6','libm.so.6','libdl.so.2','libpthread.so.0','librt.so.1','ld-linux-x86-64.so.2'}
needed={};seen=set();packages=set()
def license_for(path):
 for candidate in (path,path.resolve()):
  query=subprocess.run(['dpkg-query','-S',str(candidate)],capture_output=True,text=True)
  if query.returncode==0:
   for line in query.stdout.splitlines():
    if ': /' not in line:continue
    package=line.split(': /',1)[0].split(':',1)[0]
    source=Path('/usr/share/doc')/package/'copyright'
    if source.is_file():shutil.copyfile(source,sdk/'licenses'/(package+'.copyright'));packages.add(package)
   return
def deps(path):
 real=path.resolve()
 if real in seen:return
 seen.add(real);license_for(path)
 info=subprocess.run(['ldd',str(path)],capture_output=True,text=True)
 assert info.returncode==0,'ldd failed: '+str(path)
 assert 'not found' not in info.stdout,'Unresolved dependency: '+str(path)
 for line in info.stdout.splitlines():
  m=re.match(r'\s*(\S+)\s+=>\s+(/\S+)',line)
  if not m:continue
  name,source=m.group(1),Path(m.group(2))
  if name in base:continue
  if name not in needed:
   needed[name]=source;shutil.copyfile(source,sdk/'lib'/name);deps(source)
roots={name:libraries[name] for name in ('libddsc.so.0debian','libglfw.so.3','libGL.so.1','libGLX_mesa.so.0','libssl.so.3','libcrypto.so.3','libsqlite3.so.0')}
for name,source in roots.items():
 needed[name]=source;shutil.copyfile(source,sdk/'lib'/name);deps(source)
for p in Path('/usr/lib/x86_64-linux-gnu/libddsc0debian').glob('libdds_security_*.so'):
 shutil.copyfile(p,sdk/'lib'/p.name);deps(p)
assert all((sdk/'lib'/('libdds_security_'+n+'.so')).exists() for n in ('auth','ac','crypto'))
for alias,target in {'libddsc.so':'libddsc.so.0debian','libglfw.so':'libglfw.so.3','libGL.so':'libGL.so.1','libssl.so':'libssl.so.3','libcrypto.so':'libcrypto.so.3','libsqlite3.so':'libsqlite3.so.0'}.items():
 shutil.copyfile(sdk/'lib'/target,sdk/'lib'/alias)
(sdk/'lib/dri').mkdir();driver=Path('/usr/lib/x86_64-linux-gnu/dri/swrast_dri.so')
shutil.copyfile(driver,sdk/'lib/dri/swrast_dri.so');deps(driver)
for source,name in [(Path('/usr/bin/openssl'),'openssl'),(Path(sys.executable).resolve(),'python3')]:
 shutil.copyfile(source,sdk/'bin'/name);os.chmod(sdk/'bin'/name,0o755);deps(source)
stdlib=Path(sysconfig.get_path('stdlib'));version='python'+str(sys.version_info.major)+'.'+str(sys.version_info.minor)
shutil.copytree(stdlib,sdk/'lib'/version,ignore=shutil.ignore_patterns('__pycache__','test','tests','idlelib','tkinter','turtledemo','site-packages','dist-packages'))
for extension in (sdk/'lib'/version/'lib-dynload').glob('*.so'):deps(stdlib/'lib-dynload'/extension.name)
(sdk/'openssl.cnf').write_text('openssl_conf = init\n[init]\nproviders = providers\n[providers]\ndefault = default_provider\n[default_provider]\nactivate = 1\n')
gen=root/'portable/generated';gen.mkdir()
for name in ('messages','accounts'):
 for suffix in ('.c','.h'):shutil.copyfile(root/'build/gen'/(name+suffix),gen/(name+suffix))
(gen/'idl.sha256').write_text(''.join(hashlib.sha256((root/'idl'/(n+'.idl')).read_bytes()).hexdigest()+'  idl/'+n+'.idl\n' for n in ('messages','accounts')))
manifest={'target':'Ubuntu24.04 x86_64 / glibc2.39 / WSLg','compiler':'Host GCC11+ with standard C/binutils headers; not bundled','system_base_libraries_not_bundled':sorted(base),'libraries':{n:str(p) for n,p in sorted(needed.items())},'copyright_packages':sorted(packages),'python':sys.version,'runtime_files':{str(p.relative_to(sdk)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sdk.rglob('*') if p.is_file()}}
(sdk/'manifest.json').write_text(json.dumps(manifest,indent=2))
size=sum(p.stat().st_size for p in sdk.rglob('*') if p.is_file())
print('PASS prepared local offline SDK; headers, game/security/graphics libraries, OpenSSL/Python helpers and generated DDS source')
print('SDK MiB:',round(size/1048576,1));print('Shared dependencies:',len(needed));print('License/copyright records:',len(packages))
print('No user keys/profiles/databases copied; no runtime package installation performed')
