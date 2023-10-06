from time import sleep

from ledger_client.ledger_cmd_builder import WalletType

def test_address(cmd):
    sleep(0.5)
    # True = verify address, False = skip display verification
    if cmd.model == "nanos":
        # Test with Wallet v3
        assert cmd.get_address(0, WalletType.WalletV3, False) == "7571b498e3fed7a0fffbe21377e50548c92da4a04842e1b163547d3e8980cf64"
        # Test with Everwallet
        assert cmd.get_address(0, WalletType.EverWallet, False) == "522a4cc774797f537c41b4853a9b83f359fe2802a5cf3bd9f31240c495e82358"
        # Test with SafeMultisig
        assert cmd.get_address(0, WalletType.SafeMultisig, False) == "ae990783e06a196ab03029fe4517dda0ea318c091cd49ff51cc0a40369b0aff5"
        # Test with SafeMultisig24h
        assert cmd.get_address(0, WalletType.SafeMultisig24h, False) == "98135fb68e91833e810122abfe00ff3b38c1d555bf773741f869dea8b87fb72d"
        # Test with SetcodeMultisig
        assert cmd.get_address(0, WalletType.SetcodeMultisig, False) == "23e76dee54e3f715e11cf374177e786878814ad2c7ac6e38bc06515efdb5fab3"
        # Test with BridgeMultisig
        assert cmd.get_address(0, WalletType.BridgeMultisig, False) == "0806dbe6c4581af1165879fd196d3e02404029540e818921edfedbffb46d3c65"
        # Test with Surf
        assert cmd.get_address(0, WalletType.Surf, False) == "ec03eb7af13d70083b9f3c8202b0321ede255edc624292c531106f50d9f005b3"
        # Test with Multisig2
        assert cmd.get_address(0, WalletType.Multisig2, False) == "9760a1b7393cdbb389205f72ebf4d7e362b06b419fdac9aeccd83bf39ce0d05a"
        # Test with Multisig2_1
        assert cmd.get_address(0, WalletType.Multisig2_1, False) == "1de569744cf341d8e2e35996f23cdf5d5f9226c1400c98100f480d89e70c6bcf"








