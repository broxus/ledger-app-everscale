from time import sleep
import ed25519
import pytest
import ledger_client
from ledger_client.ledger_cmd_builder import WalletType
from dataclasses import dataclass
import base64
from ledger_client.utils import save_screenshot, compare_screenshot, PATH_IMG

SIGN_MAGIC: [4] = [0xFF, 0xFF, 0xFF, 0xFF]

SUCCESS = 0x9000

EVER_DECIMALS: int = 9
EVER_TICKER: str = "EVER"
USDT_DECIMALS: int = 9
USDT_TICKER: str = "USDT"
WALLET_ID: int = 0x4BA92D8A
DEFAULT_EXPIRATION_TIMEOUT: int = 60

class SignTransactionMeta:
    chain_id: int
    workchain_id: int
    current_wallet_type: WalletType


def test_sign_tx(cmd):

    account = 0
    wallet_type = WalletType.SafeMultisig
    # # # getting pubkey
    public_key = cmd.get_public_key(account=0, display=False)
    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

    message = "te6ccgEBAQEAOwAAcfDmnGpQVUZxL24fHgfUfLGp2/wzR+YWmZukQraxETyqAAAAxHF5r4KyShssDVOgdrJKGvzpVh6gwA=="
    boc = base64.b64decode(message)
    hash = "1ce8d3bd135bdc9b5538e086643e06eb74ebf6aed06161a14b19bd6abcb602ec"

    sign_transaction_meta = SignTransactionMeta()

    res: list = []
    with cmd.sign_tx(res, account, wallet_type, EVER_DECIMALS, EVER_TICKER, 0, boc[4:]) as exchange:
        if cmd.model == "nanos":
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
        if cmd.model == "nanox":
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
        if cmd.model == "nanosp":
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/sign_confirm_tx/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
    signature = res[0]

    print("SIGNATURE =", signature)
    verifying_key = ed25519.VerifyingKey(public_key, encoding="hex")
    result = 0
    try:
        verifying_key.verify(signature, bytes.fromhex(hash))
        result = SUCCESS
        print("The signature is valid.")
    except ed25519.BadSignatureError:
        print("Invalid signature!")

    assert result == SUCCESS


def test_burn_tx(cmd):
    sleep(0.5)

    account = 0
    wallet_type = WalletType.SafeMultisig
    # # # getting pubkey
    public_key = cmd.get_public_key(account=0, display=False)
    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

    message = "te6ccgEBBQEAyQABYbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGxjJEySil5CY7BZsABAWOAG+Ilaz1wTyTEauoymMGl6o+NGqhszIlHS8BXAmXniYrAAAAAAAAAAAA202lAb5VWNAIBa1YlSK0AAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AMBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJitAEAAA=";
    boc = base64.b64decode(message)
    hash = "1675cfae267250459cee3ccbc1f559322f24f6d0fde33b1b3141f9498a7e4b89"

    sign_transaction_meta = SignTransactionMeta()

    res: list = []
    with cmd.sign_tx(res, account, wallet_type, USDT_DECIMALS, USDT_TICKER, 0, boc[4:]) as exchange:
        if cmd.model == "nanos":
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
        if cmd.model == "nanox":
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
        if cmd.model == "nanosp":
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00000.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00001.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00002.png")
            cmd.client.press_and_release('right')
            compare_screenshot(cmd, f"screenshots/sign_tx/burn_tx/{PATH_IMG[cmd.model]}/00003.png")
            cmd.client.press_and_release('both')
    signature = res[0]
    verifying_key = ed25519.VerifyingKey(public_key, encoding="hex")
    result = 0
    try:
        verifying_key.verify(signature, bytes.fromhex(hash))
        result = SUCCESS
        print("The signature is valid.")
    except ed25519.BadSignatureError:
        print("Invalid signature!")

    assert result == SUCCESS


def test_burn_tx_reject(cmd):
    sleep(0.5)
    account = 0
    wallet_type = WalletType.SafeMultisig
    # # # getting pubkey
    public_key = cmd.get_public_key(account=0, display=False)
    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

    message = "te6ccgEBBQEAyQABYbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGxjJEySil5CY7BZsABAWOAG+Ilaz1wTyTEauoymMGl6o+NGqhszIlHS8BXAmXniYrAAAAAAAAAAAA202lAb5VWNAIBa1YlSK0AAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AMBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJitAEAAA=";
    boc = base64.b64decode(message)
    res: list = []
    with pytest.raises(ledger_client.exception.errors.DenyError) as error:

        with cmd.sign_tx(res, account, wallet_type, USDT_DECIMALS, USDT_TICKER, 0, boc[4:]) as exchange:
            if cmd.model == "nanos":
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')
            if cmd.model == "nanox":
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')
            if cmd.model == "nanosp":
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/burn_reject/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')
    assert error.value.args[0] == '0x6985'


def test_sign_tx_reject(cmd):

    account = 0
    wallet_type = WalletType.SafeMultisig
    # # # getting pubkey
    public_key = cmd.get_public_key(account=0, display=False)
    assert public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"

    message = "te6ccgEBAQEAOwAAcfDmnGpQVUZxL24fHgfUfLGp2/wzR+YWmZukQraxETyqAAAAxHF5r4KyShssDVOgdrJKGvzpVh6gwA=="
    boc = base64.b64decode(message)
    hash = "1ce8d3bd135bdc9b5538e086643e06eb74ebf6aed06161a14b19bd6abcb602ec"

    sign_transaction_meta = SignTransactionMeta()

    res: list = []
    with pytest.raises(ledger_client.exception.errors.DenyError) as error:
        with cmd.sign_tx(res, account, wallet_type, EVER_DECIMALS, EVER_TICKER, 0, boc[4:]) as exchange:
            if cmd.model == "nanos":
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')
            if cmd.model == "nanox":
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')
            if cmd.model == "nanosp":
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00000.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00001.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00002.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00003.png")
                cmd.client.press_and_release('right')
                compare_screenshot(cmd, f"screenshots/sign_tx/sign_reject_tx/{PATH_IMG[cmd.model]}/00004.png")
                cmd.client.press_and_release('both')
    assert error.value.args[0] == '0x6985'

