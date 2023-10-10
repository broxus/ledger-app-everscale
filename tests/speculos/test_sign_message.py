from time import sleep
import ed25519
import pytest
import ledger_client
from ledger_client.utils import save_screenshot, compare_screenshot, PATH_IMG

SIGN_MAGIC: [4] = [0xFF, 0xFF, 0xFF, 0xFF]

SUCCESS = 0x9000


def test_sign(cmd):
    sleep(0.5)
    if cmd.model == "nanos":
        account = 0
        # # # getting pubkey
        public_key = cmd.get_public_key(account=0, display=False)

        assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

        hash = "c617baa256c79192029d57bf8cce9a0dccd57923faf395e399fe04a4d05ed71d"
        hash_in_bytes = bytes.fromhex(hash)
        assert len(hash_in_bytes) == 32

        prefix = bytes(SIGN_MAGIC)
        assert len(prefix) == 4
        message = prefix + hash_in_bytes

        res: list = []
        # an acc num | bytes to sign
        with cmd.sign_message(res, account, hash_in_bytes) as exchange:
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00004.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00005.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign/sign_message/{PATH_IMG[cmd.model]}/00006.png")
            cmd.client.press_and_release('both')
        signature = res[0]
        assert len(signature) == 64

        verifying_key = ed25519.VerifyingKey(public_key, encoding="hex")
        result = 0
        try:
            verifying_key.verify(signature, message)
            result = SUCCESS
            print("The signature is valid.")
        except ed25519.BadSignatureError:
            print("Invalid signature!")

        assert result == SUCCESS

def test_sign_reject(cmd):
    sleep(0.5)
    if cmd.model == "nanos":
        account = 0
        # # # getting pubkey
        public_key = cmd.get_public_key(account=0, display=False)
        assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

        hash = "c617baa256c79192029d57bf8cce9a0dccd57923faf395e399fe04a4d05ed71d"
        hash_in_bytes = bytes.fromhex(hash)
        assert len(hash_in_bytes) == 32

        prefix = bytes(SIGN_MAGIC)
        assert len(prefix) == 4
        message = prefix + hash_in_bytes

        res: list = []
        # an acc num | bytes to sign
        with pytest.raises(ledger_client.exception.errors.DenyError) as error:

            with cmd.sign_message(res, account, hash_in_bytes) as exchange:
                compare_screenshot(cmd, f"screenshots/sign/sign_reject/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign/sign_reject/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign/sign_reject/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign/sign_reject/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign/sign_reject/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign/sign_reject/{PATH_IMG[cmd.model]}/00005.png")
                cmd.client.press_and_release('both')
        assert error.value.args[0] == '0x6985'


