FROM ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-legacy:latest

RUN apt update && apt install -y libssl-dev

# Patch SDK
RUN cd /opt/ledger-secure-sdk && echo -e 'diff --git a/include/os_seed.h b/include/os_seed.h\n\
index da35f4c5..1f4a224f 100644\n\
--- a/include/os_seed.h\n\
+++ b/include/os_seed.h\n\
@@ -6,6 +6,8 @@\n\
 ''#include "decorators.h"\n\
 ''#include "lcx_ecfp.h"\n\
 ''#include "os_types.h"\n\
+#include "ox_ec.h"\n\
+#include "exceptions.h"\n\
 \n\
 /* ----------------------------------------------------------------------- */\n\
 /* -                             SEED FEATURE                            - */\n' > os_seed-fix.patch  && \
patch -p1 < os_seed-fix.patch
