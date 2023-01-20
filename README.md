![Build Status](https://github.com/broxus/ledger-app-everscale/actions/workflows/ci.yml/badge.svg?branch=master)

# Everscale app for Ledger Wallet

## Overview

This app adds support for the Everscale tokens to Ledger hardware wallets.

Current Features:
- Pubkey queries
- Address queries
- Sign transaction hash
- Parse, display and sign Everscale transaction 

## Prerequisites
### For building the app
* [Install Docker](https://docs.docker.com/get-docker/)
* For Linux hosts, install the Ledger [udev rules](https://github.com/LedgerHQ/udev-rules)
#### Build the [Ledger App Builder](https://developers.ledger.com/docs/nano-app/build/) Docker image
1. Clone the git repository
```
git clone https://github.com/LedgerHQ/ledger-app-builder.git
```
2. Change directories
```
cd ledger-app-builder
```
3. Build the image
```
docker build -t ledger-app-builder:latest .
```
* If permissions errors are encountered, ensure that your user is in the `docker`
  group and that the session has been restarted

4. Compile your app in the container

    In the source folder of your application, for Nano S:

    ```bash
    $ docker run --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
    root@656be163fe84:/app# make
    ```
    For Nano X, specify the `BOLOS_SDK` environment variable before building your app:

    ```bash
    $ docker run --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
    root@656be163fe84:/app# BOLOS_SDK=$NANOX_SDK make
    ```

    For Nano S+, specify the `BOLOS_SDK` environment variable before building your app:
    
    ```bash
    $ docker run --rm -ti -v "$(realpath .):/app" ledger-app-builder:latest
    root@656be163fe84:/app# BOLOS_SDK=$NANOSP_SDK make

5. Load app
* To load application on Ledger see guide for [linux](https://developers.ledger.com/docs/embedded-app/load-linux/) or [macos](https://developers.ledger.com/docs/embedded-app/load-mac/).

## Example of Ledger wallet functionality

**Install Rust to compile client to interact with Everscale Ledger app**
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

**Build client**
```bash
cargo build --release --manifest-path client/Cargo.toml && \
cd ./client/target/release
```

**List of Everscale wallets**
```bash
./client get-wallets
```

**Request public key**
```bash
./client get-pubkey --account ${LEDGER_ACCOUNT_NUMBER}
```

```bash
# Example
./client get-pubkey --account 0
```

**Request address**
```bash
./client get-address --account ${LEDGER_ACCOUNT_NUMBER} --wallet ${WALLET_TYPE}
# send some EVER's to the this address (about 1 should be enough)
```

```bash
# Example
./client get-address --account 0 --wallet EverWallet
```

**Check balance**
```bash
./client get-address --account ${LEDGER_ACCOUNT_NUMBER} --wallet ${WALLET_TYPE}
```

```bash
# Example
./client get-balance --account 0 --wallet EverWallet
```

**Deploy wallet**

It only applies to Multisig wallets
```bash
./client deploy --account ${LEDGER_ACCOUNT_NUMBER} -wallet ${WALLET_TYPE}
```

```bash
# Example
./client deploy --account 0 --wallet Multisig2_1
```

**Send EVER transaction**
```bash
./client send-transaction \
    --account 0 \
    --wallet  EverWallet \
    --amount  0.1 \
    --address 0:7094fc3cb69fa1b7bde8e830e2cd74bc9455d93561ce2c562182215686eb45e2
```

./client send-transaction --account 0 --wallet Multisig2_1 --amount  0.1 --address 0:7094fc3cb69fa1b7bde8e830e2cd74bc9455d93561ce2c562182215686eb45e2

```bash
Example
./client send-transaction \
    --account ${LEDGER_ACCOUNT_NUMBER} \
    --wallet  ${WALLET_TYPE} \
    --amount  ${AMOUNT} \
    --address ${RECIPIENT_ADDRESS}
```

**Get list of supported tokens**
```bash
./client get-tokens
```

**Check token balance**
```bash
./client get-address --account ${LEDGER_ACCOUNT_NUMBER} --wallet ${WALLET_TYPE} --token ${TOKEN_NAME}
```

```bash
# Example
./client get-balance --account 0 --wallet EverWallet --token WEVER
```

**Send token transaction**
```bash
./client send-token-transaction \
    --account ${LEDGER_ACCOUNT_NUMBER} \
    --wallet  ${WALLET_TYPE} \
    --amount  ${AMOUNT} \
    --address ${RECIPIENT_ADDRESS} \
    --token   ${TOKEN_NAME}
```

```bash
# Example
./client send-token-transaction \
    --account 0 \
    --wallet  EverWallet \
    --amount  1.5 \
    --address 0:ed7439e12d67d23fcaf701ff3bd4e30d390c1e8e14f6f40d52089590e28d9c70 \
    --token   WEVER
```

## Test
### Integration
Some tests require interactive approval on the ledger
```bash
cargo run --manifest-path tests/Cargo.toml
```

## Documentation
This follows the specification available in the [`api.md`](doc/api.md)
