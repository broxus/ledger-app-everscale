use std::borrow::Cow;
use std::str::FromStr;

use base64::Engine;
use ed25519_dalek::{Verifier, SIGNATURE_LENGTH};
use everscale_ledger_wallet::ledger::{SignMode, SignTransactionMeta, WalletType};
use everscale_ledger_wallet::remote_wallet::RemoteWallet;
use nekoton::core::models::Expiration;
use nekoton::core::ton_wallet::wallet_v3v4::{InitData, WalletVersion};
use nekoton::core::ton_wallet::Gift;
use nekoton::core::utils::make_labs_unsigned_message;
use nekoton_abi::{BigUint128, MessageBuilder};
use nekoton_utils::{Clock, SimpleClock, TrustMe};
use serial_test::serial;
use sha2::{Digest, Sha256};
use ton_block::{MsgAddressInt, Serializable};
use ton_types::{AccountId, BuilderData, Cell, IBitstring, SliceData, UInt256};

use crate::utils::{
    get_ledger, DEFAULT_EXPIRATION_TIMEOUT, EVER_DECIMALS, EVER_TICKER, USDT_DECIMALS, USDT_TICKER,
};

mod utils;

const TL_TAG_SIGNATURE_DOMAIN: u32 = 0x71b34ee1;

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
    let (message_hash, boc) = fake_sign_and_extract_boc(&*unsigned_message)?;

    let global_id: i32 = 42;
    let meta = SignTransactionMeta::new(SignMode::SignatureDomain(global_id as u32), None, None);

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;

    verify_signature_domain(&public_key, global_id, &message_hash, &signature);

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
    let (message_hash, boc) = fake_sign_and_extract_boc(&*unsigned_message)?;

    let global_id: i32 = 42;
    let meta = SignTransactionMeta::new(SignMode::SignatureId(global_id as u32), None, None);

    let signature =
        ledger.sign_transaction(account, wallet_type, USDT_DECIMALS, USDT_TICKER, meta, &boc)?;

    let mut to_verify = global_id.to_be_bytes().to_vec();
    to_verify.extend_from_slice(&message_hash);
    assert!(public_key.verify(&to_verify, &signature).is_ok());

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
    // Multisig with 15 custodians (too large transaction)
    let boc = base64::engine::general_purpose::STANDARD
        .decode("te6ccgECHwEAApQAAmuzuttTXRuI0OTWDTFlZ7FEhWjvr98hhG7NC6AuOtq/lwAAAMrQsOrKM+uyhxXYd8eAAAAHwGACAQAIAAAAAQIDzkAQAwIBIAkEAgEgBgUAQUYLBilvnAvo1CkFGNMQGOqQO13kBQQZKVO/Yx8ei1a5KAIBIAgHAEEULBilvnAvo1CkFGNMQGOqQO13kBQQZKVO/Yx8ei1a5KAAQRAsGKW+cC+jUKQUY0xAY6pA7XeQFBBkpU79jHx6LVrkoAIBIA0KAgEgDAsAQQwsGKW+cC+jUKQUY0zAY6pA7XeQFBBkpU79jHy6LVrAoABBCCwYpb5wL6NQpBRfTEBjqkDtd5AUEGSlTv2MfLotWsCgAgEgDw4AQQQsGKW+cC+jUKQUX0yAY6pA7XeQFBBkpU79jHy6LVrAoABBJuwYpb5wL6NQpBRfTMBjqkDtd5AUEGSlTv2MfLotWsCgAgEgGBECASAVEgIBIBQTAEEi7BilvnAvo1CkFF9MwGOoQO13kBQQZKVO/Yx8ui1awKAAQR7sGKW+cC+jUKQUX0zAY6oA7XeQFBBkpU79jHy6LVrAoAIBIBcWAEEa7BilvnAvo1CkFF9MwGOoQO13kBQQZKVO/Yx8ui1awuAAQRbsGKW+cC+jUKQUX0zAY6pA7XeQFBBkpU79jHy6LVrC4AIBIBwZAgEgGxoAQRLsGKW+cC+jUKQUX0zAY6pA7XeQFBBkpU79jHy6LVrC4ABBDuwYpb5wL6NQpBRfTMBjqkDtd5AUEGSlTv2MfLotWsLgAgEgHh0AQQrsGKW+cC+jUKQUX0zAY6pA7XeQFBBkpU79jHy6LVrC4ABBBuwYpb5wL6NQpBRfTMBjqkDtd5AUEGSlTv2MfLotWsLg")?;

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    let meta = SignTransactionMeta::default();

    let res = ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc);
    assert!(res.is_err()); // Invalid contract

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_wallet_v5r1_send_transaction() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::WalletV5R1;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    // Transfer parameters
    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;

    let gift = Gift {
        flags: 3,
        bounce: true,
        destination,
        amount: 123_456_785_012_345_678,
        body: None,
        state_init: None,
    };

    // Build V5R1 payload directly (no ABI — V5R1 uses binary format)
    let init_data = nekoton::core::ton_wallet::wallet_v5r1::make_init_data(&public_key);

    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);
    let (hash, payload) =
        init_data.make_transfer_payload(vec![gift], expiration.timestamp(&SimpleClock), false)?;

    let cell = payload.into_cell()?;
    let boc = ton_types::serialize_toc(&cell)?;

    let global_id: i32 = 42;
    let meta = SignTransactionMeta::new(SignMode::SignatureDomain(global_id as u32), None, None);

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;

    verify_signature_domain(&public_key, global_id, hash.as_slice(), &signature);

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_wallet_v3r1_send_transaction() -> anyhow::Result<()> {
    ledger_sign_wallet_v4_send_transaction(WalletType::WalletV3R1, WalletVersion::V3R1)
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_wallet_v3r2_send_transaction() -> anyhow::Result<()> {
    ledger_sign_wallet_v4_send_transaction(WalletType::WalletV3R2, WalletVersion::V3R2)
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_wallet_v4r1_send_transaction() -> anyhow::Result<()> {
    ledger_sign_wallet_v4_send_transaction(WalletType::WalletV4R1, WalletVersion::V4R1)
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_wallet_v4r2_send_transaction() -> anyhow::Result<()> {
    ledger_sign_wallet_v4_send_transaction(WalletType::WalletV4R2, WalletVersion::V4R2)
}

const TON_WALLET_ID: i32 = 0x29A9A317;

fn ledger_sign_wallet_v4_send_transaction(
    wallet_type: WalletType,
    version: WalletVersion,
) -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    // Transfer parameters
    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;

    let gift = Gift {
        flags: 3,
        bounce: true,
        destination,
        amount: 123_456_785_012_345_678,
        body: None,
        state_init: None,
    };

    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let init_data = InitData::from_key(&public_key).with_subwallet_id(TON_WALLET_ID);
    let (hash, payload) =
        init_data.make_transfer_payload(vec![gift], expiration.timestamp(&SimpleClock), version)?;

    let cell = payload.into_cell()?;
    let boc = ton_types::serialize_toc(&cell)?;

    let meta = SignTransactionMeta::default();

    let signature =
        ledger.sign_transaction(account, wallet_type, EVER_DECIMALS, EVER_TICKER, meta, &boc)?;

    assert!(public_key.verify(hash.as_slice(), &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_ever_wallet_jetton_transfer() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::EverWallet;

    let public_key = ledger.get_pubkey(account, false)?;

    let address_bytes = ledger.get_address(account, wallet_type, false)?;
    let address = MsgAddressInt::with_standart(
        None,
        ton_block::BASE_WORKCHAIN_ID as i8,
        AccountId::from(UInt256::from_be_bytes(&address_bytes)),
    )?;

    let message = ton_block::Message::with_ext_in_header(ton_block::ExternalInboundMessageHeader {
        dst: address,
        ..Default::default()
    });

    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let response_destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let jetton_body =
        build_jetton_transfer_body(1_000_000_000, &destination, &response_destination)?;

    let dest_contract = MsgAddressInt::from_str(
        "0:af212b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let flags: u8 = 3;
    let bounce = true;
    let amount: u64 = 100_000_000;
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let (function, input) =
        MessageBuilder::new(nekoton_contracts::wallets::ever_wallet::send_transaction())
            .arg(dest_contract)
            .arg(BigUint128(amount.into()))
            .arg(bounce)
            .arg(flags)
            .arg(jetton_body)
            .build();

    let unsigned_message = make_labs_unsigned_message(
        &SimpleClock,
        message,
        expiration,
        &public_key,
        Cow::Borrowed(function),
        input,
    )?;

    let (message_hash, boc) = fake_sign_and_extract_boc(&*unsigned_message)?;

    let global_id: i32 = 42;
    let meta = SignTransactionMeta::new(SignMode::SignatureDomain(global_id as u32), None, None);

    let signature =
        ledger.sign_transaction(account, wallet_type, USDT_DECIMALS, USDT_TICKER, meta, &boc)?;

    verify_signature_domain(&public_key, global_id, &message_hash, &signature);

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
#[test]
#[serial]
fn ledger_sign_wallet_v5r1_jetton_transfer() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::WalletV5R1;

    let public_key = ledger.get_pubkey(account, false)?;

    let destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let response_destination = MsgAddressInt::from_str(
        "0:df112b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;
    let jetton_body =
        build_jetton_transfer_body(1_000_000_000, &destination, &response_destination)?;

    let dest_contract = MsgAddressInt::from_str(
        "0:af212b59eb82792623575194c60d2f547c68d54366644a3a5e02b8132f3c4c56",
    )?;

    let gift = Gift {
        flags: 3,
        bounce: true,
        destination: dest_contract,
        amount: 100_000_000,
        body: Some(SliceData::load_cell(jetton_body)?),
        state_init: None,
    };

    let init_data = nekoton::core::ton_wallet::wallet_v5r1::make_init_data(&public_key);
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);
    let (hash, payload) =
        init_data.make_transfer_payload(vec![gift], expiration.timestamp(&SimpleClock), false)?;

    let cell = payload.into_cell()?;
    let boc = ton_types::serialize_toc(&cell)?;

    let global_id: i32 = 42;
    let meta = SignTransactionMeta::new(SignMode::SignatureDomain(global_id as u32), None, None);

    let signature =
        ledger.sign_transaction(account, wallet_type, USDT_DECIMALS, USDT_TICKER, meta, &boc)?;

    verify_signature_domain(&public_key, global_id, hash.as_slice(), &signature);

    Ok(())
}

fn fake_sign_and_extract_boc(
    unsigned_message: &dyn nekoton::crypto::UnsignedMessage,
) -> anyhow::Result<(Vec<u8>, Vec<u8>)> {
    let hash = unsigned_message.hash().to_vec();

    let fake_sig: nekoton::crypto::Signature = [0_u8; 64];
    let signed = unsigned_message.sign(&fake_sig)?;

    let mut data = signed.message.body().trust_me();
    data.get_next_bit()?; // ABI has-signature bit
    data.move_by(SIGNATURE_LENGTH * 8)?; // zero signature

    let cell = data.into_cell();

    Ok((hash, ton_types::serialize_toc(&cell)?))
}

fn verify_signature_domain(
    public_key: &ed25519_dalek::PublicKey,
    global_id: i32,
    message_hash: &[u8],
    signature: &ed25519_dalek::Signature,
) {
    let mut tl_buffer = Vec::new();
    tl_buffer.extend_from_slice(&TL_TAG_SIGNATURE_DOMAIN.to_le_bytes());
    tl_buffer.extend_from_slice(&global_id.to_le_bytes());

    let prefix = Sha256::digest(&tl_buffer);

    let mut to_verify = prefix.to_vec();
    to_verify.extend_from_slice(message_hash);

    assert!(public_key.verify(&to_verify, signature).is_ok());
}

const JETTON_TRANSFER_OPCODE: u32 = 0x0f8a7ea5;

fn build_jetton_transfer_body(
    amount: u128,
    destination: &MsgAddressInt,
    response_destination: &MsgAddressInt,
) -> anyhow::Result<Cell> {
    let mut builder = BuilderData::new();

    // Opcode
    builder.append_u32(JETTON_TRANSFER_OPCODE)?;

    // Query id
    builder.append_u64(SimpleClock.now_ms_u64())?;

    // Amount
    ton_block::Grams::new(amount)?.write_to(&mut builder)?;

    // Recipient
    destination.write_to(&mut builder)?;

    // Response destination
    response_destination.write_to(&mut builder)?;

    // Custom payload
    builder.append_bit_zero()?;

    // Callback value
    let grams = ton_block::Grams::new(0)?;
    grams.write_to(&mut builder)?;

    // Callback payload
    let callback_payload = Cell::default();
    builder.append_bit_zero()?;
    let callback_builder = BuilderData::from_cell(&callback_payload);
    builder.append_builder(&callback_builder)?;

    Ok(builder.into_cell()?)
}
