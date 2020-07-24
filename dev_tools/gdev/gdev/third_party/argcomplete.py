try:
    from argcomplete import autocomplete
except ImportError:
    from subprocess import check_call as _check_call

    _check_call('python3 -m pip install argcomplete'.split())

    from argcomplete import autocomplete
