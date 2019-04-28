import os
import sys
import time
import uuid
import numpy as np

jiff = list()
aex = list()
for i in range(0, 20):
    id = str(uuid.uuid1())
    pid = os.fork()
    if pid== 0:
        f = open('.py_test.log.' + str(id) + '.tmp', 'a')
        os.dup2(f.fileno(), sys.stdout.fileno())
        os.execvp('./loader', ['./loader', '../samples/hello', '0']);
        exit()
    else:
        time.sleep(2)
        os.system('kill %d'%pid)
        printed = False
        with open('.py_test.log.' + str(id) + '.tmp', 'r') as f:
            with open('py_test_no_hint.log', 'a') as f2:
                for line in f.readlines():
                    #if 'jiff' in line and (not 'CPU' in line):
                    if 'CPU cycles' in line:
                        f2.write(line[line.find('cycles')+len('cycles: '):])
                        jiff.append(int(line[line.find('cycles')+len('cycles: '):]))
                        print("[%d] "%i + line[:-1])
                        continue
                    if 'aex' in line:
                        print("[%d] "%i + line[:-1])
                        line = line[line.find('aex')+len('aex: '):]
                        line = line[:line.find(',')]
                        aex.append(int(line))
        os.system('rm .py_test.log.' + str(id) + '.tmp' )
jiff = np.array(jiff)
jiff = jiff[abs(jiff - np.mean(jiff)) < 2 * np.std(jiff)]
print("median: %d"%np.median(jiff))
print("mean: %d"%jiff.mean())

aex = np.array(aex)
aex = aex[abs(aex - np.mean(aex)) < 2 * np.std(aex)]
print("median: %d"%np.median(aex))
print("mean: %d"%aex.mean())
