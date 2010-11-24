from twisted.trial import unittest

import multiprocessing
import subprocess
import os

class StartupTest(unittest.TestCase):
    
    image = os.path.dirname(__file__) + "/../../../images/benchmarks/benchmarks.image"
    rvm   = os.path.dirname(__file__) + "/../build/rvm"
    
    def test_hello_world(self):
        p = subprocess.Popen([self.rvm, "-headless", self.image, "HelloWorld"],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEquals(0, p.wait()) # ends without error
        
        # output containts a line like this: 11 on 0: primitivePrint: Hello World!
        gotHelloWorld = False
        for l in p.stdout.readlines():
            gotHelloWorld = gotHelloWorld or l.endswith("Hello World!\n")
        
        self.assertTrue(gotHelloWorld)
    
    def test_mmap_bug(self):
        cpu_count = multiprocessing.cpu_count()
        self.assertTrue(cpu_count > 1)
        devnull = open(os.devnull, 'w')
        
        for heap in [128, 256, 512, 1024]:
            for c in range(1, cpu_count):
                self.assertEquals(0, 
                    subprocess.call([self.rvm, "-num_cores", str(c),
                                     "-min_heap_MB", str(heap),
                                     "-headless", self.image, "HelloWorld"],
                                    stdout=devnull, stderr=devnull),
                    "Failed starting rvm with -num_cores %d -min_heap_MB %d" %(c, heap))

