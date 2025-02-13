from time import sleep

import pytest

import ledger_client
from ledger_client.utils import compare_screenshot, save_screenshot, PATH_IMG


def test_get_public_key_simple(cmd):

    # without display
    public_key = cmd.get_public_key(account=0, display=False)

    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"


def test_get_public_key(cmd):

    # without display
    result: list = []
    with cmd.get_public_key_with_display(account=0, display=False, result=result) as exchange:
        pass

    public_key = result[0]
    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

    # with display
    result: list = []
    with cmd.get_public_key_with_display(account=0, display=True, result=result) as exchange:
        if cmd.model == "nanos":
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00004.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00005.png")
            cmd.client.press_and_release('both')
        if cmd.model == "nanox" or cmd.model == "nanosp":
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/get_pubkey/approve/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
    public_key1 = result[0]
    assert public_key1 == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"


def test_get_public_key_reject(cmd):

    result: list = []
    with pytest.raises(ledger_client.exception.errors.DenyError) as error:
        with cmd.get_public_key_with_display(account=0, display=True, result=result) as exchange:
            if cmd.model == "nanos":
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')

            if cmd.model == "nanox" or cmd.model == "nanosp":
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/get_pubkey/reject/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('both')
    assert error.value.args[0] == '0x6985'