Import("env")
import os

def before_build(source, target, env):
    os.system("pio run -t buildfs")

env.AddPreAction("buildprog", before_build)
