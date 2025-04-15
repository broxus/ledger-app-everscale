use std::rc::Rc;

use ed25519_dalek::PublicKey;
use everscale_ledger_wallet::ledger::LedgerWallet;
use everscale_ledger_wallet::locator::Manufacturer;
use everscale_ledger_wallet::remote_wallet::initialize_wallet_manager;

#[allow(dead_code)]
pub const WALLET_ID: u32 = 0x4BA92D8A;

#[allow(dead_code)]
pub const EVER_DECIMALS: u8 = 9;
#[allow(dead_code)]
pub const EVER_TICKER: &str = "EVER";

#[allow(dead_code)]
pub const USDT_DECIMALS: u8 = 9;
#[allow(dead_code)]
pub const USDT_TICKER: &str = "USDT";

#[allow(dead_code)]
pub const DEFAULT_EXPIRATION_TIMEOUT: u32 = 60; // sec

pub fn get_ledger() -> (Rc<LedgerWallet>, PublicKey) {
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
