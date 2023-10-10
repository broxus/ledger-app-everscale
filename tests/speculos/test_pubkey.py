from time import sleep

import pytest

import ledger_client
from ledger_client.utils import compare_screenshot, save_screenshot, PATH_IMG


def test_get_public_key(cmd):

    # without display
    public_key = cmd.get_public_key(account=0, display=False)

    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"


