#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import subprocess
from setuptools import setup, find_packages, Extension

setup(
    name='airport',
    description='A demonstration of the integration of the Gaia COW storage prototype with Postgres, using airport data.',
    version='0.0.16',
    author='Nick Kline',
    author_email='nick@gaiaplatform.io',
    license='(c) Gaia Platform LLC. All rights reserved.',
    packages=['gaia','db_mapper','db_mapper.AirportDemo','db_mapper.multicorn'],
    package_dir={'gaia': 'cow_se', 'db_mapper': '../airport/db_mapper'},
    package_data={'gaia': ['cow_se.so']},
    zip_safe=False
)

