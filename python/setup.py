from distutils.core import setup

from maid import __version__

def run_setup():
    setup(name="maid",
            version=__version__)

if __name__ == "__main__":
    run_setup()
