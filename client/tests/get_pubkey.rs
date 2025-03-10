use everscale_ledger_wallet::remote_wallet::RemoteWallet;

use crate::utils::get_ledger;

mod utils;

#[test]
fn ledger_get_pubkey() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    ledger.get_pubkey(0, false)?;
    ledger.get_pubkey(1, false)?;
    ledger.get_pubkey(2, false)?;

    Ok(())
}
