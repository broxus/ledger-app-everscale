FROM ubuntu:22.04

RUN apt update
RUN apt install -y git cmake clang libssl-dev

# Clone the repository.
# WARNING: use the latest version of the app and integrate the fuzzer
RUN git clone https://github.com/qbLedger/ledger-app-everscale.git
RUN cd ledger-app-everscale && git checkout pr_ready

# Get the sdk
RUN git clone https://github.com/LedgerHQ/nanos-secure-sdk.git

# Patch the sdk
RUN echo -e 'diff --git a/include/os_seed.h b/include/os_seed.h\n\
index 63b12ee..633346b 100644\n\
--- a/include/os_seed.h\n\
+++ b/include/os_seed.h\n\
@@ -4,6 +4,8 @@\n\
 ''#include "decorators.h"\n\
 ''#include "lcx_ecfp.h"\n\
 ''#include "os_types.h"\n\
+#include "ox_ec.h"\n\
+#include "exceptions.h"\n\
 \n\
 /* ----------------------------------------------------------------------- */\n\
 /* -                             SEED FEATURE                            - */\n' > os_seed-fix.patch
RUN cat /os_seed-fix.patch
RUN patch -p1 -i /os_seed-fix.patch -d /nanos-secure-sdk/

# Set the BOLOS_SDK env var
ENV BOLOS_SDK /nanos-secure-sdk/

# Build the fuzzer
RUN cd ledger-app-everscale/fuzz && ./build.sh

# Run the fuzzer. Here you can specify a pre-existing corpus
#RUN cd ledger-app-everscale/fuzz/cmake-build-fuzz && mkdir corpus && ./fuzzer corpus/
