# Fuzzing on transaction parser

## Compilation

* Build docker image

```
pushd fuzzing && docker build --network=host -t ledger-app-fuzzer . && popd
```

* Run docker container

```
docker run --rm -ti --user "$(id -u):$(id -g)" -v "$(realpath .):/app:z" ledger-app-fuzzer:latest

```

* Build fuzzer inside the container

```
cd /app/fuzzing && cmake -DBOLOS_SDK=/opt/ledger-secure-sdk -DTARGET_DEVICE=nanos -DCMAKE_C_COMPILER=/usr/bin/clang -Bbuild -H.
```

then

```
make -C build
```

## Run

Without seed corpus (slow — libFuzzer starts from random bytes):
```
./build/fuzzer
```
