from setuptools import setup, find_packages

setup(
    name="iqos-xapp",
    version="0.0.1",
    description="xApp for QoS management in O-RAN network contexts",
    url="https://github.com/gercom-ufpa/iqos-xapp",
    author="GT-IQoS/UFPA",
    author_email="murilosilva@itec.ufpa.br",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: RNP",
        "Topic :: Software Development :: Network Application",
        # "License :: TODO",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3 :: Only"
    ],
    keywords="o-ran,xapp,network,ufpa,rnp",
    package_dir={"": "src"},
    packages=find_packages(where="src"),
    python_requires=">=3.10",
    install_requires=[
        "ricxappframe",
        "setuptools"
    ],
    entry_points={
        "console_scripts": [
            "run = main:main"
        ]
    }
)
