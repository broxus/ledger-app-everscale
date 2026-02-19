use everscale_ledger_wallet::ledger::WalletType;
use everscale_ledger_wallet::remote_wallet::RemoteWallet;
use ton_block::MsgAddressInt;
use ton_types::{AccountId, UInt256};
use crate::utils::get_ledger;

mod utils;

#[test]
fn ledger_get_address() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    ledger.get_address(0, WalletType::WalletV3, false)?;
    ledger.get_address(0, WalletType::EverWallet, false)?;
    ledger.get_address(0, WalletType::SafeMultisig, false)?;
    ledger.get_address(0, WalletType::SafeMultisig24h, false)?;
    ledger.get_address(0, WalletType::SetcodeMultisig, false)?;
    ledger.get_address(0, WalletType::BridgeMultisig, false)?;
    ledger.get_address(0, WalletType::Surf, false)?;
    ledger.get_address(0, WalletType::Multisig2, false)?;
    ledger.get_address(0, WalletType::Multisig2_1, false)?;
    ledger.get_address(0, WalletType::WalletV5R1, false)?;
    ledger.get_address(0, WalletType::WalletV4R1, false)?;
    ledger.get_address(0, WalletType::WalletV4R2, false)?;
    ledger.get_address(0, WalletType::WalletV3R1, false)?;
    ledger.get_address(0, WalletType::WalletV3R2, false)?;

    Ok(())
}
