from setuptools import setup, find_packages

setup(
    name='blob',
    version='1.0.0',
    author='Joe McKee',
    author_email='jsphmck92@gmail.com',
    description='Blob, stream and receive arbitrary variables via udp and websockets.',
    packages=find_packages(),
    package_data={'': ['blob/*']},
    include_package_data=True,
)
