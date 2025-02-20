#pragma once

#include <stdbool.h>  // bool

/**
 * Action for public key validation and export.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void validate_pubkey(bool choice);

/**
 * Action for address validation and export.
 *
 * @param[in] choice
 *   User choice (either approved or rejectd).
 *
 */
void validate_address(bool choice);
/**
 * Action for transaction information validation.
 *
 * @param[in] choice
 *   User choice (either approved or rejectd).
 *
 */
void validate_transaction(bool choice);

/**
 * Action for sign validation.
 *
 * @param[in] choice
 */
void validate_message(bool choice);