# Fuzzing on transaction parser

## Compilation

* Build docker image

```
pushd fuzzing && docker build -t ledger-app-fuzzer . && popd
```

* Run docker container

```
docker run --rm -ti --user "$(id -u):$(id -g)" -v "$(realpath .):/app" ledger-app-fuzzer:latest

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

```
./build/fuzzer
```
