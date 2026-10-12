"""Host orchestration contracts; no Podman installation required for these checks."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MOCK = r'''#!/usr/bin/env python3
import json, os, pathlib, sys
args=sys.argv[1:]
with open(os.environ['MOCK_LOG'],'a') as log:log.write(json.dumps(args)+'\n')
operation=args[0]
if operation==os.environ.get('MOCK_FAIL'):sys.exit(9)
if operation=='create':print('temporary-container')
if operation=='cp':
    dest=pathlib.Path(args[2]);dest.mkdir(exist_ok=True)
    if os.environ.get('MOCK_INCOMPLETE'):sys.exit(0)
    for name in ['escalation','escalation_headless','escalation.exe','escalation_headless.exe','data/world.geojson','data/histories.json','README.md','THIRD_PARTY_NOTICES.md']:
        p=dest/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('new-package')
'''

class BuildPipeline(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)/'repo with spaces';self.root.mkdir()
        shutil.copy(ROOT/'build.sh',self.root/'build.sh')
        self.mock=self.root/'fake-podman';self.mock.write_text(MOCK);self.mock.chmod(0o755)
        self.log=self.root/'calls.jsonl'
        self.env=dict(os.environ,CONTAINER_ENGINE=str(self.mock),MOCK_LOG=str(self.log))
    def run_build(self,target='linux',**env):
        return subprocess.run(['sh',str(self.root/'build.sh'),target],cwd=self.root.parent,env=dict(self.env,**env),capture_output=True,text=True)
    def calls(self):
        return [json.loads(s) for s in self.log.read_text().splitlines()] if self.log.exists() else []
    def test_linux_package_and_no_mounts(self):
        result=self.run_build();self.assertEqual(result.returncode,0,result.stderr)
        calls=self.calls();self.assertEqual([c[0] for c in calls],['build','create','cp','rm'])
        self.assertIn('linux-artifacts',calls[0]);self.assertIn('linux/amd64',calls[0]);self.assertEqual(calls[0][-1],str(self.root))
        self.assertFalse(any(x in calls[0] for x in ['-v','--volume','compose','run']))
        self.assertTrue((self.root/'dist-linux/escalation').is_file())
        self.assertTrue(os.access(self.root/'dist-linux/escalation',os.X_OK))
    def test_windows_and_both_targets(self):
        result=self.run_build('all');self.assertEqual(result.returncode,0,result.stderr)
        builds=[c for c in self.calls() if c[0]=='build']
        self.assertEqual(len(builds),2);self.assertIn('windows-artifacts',builds[1])
        self.assertTrue((self.root/'dist-windows/escalation.exe').is_file())
    def test_build_failure_never_extracts(self):
        result=self.run_build(MOCK_FAIL='build');self.assertEqual(result.returncode,9)
        self.assertEqual([c[0] for c in self.calls()],['build'])
    def test_copy_failure_cleans_container_preserves_previous(self):
        previous=self.root/'dist-linux';previous.mkdir();(previous/'old').write_text('keep')
        result=self.run_build(MOCK_FAIL='cp');self.assertEqual(result.returncode,9)
        self.assertEqual((previous/'old').read_text(),'keep');self.assertEqual(self.calls()[-1][0],'rm')
    def test_incomplete_package_is_rejected(self):
        result=self.run_build(MOCK_INCOMPLETE='1');self.assertNotEqual(result.returncode,0)
        self.assertIn('Incomplete package',result.stderr);self.assertFalse((self.root/'dist-linux').exists())
        self.assertEqual(self.calls()[-1][0],'rm')
    def test_rebuild_preserves_old_package(self):
        previous=self.root/'dist-linux';previous.mkdir();(previous/'old').write_text('keep')
        result=self.run_build();self.assertEqual(result.returncode,0,result.stderr)
        backups=list(self.root.glob('dist-linux.previous-*'));self.assertEqual(len(backups),1)
        self.assertEqual((backups[0]/'old').read_text(),'keep')
    def test_clean_build_and_invalid_arguments(self):
        result=self.run_build(CLEAN_BUILD='1',JOBS='3');self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn('--no-cache',self.calls()[0]);self.assertIn('JOBS=3',self.calls()[0])
        count=len(self.calls());self.assertEqual(self.run_build('unknown').returncode,2)
        self.assertEqual(self.run_build(JOBS='invalid').returncode,2);self.assertEqual(len(self.calls()),count)

if __name__=='__main__':unittest.main()
