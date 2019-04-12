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
        time.sleep(3)
        os.system('kill %d'%pid)
        with open('.py_test.log.' + str(id) + '.tmp', 'r') as f:
            with open('py_test_no_hint.log', 'a') as f2:
                for line in f.readlines():
                    #if 'jiff' in line and (not 'CPU' in line):
                    if 'time' in line:
                        f2.write(line)

        os.system('rm .py_test.log.' + str(id) + '.tmp' )
