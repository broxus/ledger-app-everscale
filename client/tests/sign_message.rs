use std::str::FromStr;

use ed25519_dalek::Verifier;
use everscale_ledger_wallet::ledger::SIGN_MAGIC;
use everscale_ledger_wallet::remote_wallet::RemoteWallet;
use nekoton::core::models::Expiration;
use nekoton::core::ton_wallet::wallet_v3::InitData;
use nekoton::core::ton_wallet::Gift;
use nekoton_utils::SimpleClock;
use ton_block::MsgAddressInt;

use crate::utils::{get_ledger, DEFAULT_EXPIRATION_TIMEOUT, WALLET_ID};

mod utils;

// This test requires interactive approval of message signing on the ledger.
#[test]
fn ledger_sign_message() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    // Transfer parameters
    let flags = 3;
    let bounce = true;
    let amount = 123_456_785_012_345_678;
    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;

    let gift = Gift {
        flags,
        bounce,
        destination,
        amount,
        body: None,
        state_init: None,
    };
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let init_data = InitData::from_key(&public_key).with_wallet_id(WALLET_ID);
    let (hash, _) =
        init_data.make_transfer_payload(vec![gift], expiration.timestamp(&SimpleClock))?;

    let signature = ledger.sign_message(account, hash.as_slice())?;

    let mut message = Vec::with_capacity(SIGN_MAGIC.len() + hash.as_slice().len());
    message.extend(SIGN_MAGIC.to_vec());
    message.extend(hash.into_vec());

    assert!(public_key.verify(message.as_slice(), &signature).is_ok());

    Ok(())
}
