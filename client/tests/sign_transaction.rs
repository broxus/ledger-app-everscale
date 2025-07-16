use std::borrow::Cow;
use std::str::FromStr;

use base64::Engine;
use ed25519_dalek::{Verifier, SIGNATURE_LENGTH};
use everscale_ledger_wallet::ledger::{SignTransactionMeta, WalletType};
use everscale_ledger_wallet::remote_wallet::RemoteWallet;
use nekoton::core::models::Expiration;
use nekoton::core::utils::make_labs_unsigned_message;
use nekoton::crypto::Signature;
use nekoton_abi::{BigUint128, MessageBuilder};
use nekoton_utils::{SimpleClock, TrustMe};
use serial_test::serial;
use ton_block::MsgAddressInt;
use ton_types::{AccountId, UInt256};

use crate::utils::{
    get_ledger, DEFAULT_EXPIRATION_TIMEOUT, EVER_DECIMALS, EVER_TICKER, USDT_DECIMALS, USDT_TICKER,
};

mod utils;

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_send_transaction() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::EverWallet;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    // Get address
    let address_bytes = ledger.get_address(account, wallet_type, false)?;
    let address = MsgAddressInt::with_standart(
        None,
        ton_block::BASE_WORKCHAIN_ID as i8,
        AccountId::from(UInt256::from_be_bytes(&address_bytes)),
    )?;

    // Prepare message
    let message = ton_block::Message::with_ext_in_header(ton_block::ExternalInboundMessageHeader {
        dst: address,
        ..Default::default()
    });

    // Transfer parameters
    let flags: u8 = 3;
    let bounce = true;
    let amount: u64 = 123_456_785_012_345_678;
    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let body: ton_types::Cell = Default::default();
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let (function, input) =
        MessageBuilder::new(nekoton_contracts::wallets::ever_wallet::send_transaction())
            .arg(destination)
            .arg(BigUint128(amount.into()))
            .arg(bounce)
            .arg(flags)
            .arg(body)
            .build();

    // Create unsigned message
    let unsigned_message = make_labs_unsigned_message(
        &SimpleClock,
        message,
        expiration,
        &public_key,
        Cow::Borrowed(function),
        input,
    )?;
    let message_hash = unsigned_message.hash();

    // Fake sign
    let signature: Signature = [0_u8; 64];
    let signed = unsigned_message.sign(&signature)?;

    // Extract message body
    let mut data = signed.message.body().trust_me();

    let bit = data.get_next_bit()?;
    assert!(bit);

    // Strip empty signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let meta = SignTransactionMeta::default();

    let cell = data.into_cell();
    let boc = ton_types::serialize_toc(&cell)?;

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;
    assert!(public_key.verify(message_hash, &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_send_token_transaction() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    // Get address
    let address_bytes = ledger.get_address(account, wallet_type, false)?;
    let address = MsgAddressInt::with_standart(
        None,
        ton_block::BASE_WORKCHAIN_ID as i8,
        AccountId::from(UInt256::from_be_bytes(&address_bytes)),
    )?;

    // Prepare message
    let message = ton_block::Message::with_ext_in_header(ton_block::ExternalInboundMessageHeader {
        dst: address,
        ..Default::default()
    });

    // Transfer parameters
    let flags: u8 = 3;
    let bounce = true;
    let amount: u64 = 123_456_785_012;
    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let body: ton_types::Cell = ton_types::deserialize_tree_of_cells(&mut base64::engine::general_purpose::STANDARD
        .decode("te6ccgEBAwEAYAABa0ap1+wAAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AEBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJisgCAAA=")?.as_slice())?;

    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let (function, input) =
        MessageBuilder::new(nekoton_contracts::wallets::multisig::send_transaction())
            .arg(destination)
            .arg(BigUint128(amount.into()))
            .arg(bounce)
            .arg(flags)
            .arg(body)
            .build();

    // Create unsigned message
    let unsigned_message = make_labs_unsigned_message(
        &SimpleClock,
        message,
        expiration,
        &public_key,
        Cow::Borrowed(function),
        input,
    )?;
    let message_hash = unsigned_message.hash();

    // Fake sign
    let signature: Signature = [0_u8; 64];
    let signed = unsigned_message.sign(&signature)?;

    // Extract message body
    let mut data = signed.message.body().trust_me();

    let bit = data.get_next_bit()?;
    assert!(bit);

    // Strip empty signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let meta = SignTransactionMeta::default();

    let cell = data.into_cell();
    let boc = ton_types::serialize_toc(&cell)?;

    let signature =
        ledger.sign_transaction(account, wallet_type, USDT_DECIMALS, USDT_TICKER, meta, &boc)?;
    assert!(public_key.verify(message_hash, &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_confirm_transaction() -> anyhow::Result<()> {
    // let boc = base64::decode("te6ccgEBAQEAOwAAcbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGVwt0ySiJcDVOgdrJKDeYh5nYAwA==")?;

    let boc = base64::engine::general_purpose::STANDARD
        .decode("te6ccgEBAQEAOwAAcfDmnGpQVUZxL24fHgfUfLGp2/wzR+YWmZukQraxETyqAAAAxHF5r4KyShssDVOgdrJKGvzpVh6gwA==")?;

    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let meta = SignTransactionMeta::default();

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;
    assert!(public_key
        .verify(message_hash.as_slice(), &signature)
        .is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_submit_transaction() -> anyhow::Result<()> {
    let boc = base64::engine::general_purpose::STANDARD
        .decode("te6ccgEBBQEAyQABYbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGlca+ySiZfiY7BZsABAWOAG+Ilaz1wTyTEauoymMGl6o+NGqhszIlHS8BXAmXniYrAAAAAAAAAAAA202lAb5VWNAIBa0ap1+wAAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AMBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJisgEAAA=")?;
    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let meta = SignTransactionMeta::default();

    let signature =
        ledger.sign_transaction(account, wallet_type, USDT_DECIMALS, USDT_TICKER, meta, &boc)?;
    assert!(public_key
        .verify(message_hash.as_slice(), &signature)
        .is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_burn_transaction() -> anyhow::Result<()> {
    let boc = base64::engine::general_purpose::STANDARD
        .decode("te6ccgEBBQEAyQABYbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGxjJEySil5CY7BZsABAWOAG+Ilaz1wTyTEauoymMGl6o+NGqhszIlHS8BXAmXniYrAAAAAAAAAAAA202lAb5VWNAIBa1YlSK0AAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AMBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJitAEAAA=")?;

    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let meta = SignTransactionMeta::default();

    let signature =
        ledger.sign_transaction(account, wallet_type, USDT_DECIMALS, USDT_TICKER, meta, &boc)?;
    assert!(public_key
        .verify(message_hash.as_slice(), &signature)
        .is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
// Chunks test
#[test]
#[serial]
fn ledger_sign_large_transaction() -> anyhow::Result<()> {
    let boc = base64::engine::general_purpose::STANDARD
        .decode("te6ccgECBgEAASwAIWHw5pxqUFVGcS9uHx4H1Hyxqdv8M0fmFpmbpEK2sRE8qgAAAMVB8+fysn958qZ3MjZAASFlgBpSNW8J55zSsfWhyOd5rDEjZgefz1LcWODp5B1bqgHogAAAAAAAAAAAAAAAJUC+QBA4AiOVEMgafQAACAoJgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACBQQDKEgBAUZsZIPd9xkGAco8YjJOBZgvCE3wlneY/Wx25kfOjumaAAAoSAEB1IDh09lVWihGkWEM9d9w01WXUrJVmfTd7p+dAZMyU/oAAChIAQGQCROE6sQnHzRREUyjfCGGj+o03d7B++Lo06vNLc44JgAA")?;

    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.hash(0); // pruned hash

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let meta = SignTransactionMeta::default();

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;
    assert!(public_key
        .verify(message_hash.as_slice(), &signature)
        .is_ok());

    Ok(())
}

#[test]
#[serial]
fn ledger_many_msig_custodians_transaction() -> anyhow::Result<()> {
    // Multisig with 15 custodians
    let boc = base64::engine::general_purpose::STANDARD
        .decode("te6ccgECHwEAApQAAmuzuttTXRuI0OTWDTFlZ7FEhWjvr98hhG7NC6AuOtq/lwAAAMrQsOrKM+uyhxXYd8eAAAAHwGACAQAIAAAAAQIDzkAQAwIBIAkEAgEgBgUAQUYLBilvnAvo1CkFGNMQGOqQO13kBQQZKVO/Yx8ei1a5KAIBIAgHAEEULBilvnAvo1CkFGNMQGOqQO13kBQQZKVO/Yx8ei1a5KAAQRAsGKW+cC+jUKQUY0xAY6pA7XeQFBBkpU79jHx6LVrkoAIBIA0KAgEgDAsAQQwsGKW+cC+jUKQUY0zAY6pA7XeQFBBkpU79jHy6LVrAoABBCCwYpb5wL6NQpBRfTEBjqkDtd5AUEGSlTv2MfLotWsCgAgEgDw4AQQQsGKW+cC+jUKQUX0yAY6pA7XeQFBBkpU79jHy6LVrAoABBJuwYpb5wL6NQpBRfTMBjqkDtd5AUEGSlTv2MfLotWsCgAgEgGBECASAVEgIBIBQTAEEi7BilvnAvo1CkFF9MwGOoQO13kBQQZKVO/Yx8ui1awKAAQR7sGKW+cC+jUKQUX0zAY6oA7XeQFBBkpU79jHy6LVrAoAIBIBcWAEEa7BilvnAvo1CkFF9MwGOoQO13kBQQZKVO/Yx8ui1awuAAQRbsGKW+cC+jUKQUX0zAY6pA7XeQFBBkpU79jHy6LVrC4AIBIBwZAgEgGxoAQRLsGKW+cC+jUKQUX0zAY6pA7XeQFBBkpU79jHy6LVrC4ABBDuwYpb5wL6NQpBRfTMBjqkDtd5AUEGSlTv2MfLotWsLgAgEgHh0AQQrsGKW+cC+jUKQUX0zAY6pA7XeQFBBkpU79jHy6LVrC4ABBBuwYpb5wL6NQpBRfTMBjqkDtd5AUEGSlTv2MfLotWsLg")?;

    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let meta = SignTransactionMeta::default();

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;
    assert!(public_key
        .verify(message_hash.as_slice(), &signature)
        .is_ok());

    Ok(())
}
