import os
import sys
import time
import uuid

for i in range(0, 100):
    id = str(uuid.uuid1())
    pid = os.fork()
    if pid== 0:
        f = open('.py_test.log.' + str(id) + '.tmp', 'a')
        os.dup2(f.fileno(), sys.stdout.fileno())
        os.execvp('./loader', ['./loader', '../samples/hello', '0']);
        exit()
    else:
        time.sleep(5)
        os.system('kill %d'%pid)
        printed = False
        with open('.py_test.log.' + str(id) + '.tmp', 'r') as f:
            with open('py_test_hint.log', 'a') as f2:
                for line in f.readlines():
                    #if 'jiff' in line and (not 'CPU' in line):
                    if 'CPU' in line:
                        f2.write(line)
                        print("[%d] "%i + line[:-1])
                        printed=True
        if printed:
            os.system('rm .py_test.log.' + str(id) + '.tmp' )
