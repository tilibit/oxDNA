import numpy as np
from Cython.Build import cythonize
from setuptools import setup

setup(
    include_dirs=[np.get_include()],
    ext_modules=cythonize("get_confs.pyx", annotate=True, compiler_directives={"language_level": "3"}),
)
