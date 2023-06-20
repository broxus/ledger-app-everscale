use bigdecimal::BigDecimal;
use std::str::FromStr;
use std::sync::Arc;

use bigdecimal::num_bigint::ToBigInt;
use clap::Parser;
use ed25519_dalek::{PublicKey, SIGNATURE_LENGTH};
use everscale_ledger_wallet::ledger::{LedgerWallet, WalletType};
use everscale_ledger_wallet::locator::Manufacturer;
use everscale_ledger_wallet::remote_wallet::{initialize_wallet_manager, RemoteWallet};
use nekoton::core::models::{Expiration, TokenWalletVersion};
use nekoton::core::token_wallet::{RootTokenContractState, TokenWalletContractState};
use nekoton::core::ton_wallet::{Gift, MultisigType, TransferAction, DEFAULT_WORKCHAIN};
use nekoton::crypto::UnsignedMessage;
use nekoton::transport::models::ExistingContract;
use nekoton_abi::num_bigint::BigUint;
use nekoton_abi::{BigUint128, MessageBuilder};
use nekoton_contracts::tip3_1;
use nekoton_utils::{SimpleClock, TrustMe};
use rust_decimal::prelude::{FromPrimitive, ToPrimitive};
use rust_decimal::Decimal;
use ton_block::{AccountState, GetRepresentationHash, MsgAddressInt};
use ton_types::{AccountId, SliceData, UInt256};
use url::Url;

const EVER_DECIMALS: u8 = 9;
const EVER_TICKER: &str = "EVER";

const DEFAULT_EXPIRATION_TIMEOUT: u32 = 60; // sec
const INITIAL_BALANCE: u64 = 100_000_000; // 0.1 EVER
const ATTACHED_AMOUNT: u64 = 500_000_000; // 0.5 EVER

const RPC_ENDPOINT: &str = "https://extension-api.broxus.com/rpc";

#[derive(clap::Parser)]
struct Args {
    #[command(subcommand)]
    command: SubCommand,
}

#[derive(clap::Subcommand)]
enum SubCommand {
    /// Get public key
    GetPubkey {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        /// Confirm key
        #[clap(long, short, action, default_value_t = false)]
        confirm: bool,
    },

    /// Get address
    GetAddress {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        #[arg(short, long, default_value_t = ("EverWallet").to_string())]
        /// Wallet type
        wallet: String,

        /// Confirm key
        #[clap(long, short, action, default_value_t = false)]
        confirm: bool,
    },

    /// Get balance
    GetBalance {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        #[arg(short, long, default_value_t = ("EverWallet").to_string())]
        /// Wallet type
        wallet: String,
    },

    /// Send transaction
    SendTransaction {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        #[arg(short, long, default_value_t = ("EverWallet").to_string())]
        /// Wallet type
        wallet: String,

        #[arg(long)]
        /// Amount to send
        amount: String,

        #[arg(long)]
        /// Destination address
        address: String,
    },

    /// Get balance
    GetTokenBalance {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        #[arg(short, long, default_value_t = ("EverWallet").to_string())]
        /// Wallet type
        wallet: String,

        #[arg(short, long)]
        /// Token name (WEVER, USDT, USDC, DAI)
        token: String,
    },

    /// Send token transaction
    SendTokenTransaction {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        #[arg(short, long, default_value_t = ("EverWallet").to_string())]
        /// Wallet type
        wallet: String,

        #[arg(long)]
        /// Amount to send
        amount: String,

        #[arg(long)]
        /// Destination address
        address: String,

        #[arg(short, long)]
        /// Token name (WEVER, USDT, USDC, DAI)
        token: String,
    },

    /// Deploy wallet
    Deploy {
        #[arg(short, long, default_value_t = 0)]
        /// Number of account on Ledger
        account: u32,

        #[arg(short, long, default_value_t = ("EverWallet").to_string())]
        /// Wallet type
        wallet: String,
    },

    /// List of Everscale Wallets
    GetWallets,

    /// List of tokens
    GetTokens,
}

struct TokenDetails<'a> {
    ticker: &'a str,
    decimals: u8,
    factor: BigDecimal,
    root: MsgAddressInt,
}

enum Token {
    Usdt,
    Usdc,
    Dai,
    Wever,
}

impl FromStr for Token {
    type Err = anyhow::Error;

    fn from_str(input: &str) -> Result<Token, Self::Err> {
        match input {
            "WEVER" => Ok(Token::Wever),
            "USDT" => Ok(Token::Usdt),
            "USDC" => Ok(Token::Usdc),
            "DAI" => Ok(Token::Dai),
            _ => anyhow::bail!("Invalid token"),
        }
    }
}

impl Token {
    fn details(&self) -> TokenDetails {
        match *self {
            Token::Wever => TokenDetails {
                ticker: "WEVER",
                decimals: 9,
                factor: BigDecimal::from_u128(1_000_000_000).trust_me(),
                root: MsgAddressInt::from_str(
                    "0:a49cd4e158a9a15555e624759e2e4e766d22600b7800d891e46f9291f044a93d",
                )
                .trust_me(),
            },
            Token::Usdt => TokenDetails {
                ticker: "USDT",
                decimals: 6,
                factor: BigDecimal::from_u128(1_000_000).trust_me(),
                root: MsgAddressInt::from_str(
                    "0:a519f99bb5d6d51ef958ed24d337ad75a1c770885dcd42d51d6663f9fcdacfb2",
                )
                .trust_me(),
            },
            Token::Usdc => TokenDetails {
                ticker: "USDC",
                decimals: 6,
                factor: BigDecimal::from_u128(1_000_000).trust_me(),
                root: MsgAddressInt::from_str(
                    "0:c37b3fafca5bf7d3704b081fde7df54f298736ee059bf6d32fac25f5e6085bf6",
                )
                .trust_me(),
            },
            Token::Dai => TokenDetails {
                ticker: "DAI",
                decimals: 18,
                factor: BigDecimal::from_u128(1_000_000_000_000_000_000).trust_me(),
                root: MsgAddressInt::from_str(
                    "0:eb2ccad2020d9af9cec137d3146dde067039965c13a27d97293c931dae22b2b9",
                )
                .trust_me(),
            },
        }
    }
}

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

fn prepare_wallet_v3_transfer(
    pubkey: PublicKey,
    amount: u64,
    destination: MsgAddressInt,
    contract: ExistingContract,
    body: Option<SliceData>,
) -> anyhow::Result<(ton_types::Cell, Box<dyn UnsignedMessage>)> {
    let gift = Gift {
        flags: 3,
        bounce: true,
        destination,
        amount,
        body,
        state_init: None,
    };
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let seqno_offset = nekoton::core::ton_wallet::wallet_v3::estimate_seqno_offset(
        &SimpleClock,
        &contract.account,
        &[],
    );

    let action = nekoton::core::ton_wallet::wallet_v3::prepare_transfer(
        &SimpleClock,
        &pubkey,
        &contract.account,
        seqno_offset,
        vec![gift],
        expiration,
    )?;

    let unsigned_message = match action {
        TransferAction::Sign(message) => message,
        TransferAction::DeployFirst => {
            anyhow::bail!("WalletV3 unreachable action")
        }
    };

    // Sign with null signature to extract payload later
    let signed_message = unsigned_message.sign(&[0_u8; 64])?;
    let mut data = signed_message.message.body().trust_me();

    // Skip null signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let payload = data.into_cell();

    Ok((payload, unsigned_message))
}

fn prepare_ever_wallet_transfer(
    pubkey: PublicKey,
    address: MsgAddressInt,
    amount: u64,
    destination: MsgAddressInt,
    contract: ExistingContract,
    body: Option<SliceData>,
) -> anyhow::Result<(ton_types::Cell, Box<dyn UnsignedMessage>)> {
    let gift = Gift {
        flags: 3,
        bounce: true,
        destination,
        amount,
        body,
        state_init: None,
    };
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let action = nekoton::core::ton_wallet::ever_wallet::prepare_transfer(
        &SimpleClock,
        &pubkey,
        &contract.account,
        address,
        vec![gift],
        expiration,
    )?;

    let unsigned_message = match action {
        TransferAction::Sign(message) => message,
        TransferAction::DeployFirst => {
            anyhow::bail!("EverWallet unreachable action")
        }
    };

    // Sign with null signature to extract payload later
    let signed_message = unsigned_message.sign(&[0_u8; 64])?;
    let mut data = signed_message.message.body().trust_me();

    let first_bit = data.get_next_bit()?;
    assert!(first_bit);

    // Skip null signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let payload = data.into_cell();

    Ok((payload, unsigned_message))
}

fn prepare_multisig_wallet_transfer(
    pubkey: PublicKey,
    address: MsgAddressInt,
    amount: u64,
    destination: MsgAddressInt,
    wallet_type: WalletType,
    body: Option<SliceData>,
) -> anyhow::Result<(ton_types::Cell, Box<dyn UnsignedMessage>)> {
    let multisig_type = match wallet_type {
        WalletType::SafeMultisig => MultisigType::SafeMultisigWallet,
        WalletType::SafeMultisig24h => MultisigType::SafeMultisigWallet24h,
        WalletType::SetcodeMultisig => MultisigType::SetcodeMultisigWallet,
        WalletType::BridgeMultisig => MultisigType::BridgeMultisigWallet,
        WalletType::Multisig2 => MultisigType::Multisig2,
        WalletType::Multisig2_1 => MultisigType::Multisig2_1,
        _ => anyhow::bail!("Invalid multisig type"),
    };

    let gift = Gift {
        flags: 3,
        bounce: true,
        destination,
        amount,
        body,
        state_init: None,
    };
    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let action = nekoton::core::ton_wallet::multisig::prepare_transfer(
        &SimpleClock,
        multisig_type,
        &pubkey,
        false,
        address,
        gift,
        expiration,
    )?;

    let unsigned_message = match action {
        TransferAction::Sign(message) => message,
        TransferAction::DeployFirst => {
            anyhow::bail!("EverWallet unreachable action")
        }
    };

    // Sign with null signature to extract payload later
    let signed_message = unsigned_message.sign(&[0_u8; 64])?;
    let mut data = signed_message.message.body().trust_me();

    let first_bit = data.get_next_bit()?;
    assert!(first_bit);

    // Skip null signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let payload = data.into_cell();

    Ok((payload, unsigned_message))
}

fn prepare_multisig_wallet_deploy(
    pubkey: PublicKey,
    wallet_type: WalletType,
) -> anyhow::Result<(ton_types::Cell, Box<dyn UnsignedMessage>)> {
    let multisig_type = match wallet_type {
        WalletType::SafeMultisig => MultisigType::SafeMultisigWallet,
        WalletType::SafeMultisig24h => MultisigType::SafeMultisigWallet24h,
        WalletType::SetcodeMultisig => MultisigType::SetcodeMultisigWallet,
        WalletType::BridgeMultisig => MultisigType::BridgeMultisigWallet,
        WalletType::Multisig2 => MultisigType::Multisig2,
        WalletType::Multisig2_1 => MultisigType::Multisig2_1,
        _ => anyhow::bail!("Invalid multisig type"),
    };

    let expiration = Expiration::Timeout(DEFAULT_EXPIRATION_TIMEOUT);

    let deploy_params = nekoton::core::ton_wallet::multisig::DeployParams {
        owners: &[pubkey],
        req_confirms: 1,
        expiration_time: None,
    };

    let unsigned_message = nekoton::core::ton_wallet::multisig::prepare_deploy(
        &SimpleClock,
        &pubkey,
        multisig_type,
        DEFAULT_WORKCHAIN,
        expiration,
        deploy_params,
    )?;

    // Sign with null signature to extract payload later
    let signed_message = unsigned_message.sign(&[0_u8; 64])?;
    let mut data = signed_message.message.body().trust_me();

    let first_bit = data.get_next_bit()?;
    assert!(first_bit);

    // Skip null signature
    data.move_by(SIGNATURE_LENGTH * 8)?;

    let payload = data.into_cell();

    Ok((payload, unsigned_message))
}

fn prepare_token_body(
    tokens: BigUint,
    owner: &MsgAddressInt,
    destiantion: &MsgAddressInt,
) -> anyhow::Result<SliceData> {
    let payload: ton_types::Cell = Default::default();

    let (function_token, input_token) =
        MessageBuilder::new(tip3_1::token_wallet_contract::transfer())
            .arg(BigUint128(tokens)) // amount
            .arg(destiantion) // recipient owner wallet
            .arg(BigUint128(INITIAL_BALANCE.into())) // deployWalletValue
            .arg(owner) // remainingGasTo
            .arg(false) // notify
            .arg(payload) // payload
            .build();

    SliceData::load_builder(function_token.encode_internal_input(&input_token)?)
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    let (ledger, _) = get_ledger();

    match args.command {
        SubCommand::GetPubkey { account, confirm } => {
            let pubkey = ledger.get_pubkey(account, confirm)?;
            println!("Public key: 0x{}", hex::encode(pubkey.to_bytes()));
        }
        SubCommand::GetAddress {
            account,
            wallet,
            confirm,
        } => {
            let wallet_type = WalletType::from_str(&wallet).map_err(|s| anyhow::anyhow!(s))?;

            let bytes = ledger.get_address(account, wallet_type, confirm)?;
            let address = MsgAddressInt::with_standart(
                None,
                ton_block::BASE_WORKCHAIN_ID as i8,
                AccountId::from(UInt256::from_be_bytes(&bytes)),
            )?;

            println!("Address: {}", address);
        }
        SubCommand::GetBalance { account, wallet } => {
            let wallet_type = WalletType::from_str(&wallet).map_err(|s| anyhow::anyhow!(s))?;

            let bytes = ledger.get_address(account, wallet_type, false)?;
            let address = MsgAddressInt::with_standart(
                None,
                ton_block::BASE_WORKCHAIN_ID as i8,
                AccountId::from(UInt256::from_be_bytes(&bytes)),
            )?;

            let client = everscale_jrpc_client::JrpcClient::new(
                vec![Url::parse(RPC_ENDPOINT)?],
                everscale_jrpc_client::JrpcClientOptions::default(),
            )
            .await?;

            let contract = client.get_contract_state(&address).await?;
            match contract {
                Some(contract) => {
                    let mut balance =
                        Decimal::from_u128(contract.account.storage.balance.grams.as_u128()).trust_me();
                    balance.set_scale(EVER_DECIMALS as u32)?;
                    println!("Balance: {} EVER", balance);
                }
                None => {
                    println!("Account haven't deployed yet");
                }
            }
        }
        SubCommand::SendTransaction {
            account,
            wallet,
            amount,
            address,
        } => {
            let amount = (Decimal::from_str(&amount)? * Decimal::from(1_000_000_000))
                .to_u64()
                .trust_me();
            let destination = MsgAddressInt::from_str(&address)?;

            let pubkey = ledger.get_pubkey(account, false)?;

            let wallet_type = WalletType::from_str(&wallet).map_err(|s| anyhow::anyhow!(s))?;

            let bytes = ledger.get_address(account, wallet_type, false)?;
            let address = MsgAddressInt::with_standart(
                None,
                ton_block::BASE_WORKCHAIN_ID as i8,
                AccountId::from(UInt256::from_be_bytes(&bytes)),
            )?;

            let client = everscale_jrpc_client::JrpcClient::new(
                vec![Url::parse(RPC_ENDPOINT)?],
                everscale_jrpc_client::JrpcClientOptions::default(),
            )
            .await?;

            let contract = client.get_contract_state(&address).await?;
            match contract {
                Some(contract) => match wallet_type {
                    WalletType::WalletV3 => {
                        let (payload, unsigned_message) = prepare_wallet_v3_transfer(
                            pubkey,
                            amount,
                            destination,
                            contract,
                            None,
                        )?;

                        let boc = ton_types::serialize_toc(&payload)?;

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

                        let signed_message =
                            unsigned_message.sign(&nekoton::crypto::Signature::from(signature))?;

                        println!(
                            "Sending message with hash '{}'...",
                            signed_message.message.hash()?.to_hex_string()
                        );

                        let status = client
                            .send_message(
                                signed_message.message,
                                everscale_jrpc_client::SendOptions::default(),
                            )
                            .await?;

                        println!("Send status: {:?}", status);
                    }
                    WalletType::EverWallet => {
                        let (payload, unsigned_message) = prepare_ever_wallet_transfer(
                            pubkey,
                            address.clone(),
                            amount,
                            destination,
                            contract,
                            None,
                        )?;

                        let boc = ton_types::serialize_toc(&payload)?;

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

                        let signed_message =
                            unsigned_message.sign(&nekoton::crypto::Signature::from(signature))?;

                        println!(
                            "Sending message with hash '{}'...",
                            signed_message.message.hash()?.to_hex_string()
                        );

                        let status = client
                            .send_message(
                                signed_message.message,
                                everscale_jrpc_client::SendOptions::default(),
                            )
                            .await?;

                        println!("Send status: {:?}", status);
                    }
                    WalletType::SafeMultisig
                    | WalletType::SafeMultisig24h
                    | WalletType::SetcodeMultisig
                    | WalletType::BridgeMultisig
                    | WalletType::Multisig2
                    | WalletType::Multisig2_1 => {
                        if let AccountState::AccountUninit = contract.account.storage.state {
                            println!("Account haven't deployed yet");
                            return Ok(());
                        }

                        let (payload, unsigned_message) = prepare_multisig_wallet_transfer(
                            pubkey,
                            address.clone(),
                            amount,
                            destination,
                            wallet_type,
                            None,
                        )?;

                        let boc = ton_types::serialize_toc(&payload)?;

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

                        let signed_message =
                            unsigned_message.sign(&nekoton::crypto::Signature::from(signature))?;

                        println!(
                            "Sending message with hash '{}'...",
                            signed_message.message.hash()?.to_hex_string()
                        );

                        let status = client
                            .send_message(
                                signed_message.message,
                                everscale_jrpc_client::SendOptions::default(),
                            )
                            .await?;

                        println!("Send status: {:?}", status);
                    }
                    _ => unimplemented!(),
                },
                None => {
                    println!(
                        "Account state not found. You should send 1 EVER to {}",
                        address
                    );
                }
            }
        }
        SubCommand::GetTokenBalance {
            account,
            wallet,
            token,
        } => {
            let wallet_type = WalletType::from_str(&wallet).map_err(|s| anyhow::anyhow!(s))?;

            let bytes = ledger.get_address(account, wallet_type, false)?;
            let address = MsgAddressInt::with_standart(
                None,
                ton_block::BASE_WORKCHAIN_ID as i8,
                AccountId::from(UInt256::from_be_bytes(&bytes)),
            )?;

            let client = everscale_jrpc_client::JrpcClient::new(
                vec![Url::parse(RPC_ENDPOINT)?],
                everscale_jrpc_client::JrpcClientOptions::default(),
            )
            .await?;

            let contract = client.get_contract_state(&address).await?;
            match contract {
                Some(_) => {
                    let token: Token = Token::from_str(&token)?;
                    let token_details = token.details();

                    let root_contract = client
                        .get_contract_state(&token_details.root)
                        .await?
                        .trust_me();

                    let token_address = RootTokenContractState(&root_contract).get_wallet_address(
                        &SimpleClock,
                        TokenWalletVersion::Tip3,
                        &address,
                    )?;

                    let token_contract = client.get_contract_state(&token_address).await?;
                    match token_contract {
                        Some(token_contract) => {
                            let state = TokenWalletContractState(&token_contract);
                            let balance =
                                state.get_balance(&SimpleClock, TokenWalletVersion::Tip3)?;

                            println!(
                                "Balance: {} {}",
                                BigDecimal::new(
                                    balance.to_bigint().trust_me(),
                                    token_details.decimals as i64
                                ),
                                token_details.ticker
                            );
                        }
                        None => {
                            println!("Token account haven't deployed yet");
                        }
                    }
                }
                None => {
                    println!("Account haven't deployed yet");
                }
            }
        }
        SubCommand::SendTokenTransaction {
            account,
            wallet,
            amount,
            address,
            token,
        } => {
            let token: Token = Token::from_str(&token)?;
            let token_details = token.details();

            let amount = (BigDecimal::from_str(&amount)? * token_details.factor)
                .with_scale(0)
                .into_bigint_and_exponent()
                .0
                .to_biguint()
                .trust_me();

            let destination = MsgAddressInt::from_str(&address)?;

            let pubkey = ledger.get_pubkey(account, false)?;

            let wallet_type = WalletType::from_str(&wallet).map_err(|s| anyhow::anyhow!(s))?;

            let bytes = ledger.get_address(account, wallet_type, false)?;
            let owner = MsgAddressInt::with_standart(
                None,
                ton_block::BASE_WORKCHAIN_ID as i8,
                AccountId::from(UInt256::from_be_bytes(&bytes)),
            )?;

            let client = everscale_jrpc_client::JrpcClient::new(
                vec![Url::parse(RPC_ENDPOINT)?],
                everscale_jrpc_client::JrpcClientOptions::default(),
            )
            .await?;

            let owner_contract = client.get_contract_state(&owner).await?;
            match owner_contract {
                Some(owner_contract) => {
                    let root_contract = client
                        .get_contract_state(&token_details.root)
                        .await?
                        .trust_me();
                    let owner_token = RootTokenContractState(&root_contract).get_wallet_address(
                        &SimpleClock,
                        TokenWalletVersion::Tip3,
                        &owner,
                    )?;

                    let token_body = prepare_token_body(amount, &owner, &destination)?;

                    match wallet_type {
                        WalletType::WalletV3 => {
                            let (payload, unsigned_message) = prepare_wallet_v3_transfer(
                                pubkey,
                                ATTACHED_AMOUNT,
                                owner_token,
                                owner_contract,
                                Some(token_body),
                            )?;

                            let boc = ton_types::serialize_toc(&payload)?;

                            let signature = ledger.sign_transaction(
                                account,
                                wallet_type,
                                token_details.decimals,
                                token_details.ticker,
                                None,
                                None,
                                None,
                                &boc,
                            )?;

                            let signed_message = unsigned_message
                                .sign(&nekoton::crypto::Signature::from(signature))?;

                            println!(
                                "Sending message with hash '{}'...",
                                signed_message.message.hash()?.to_hex_string()
                            );

                            let status = client
                                .send_message(
                                    signed_message.message,
                                    everscale_jrpc_client::SendOptions::default(),
                                )
                                .await?;

                            println!("Send status: {:?}", status);
                        }
                        WalletType::EverWallet => {
                            let (payload, unsigned_message) = prepare_ever_wallet_transfer(
                                pubkey,
                                owner,
                                ATTACHED_AMOUNT,
                                owner_token,
                                owner_contract,
                                Some(token_body),
                            )?;

                            let boc = ton_types::serialize_toc(&payload)?;

                            let signature = ledger.sign_transaction(
                                account,
                                wallet_type,
                                token_details.decimals,
                                token_details.ticker,
                                None,
                                None,
                                None,
                                &boc,
                            )?;

                            let signed_message = unsigned_message
                                .sign(&nekoton::crypto::Signature::from(signature))?;

                            println!(
                                "Sending message with hash '{}'...",
                                signed_message.message.hash()?.to_hex_string()
                            );

                            let status = client
                                .send_message(
                                    signed_message.message,
                                    everscale_jrpc_client::SendOptions::default(),
                                )
                                .await?;

                            println!("Send status: {:?}", status);
                        }
                        WalletType::SafeMultisig
                        | WalletType::SafeMultisig24h
                        | WalletType::SetcodeMultisig
                        | WalletType::BridgeMultisig
                        | WalletType::Multisig2
                        | WalletType::Multisig2_1 => {
                            if let AccountState::AccountUninit =
                                owner_contract.account.storage.state
                            {
                                println!("Owner account haven't deployed yet");
                                return Ok(());
                            }

                            let (payload, unsigned_message) = prepare_multisig_wallet_transfer(
                                pubkey,
                                owner,
                                ATTACHED_AMOUNT,
                                owner_token,
                                wallet_type,
                                Some(token_body),
                            )?;

                            let boc = ton_types::serialize_toc(&payload)?;

                            let signature = ledger.sign_transaction(
                                account,
                                wallet_type,
                                token_details.decimals,
                                token_details.ticker,
                                None,
                                None,
                                None,
                                &boc,
                            )?;

                            let signed_message = unsigned_message
                                .sign(&nekoton::crypto::Signature::from(signature))?;

                            println!(
                                "Sending message with hash '{}'...",
                                signed_message.message.hash()?.to_hex_string()
                            );

                            let status = client
                                .send_message(
                                    signed_message.message,
                                    everscale_jrpc_client::SendOptions::default(),
                                )
                                .await?;

                            println!("Send status: {:?}", status);
                        }
                        _ => unimplemented!(),
                    }
                }
                None => {
                    println!(
                        "Account state not found. You should send 1 EVER to {}",
                        address
                    );
                }
            }
        }
        SubCommand::GetWallets => {
            println!(
                "WalletV3\nEverWallet\nSafeMultisig\nSafeMultisig24h\nSetcodeMultisig\nBridgeMultisig\nMultisig2\nMultisig2_1\nSurf (unimplemented)",
            );
        }
        SubCommand::GetTokens => {
            println!("WEVER\nUSDT\nUSDC\nDAI",);
        }
        SubCommand::Deploy { account, wallet } => {
            let pubkey = ledger.get_pubkey(account, false)?;

            let wallet_type = WalletType::from_str(&wallet).map_err(|s| anyhow::anyhow!(s))?;

            let bytes = ledger.get_address(account, wallet_type, false)?;
            let address = MsgAddressInt::with_standart(
                None,
                ton_block::BASE_WORKCHAIN_ID as i8,
                AccountId::from(UInt256::from_be_bytes(&bytes)),
            )?;

            let client = everscale_jrpc_client::JrpcClient::new(
                vec![Url::parse(RPC_ENDPOINT)?],
                everscale_jrpc_client::JrpcClientOptions::default(),
            )
            .await?;

            let contract = client.get_contract_state(&address).await?;
            match contract {
                Some(contract) => match wallet_type {
                    WalletType::WalletV3 | WalletType::EverWallet => {
                        println!("No need to deploy");
                    }
                    WalletType::SafeMultisig
                    | WalletType::SafeMultisig24h
                    | WalletType::SetcodeMultisig
                    | WalletType::BridgeMultisig
                    | WalletType::Multisig2
                    | WalletType::Multisig2_1 => {
                        if let AccountState::AccountActive { .. } = contract.account.storage.state {
                            println!("Account have already deployed");
                            return Ok(());
                        }

                        let (payload, unsigned_message) =
                            prepare_multisig_wallet_deploy(pubkey, wallet_type)?;

                        let boc = ton_types::serialize_toc(&payload)?;

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

                        let signed_message =
                            unsigned_message.sign(&nekoton::crypto::Signature::from(signature))?;

                        println!(
                            "Sending message with hash '{}'...",
                            signed_message.message.hash()?.to_hex_string()
                        );

                        let status = client
                            .send_message(
                                signed_message.message,
                                everscale_jrpc_client::SendOptions::default(),
                            )
                            .await?;

                        println!("Send status: {:?}", status);
                    }
                    _ => unimplemented!(),
                },
                None => {
                    println!(
                        "Account state not found. You should send 1 EVER to {}",
                        address
                    );
                }
            }
        }
    };

    Ok(())
}
