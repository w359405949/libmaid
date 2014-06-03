from distutils.core import setup

from maid import __version__

def protoc():
    pass

def run_setup():
    protoc()
    setup(
            name="maid",
            version=__version__,
            url="https://github.com/w359405949/libmaid",
            description="gevent & protobuf based rpc framework",
            keywords = ("rpc", "protobuf", "gevent", "example"),

            author="w359405949",
            author_email="w359405949@gmail.com",
            packages=["maid"]
            )

if __name__ == "__main__":
    run_setup()
