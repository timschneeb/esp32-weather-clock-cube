Import("env")
import os

def before_upload(source, target, env):
    os.system("pio run -t uploadfs")

env.AddPreAction("upload", before_upload)
