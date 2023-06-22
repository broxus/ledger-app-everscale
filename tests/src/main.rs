use std::borrow::Cow;
use std::str::FromStr;
use std::sync::Arc;

use ed25519_dalek::{PublicKey, Verifier, SIGNATURE_LENGTH};

use everscale_ledger_wallet::ledger::{LedgerWallet, WalletType};
use everscale_ledger_wallet::locator::Manufacturer;
use everscale_ledger_wallet::remote_wallet::{initialize_wallet_manager, RemoteWallet};
use nekoton::core::models::Expiration;
use nekoton::core::ton_wallet::wallet_v3::InitData;
use nekoton::core::ton_wallet::Gift;
use nekoton::core::utils::make_labs_unsigned_message;
use nekoton::crypto::Signature;
use nekoton_abi::{BigUint128, MessageBuilder};
use nekoton_utils::{SimpleClock, TrustMe};
use ton_block::MsgAddressInt;
use ton_types::{AccountId, UInt256};

const EVER_DECIMALS: u8 = 9;
const EVER_TICKER: &str = "EVER";

const USDT_DECIMALS: u8 = 9;
const USDT_TICKER: &str = "USDT";

const WALLET_ID: u32 = 0x4BA92D8A;

const DEFAULT_EXPIRATION_TIMEOUT: u32 = 60; // sec

fn get_ledger() -> (Arc<LedgerWallet>, PublicKey) {
    let wallet_manager = initialize_wallet_manager().expect("Couldn't start wallet manager");

    // Update device list
    const NO_DEVICE_HELP: &str = "No Ledger found, make sure you have a unlocked Ledger connected with the Ledger Wallet Everscale running";
    wallet_manager.update_devices().expect(NO_DEVICE_HELP);
    assert!(
        !wallet_manager.list_devices().is_empty(),
        "{}",
        NO_DEVICE_HELP
    );

    // Fetch the device path and base pubkey of a connected ledger device
    let (base_pubkey, device_path) = wallet_manager
        .list_devices()
        .iter()
        .find(|d| d.manufacturer == Manufacturer::Ledger)
        .cloned()
        .map(|d| (d.pubkey, d.host_device_path))
        .expect("No ledger device detected");

    let ledger = wallet_manager.get_ledger(&device_path).expect("get device");

    (ledger, base_pubkey)
}

fn test_ledger_pubkey() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let pubkey = ledger.get_pubkey(0, false)?;
    assert_eq!(
        hex::encode(pubkey.as_bytes()),
        "6775b6a6ba3711a1c9ac1a62cacf62890ad1df5fbe4308dd9a17405c75b57f2e"
    );

    let pubkey = ledger.get_pubkey(1, false)?;
    assert_eq!(
        hex::encode(pubkey.as_bytes()),
        "874dbbeb87f22f7c687cf5bfc33dc103d78c4a9c1e63dde53869202458ca7009"
    );

    let pubkey = ledger.get_pubkey(2, false)?;
    assert_eq!(
        hex::encode(pubkey.as_bytes()),
        "3ca0c86c268d5e27f80e234cb66dc811afcd277f5882c3208466e1f3c9395b87"
    );

    Ok(())
}

fn test_ledger_address() -> anyhow::Result<()> {
    let (ledger, _) = get_ledger();

    let wallet_v3 = ledger.get_address(0, WalletType::WalletV3, false)?;
    assert_eq!(
        hex::encode(wallet_v3),
        "ed7439e12d67d23fcaf701ff3bd4e30d390c1e8e14f6f40d52089590e28d9c70"
    );

    let ever_wallet = ledger.get_address(0, WalletType::EverWallet, false)?;
    assert_eq!(
        hex::encode(ever_wallet),
        "3b94dd326f32f5ab14caef0a61d23e716271b20d7e273fc315ea3cfd0023c431"
    );

    let safe_multisig = ledger.get_address(0, WalletType::SafeMultisig, false)?;
    assert_eq!(
        hex::encode(safe_multisig),
        "aafa193fdf6c11cd20a0831ae2a33f7ff4a5add95db7b7b30e7ceef6538e2621"
    );

    let safe_multisig_24 = ledger.get_address(0, WalletType::SafeMultisig24h, false)?;
    assert_eq!(
        hex::encode(safe_multisig_24),
        "b4f9941d96904c22613581a4d905051f37ef41c5c0b995a60d5ebfc254e57a1a"
    );

    let setcode_multisig = ledger.get_address(0, WalletType::SetcodeMultisig, false)?;
    assert_eq!(
        hex::encode(setcode_multisig),
        "7c75e3bff88ec399edc5ee3a31189ccff6fd564ad2708f3e7208d5c899077f9a"
    );

    let bridge_multisig = ledger.get_address(0, WalletType::BridgeMultisig, false)?;
    assert_eq!(
        hex::encode(bridge_multisig),
        "95daf2ffc6c780ca4d4ef63495cf86f8a5f011d3e9fa10edb462ecdc64275136"
    );

    let surf = ledger.get_address(0, WalletType::Surf, false)?;
    assert_eq!(
        hex::encode(surf),
        "a1297485df8e1608109ef009b02fab5668d16b6eec7f8c763bd4ec6474be40c5"
    );

    let multisig2 = ledger.get_address(0, WalletType::Multisig2, false)?;
    assert_eq!(
        hex::encode(multisig2),
        "2bb06296f9c0be8d4290517d33018ea903b5de40504192953bf631f2e8b56b0b"
    );

    let multisig2_1 = ledger.get_address(0, WalletType::Multisig2_1, false)?;
    assert_eq!(
        hex::encode(multisig2_1),
        "bcac3b0b6d2b65b29b18c48b72f76eed1d8dfc86b462086e2731948f1a2550b8"
    );

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
fn test_ledger_sign_message() -> anyhow::Result<()> {
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

    let signature = ledger.sign_message_hash(account, hash.as_slice(), None)?;
    assert!(public_key.verify(hash.as_slice(), &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
fn test_ledger_sign_send_transaction() -> anyhow::Result<()> {
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
    let body: ton_types::Cell = ton_types::deserialize_tree_of_cells(&mut base64::decode("te6ccgEBAwEAYAABa0ap1+wAAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AEBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJisgCAAA=")?.as_slice())?;

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
    let signed = unsigned_message.sign_with_pruned_payload(&signature, 2)?;

    // Extract message body
    let mut data = signed.message.body().trust_me();

    let bit = data.get_next_bit()?;
    assert!(bit);

    // Strip empty signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let cell = data.into_cell();
    let boc = ton_types::serialize_toc(&cell)?;

    let signature = ledger.sign_transaction(
        account,
        wallet_type,
        EVER_DECIMALS,
        EVER_TICKER,
        None,
        None,
        None,
        &boc,
    )?;
    assert!(public_key.verify(message_hash, &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
fn test_ledger_sign_confirm_transaction() -> anyhow::Result<()> {
    // BAD
    let boc = base64::decode("te6ccgEBAQEAOwAAcfDmnGpQVUZxL24fHgfUfLGp2/wzR+YWmZukQraxETyqAAAAxHF5r4KyShssDVOgdrJKGvzpVh6gwA==")?;

    // GOOD
    //let boc = base64::decode("te6ccgEBAQEAOwAAcbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGVwt0ySiJcDVOgdrJKDeYh5nYAwA==")?;
    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let signature = ledger.sign_transaction(
        account,
        wallet_type,
        EVER_DECIMALS,
        EVER_TICKER,
        None,
        None,
        None,
        &boc,
    )?;
    assert!(public_key.verify(message_hash.as_slice(), &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
fn test_ledger_sign_submit_transaction() -> anyhow::Result<()> {
    let boc = base64::decode("te6ccgEBBQEAyQABYbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGlca+ySiZfiY7BZsABAWOAG+Ilaz1wTyTEauoymMGl6o+NGqhszIlHS8BXAmXniYrAAAAAAAAAAAA202lAb5VWNAIBa0ap1+wAAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AMBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJisgEAAA=")?;
    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let signature = ledger.sign_transaction(
        account,
        wallet_type,
        USDT_DECIMALS,
        USDT_TICKER,
        None,
        None,
        None,
        &boc,
    )?;
    assert!(public_key.verify(message_hash.as_slice(), &signature).is_ok());

    Ok(())
}

// This test requires interactive approval of message signing on the ledger.
fn test_ledger_sign_burn_transaction() -> anyhow::Result<()> {
    let boc = base64::decode("te6ccgEBBQEAyQABYbO621NdG4jQ5NYNMWVnsUSFaO+v3yGEbs0LoC462r+XAAAAxHGxjJEySil5CY7BZsABAWOAG+Ilaz1wTyTEauoymMGl6o+NGqhszIlHS8BXAmXniYrAAAAAAAAAAAA202lAb5VWNAIBa1YlSK0AAAAAAAAAAAAAAABJUE+AgBXkJWs9cE8kxGrqMpjBpeqPjRqobMyJR0vAVwJl54mK0AMBQ4AX5CVrPXBPJMRq6jKYwaXqj40aqGzMiUdLwFcCZeeJitAEAAA=")?;

    let cell = ton_types::deserialize_tree_of_cells(&mut boc.as_slice())?;

    let message_hash = cell.repr_hash();

    let (ledger, _) = get_ledger();

    let account = 0;
    let wallet_type = WalletType::SafeMultisig;

    // Get public key
    let public_key = ledger.get_pubkey(account, false)?;

    let signature = ledger.sign_transaction(
        account,
        wallet_type,
        USDT_DECIMALS,
        USDT_TICKER,
        None,
        None,
        None,
        &boc,
    )?;
    assert!(public_key.verify(message_hash.as_slice(), &signature).is_ok());

    Ok(())
}

fn main() {
    if let Err(e) = do_run_tests() {
        Err(e).unwrap()
    }
}

macro_rules! run {
    ($test:ident) => {
        println!(" >>> Running {} <<<", stringify!($test));
        $test()?;
    };
}

fn do_run_tests() -> anyhow::Result<()> {
    run!(test_ledger_pubkey);
    run!(test_ledger_address);
    run!(test_ledger_sign_message);
    run!(test_ledger_sign_send_transaction);
    run!(test_ledger_sign_confirm_transaction);
    run!(test_ledger_sign_submit_transaction);
    run!(test_ledger_sign_burn_transaction);

    Ok(())
}
