Import("env")
import os

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"

def before_build(source, target, env):
    os.system("pio run -t buildfs")

env.AddPreAction(APP_BIN, before_build)
