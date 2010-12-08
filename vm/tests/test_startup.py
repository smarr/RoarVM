from twisted.trial import unittest

import multiprocessing
import subprocess
import os
import exceptions

def determine_launch_executable():
    """
        Here we try to figure out whether it is a standard binary, or
        whether it might be the Tilera binary.
        If it complaines that it cannot be executed, we make the guess
        that it is a Tilera binary.
    """
    devnull = open(os.devnull, 'w')
    binary = os.path.dirname(__file__) + "/../build/rvm"
    try:
        exitcode = subprocess.call([binary], stdout=devnull, stderr=devnull)
    except exceptions.OSError as e:
        if e.errno  == 8:
            exitcode = 126
        else:
           raise e

    assert(exitcode == 1 or exitcode == 126)
    if exitcode == 1:
        return [binary]
    else:
        return [os.path.dirname(__file__) + "/../run/tile-runner",
                binary]

class StartupTest(unittest.TestCase):
    
    image = os.path.dirname(__file__) + "/../../../images/benchmarks/benchmarks.image"
    rvm = determine_launch_executable()
    on_tilera = len(rvm) == 2
    cpu_count = 59 if on_tilera else multiprocessing.cpu_count()
    
    
    def test_hello_world(self):
        cmd = self.rvm + ["-headless", self.image, "HelloWorld"]
        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEquals(0, p.wait(), "Execution failed" + " ".join(cmd)) # ends without error
        
        # output containts a line like this: 11 on 0: primitivePrint: Hello World!
        gotHelloWorld = False
        for l in p.stdout.readlines():
            gotHelloWorld = gotHelloWorld or l.endswith("Hello World!\n")
        
        self.assertTrue(gotHelloWorld, "Failed to read Hello World! in output")
    
    def test_mmap_bug(self):
        self.assertTrue(self.cpu_count > 1)
        devnull = open(os.devnull, 'w')
        
        for heap in [128, 256, 512, 1024]:
            for c in [n for n in [1,2,3,7,11,15,55,57,59] if self.cpu_count >= n]:
                cmd = self.rvm + ["-num_cores", str(c),
                                  "-min_heap_MB", str(heap),
                                  "-headless", self.image, "HelloWorld"]
                self.assertEquals(0, 
                    subprocess.call(cmd, stdout=devnull, stderr=devnull),
                    "Failed starting rvm with -num_cores %d -min_heap_MB %d: %s" %(c, heap, " ".join(cmd)))
                    
    def test_reliability(self):
        self.assertTrue(self.cpu_count > 1)
        devnull = open(os.devnull, 'w')
        cmd = self.rvm + ["-num_cores", str(self.cpu_count),
                         "-min_heap_MB", "256" if self.on_tilera else "1024",
                         "-headless", self.image, "HelloWorld"]
        for i in range(1, 10):    
            self.assertEquals(0,
                subprocess.call(cmd, stdout=devnull, stderr=devnull),
                "Failed reliability test at iteration %d: %s" %(i, " ".join(cmd)))

