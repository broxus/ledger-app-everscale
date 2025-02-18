#pragma once

#include <stdbool.h>  // bool

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

void ui_display_address();
void ui_display_public_key();
void ui_display_sign_transaction(int flow);
void ui_display_sign();
