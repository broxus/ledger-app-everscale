from time import sleep
import ed25519
import numpy as np


SIGN_MAGIC: [4] = [0xFF, 0xFF, 0xFF, 0xFF]



def test_sign_message(cmd):
    sleep(0.5)
    if cmd.model == "nanos":
        account = 0

        # # # getting pubkey

        #result: list = []
        #with cmd.get_public_key(account=0, display=False, result=result) as exchange:
       #     pass

        #public_key = result[0]
        #assert (
        #public_key == "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84")

        public_key = "3099f14eccaa0542d2d60e92eb66495f6ecf01a114e12f9db8d9cb827a87bf84"


        # # #

        hash = "c617baa256c79192029d57bf8cce9a0dccd57923faf395e399fe04a4d05ed71d"

        hash_in_bytes = bytes.fromhex(hash)
        print("hash_in_bytes", hash_in_bytes)
        assert len(hash_in_bytes) == 32
        # an acc num | bytes to sign
        prefix = bytes(SIGN_MAGIC)
        message = prefix + hash_in_bytes

        signature = cmd.sign_message(account, message)
        assert len(signature) == 64
        #print("END SIGN", signature)
        #message = [SIGN_MAGIC, hash_in_bytes]
        #print("END MESSAGE", message)
        verifying_key = ed25519.VerifyingKey(public_key, encoding="hex")
        try:
            verifying_key.verify(signature, hash_in_bytes)
            print("The signature is valid.")
        except ed25519.BadSignatureError:
            print("Invalid signature!")

        assert len(message) == 128 #  !!!!!!!!!!!!!THIS IS FOR TEST TO FAIL AND SHOW ERROR LOG






