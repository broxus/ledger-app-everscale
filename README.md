<p align="center">
  <a href="https://github.com/venom-blockchain/developer-program">
    <img src="https://raw.githubusercontent.com/venom-blockchain/developer-program/main/vf-dev-program.png" alt="Logo" width="366.8" height="146.4">
  </a>
</p>

![Build Status](https://github.com/broxus/ledger-app-everscale/actions/workflows/ci.yml/badge.svg?branch=master)

# Everscale app for Ledger Wallet

## Overview

This app adds support for the Everscale tokens to Ledger hardware wallets.

Current Features:
- Pubkey queries
- Address queries
- Sign transaction hash
- Parse, display and sign Everscale transaction 

## Build and load application
* [Install Docker](https://docs.docker.com/get-docker/)
* For Linux hosts, install the Ledger [udev rules](https://github.com/LedgerHQ/udev-rules)

1. Pull the default image
```
docker pull ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
```

2. Compile your app in the container
* For Nano S
```bash
$ docker run --rm -ti -v "$(realpath .):/app" --user $(id -u $USER):$(id -g $USER) ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
root@656be163fe84:/app# BOLOS_SDK=$NANOS_SDK make
```

* For Nano X
```bash
$ docker run --rm -ti -v "$(realpath .):/app" --user $(id -u $USER):$(id -g $USER) ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
root@656be163fe84:/app# BOLOS_SDK=$NANOX_SDK make
```

* For Nano S+
```bash
$ docker run --rm -ti -v "$(realpath .):/app" --user $(id -u $USER):$(id -g $USER) ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
root@656be163fe84:/app# BOLOS_SDK=$NANOSP_SDK make
```

3. Code static analysis
The Docker images include the [Clang Static Analyzer](https://clang-analyzer.llvm.org/), which can be invoked with:
```bash
$ docker run --rm -ti -v "$(realpath .):/app" --user $(id -u $USER):$(id -g $USER) ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
root@656be163fe84:/app# BOLOS_SDK=$NANOS_SDK make scan-build
```

4. Load the app on a physical device

:warning: Only Nano S and Nano S+ devices allow application side-loading. This section will not work with a Nano X.

To load the app from the container, you will need additional docker arguments in order to allow Docker to access your USB port.
Your physical device must be connected, unlocked and the screen showing the dashboard (not inside an application). Same as for compilation, `BOLOS_SDK` variable is used to specify the target device. Use the following docker command to load the app (here for Nano S device) :

```bash
$ docker run --rm -ti  -v "$(realpath .):/app" --privileged -v "/dev/bus/usb:/dev/bus/usb" --user $(id -u $USER):$(id -g $USER) ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest
root@656be163fe84:/app# BOLOS_SDK=$NANOS_SDK make load
```

## Fuzzing

* Build docker image
```bash
cd fuzz && docker build -t ledger-app-fuzzer .
```

* Build fuzzer
```bash
docker run --rm -ti -v "$(realpath .):/app" ledger-app-fuzzer:latest
root@72edae7503e3:/# cd /app/fuzz && ./build.sh
```

* Run fuzzer
```bash
root@72edae7503e3:/app/fuzz/cmake-build-fuzz# cd cmake-build-fuzz && mkdir out && ./fuzzer
```

## Example of Ledger wallet functionality

**Install Rust to compile client to interact with Everscale Ledger app**
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

**List of Everscale wallets**
```bash
cargo run --manifest-path client/Cargo.toml get-wallets
```

**Request public key**
```bash
cargo run --manifest-path client/Cargo.toml get-pubkey --account ${LEDGER_ACCOUNT_NUMBER}
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml get-pubkey --account 0
```

**Request address**
```bash
cargo run --manifest-path client/Cargo.toml get-address --account ${LEDGER_ACCOUNT_NUMBER} --wallet ${WALLET_TYPE}
# send some EVER's to the this address (about 1 should be enough)
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml get-address --account 0 --wallet EverWallet
```

**Check balance**
```bash
cargo run --manifest-path client/Cargo.toml get-balance --account ${LEDGER_ACCOUNT_NUMBER} --wallet  ${WALLET_TYPE}
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml get-balance --account 0 --wallet EverWallet
```

**Deploy wallet**
It only applies to Multisig wallets
```bash
cargo run --manifest-path client/Cargo.toml deploy --account ${LEDGER_ACCOUNT_NUMBER} -wallet ${WALLET_TYPE}
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml deploy --account 0 --wallet Multisig2_1
```

**Send EVER transaction**
```bash
cargo run --manifest-path client/Cargo.toml send-transaction \
    --account ${LEDGER_ACCOUNT_NUMBER} \
    --wallet  ${WALLET_TYPE} \
    --amount  ${AMOUNT} \
    --address ${RECIPIENT_ADDRESS}
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml send-transaction \
    --account 0 \
    --wallet Multisig2_1 \
    --amount 0.1 \
    --address 0:7094fc3cb69fa1b7bde8e830e2cd74bc9455d93561ce2c562182215686eb45e2
```

**Get list of supported tokens**
```bash
cargo run --manifest-path client/Cargo.toml get-tokens
```

**Check token balance**
```bash
cargo run --manifest-path client/Cargo.toml get-token-balance --account ${LEDGER_ACCOUNT_NUMBER} --wallet ${WALLET_TYPE} --token ${TOKEN_NAME}
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml get-token-balance --account 0 --wallet EverWallet --token WEVER
```

**Send token transaction**
```bash
cargo run --manifest-path client/Cargo.toml send-token-transaction \
    --account ${LEDGER_ACCOUNT_NUMBER} \
    --wallet  ${WALLET_TYPE} \
    --amount  ${AMOUNT} \
    --address ${RECIPIENT_ADDRESS} \
    --token   ${TOKEN_NAME}
```

```bash
# Example
cargo run --manifest-path client/Cargo.toml send-token-transaction \
    --account 0 \
    --wallet  EverWallet \
    --amount  1.5 \
    --address 0:ed7439e12d67d23fcaf701ff3bd4e30d390c1e8e14f6f40d52089590e28d9c70 \
    --token   WEVER
```

## Tests
Some tests require interactive approval on the ledger
```bash
cargo run --manifest-path tests/Cargo.toml
```

## Documentation
This follows the specification available in the [`api.md`](doc/api.md)
