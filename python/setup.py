from setuptools import setup
from setuptools import find_packages

from maid import __version__

def run_setup():
    setup(
            name="pymaid",
            version=__version__,
            url="https://github.com/w359405949/libmaid",
            description="gevent(libev) & google protobuf based rpc framework",
            keywords = ("rpc", "protobuf", "gevent"),

            author="w359405949",
            author_email="w359405949@gmail.com",
            packages=find_packages(),

            install_requires=[
                "gevent>=1.0",
                "protobuf>=2.4",
                ]
            )

if __name__ == "__main__":
    run_setup()
