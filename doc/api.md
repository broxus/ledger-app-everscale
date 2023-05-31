# Everscale application : Common Technical Specifications

## About

This application describes the APDU messages interface to communicate with the Everscale application.

The application covers the following functionalities :

- Retrieve a public key given an account number
- Retrieve an address given an account number
- Sign Everscale message hash
- Sign Everscale BOC transaction

The application interface can be accessed over HID or BLE

## General purpose APDUs

### GET APP CONFIGURATION

#### Description

_This command returns specific application configuration_

##### Command

| _CLA_ | _INS_ | _P1_ | _P2_ | _Lc_ | _Le_ |
| ----- |:-----:| ---: | ---- |:----:|-----:|
| E0    |  01   |   00 | 00   |  00  |   04 |

##### Input data

_None_

##### Output data

| _Description_             | _Length_ |
| ------------------------- | :------: |
| Application major version |    01    |
| Application minor version |    01    |
| Application patch version |    01    |

### GET PUBKEY

#### Description

_This command returns a public key for the given account number_

##### Command

| _CLA_ | _INS_ | _P1_                                                                             | _P2_ |   _Lc_   |     _Le_ |
| ----- |:-----:|:---------------------------------------------------------------------------------|------| :------: | -------: |
| E0    |  02   | 00 : return public key<br/>01 : display public key and confirm before returning  | 00   | variable | variable |

##### Input data

| _Description_                       | _Length_ |
|-------------------------------------|:--------:|
| An account number to retrieve       |    4     |


##### Output data

| _Description_ | _Length_ |
| ------------- |:--------:|
| Pubkey length |    1     |
| Pubkey        |    32    |

### SIGN MESSAGE

#### Description

_This command signs a message_

##### Command

| _CLA_ | _INS_ | _P1_ | _P2_ |   _Lc_   |     _Le_ |
| ----- |:-----:| ---: | ---- | :------: | -------: |
| E0    |  03   |   01 | 00   | variable | variable |

##### Input data

| _Description_                            | _Length_ |
|------------------------------------------|:--------:|
| An account number to retrieve            |    4     |
| Metadata                                 |    1     |
| Chain ID (Optional: metadata b'00001000) |    4     |
| A bytes to sign                          |    32    |

##### Output data

| _Description_    | _Length_ |
|------------------| :------: |
| Signature length |    1     |
| Signature        |    64    |

### GET ADDRESS

#### Description

_This command returns an address for the given account number_

##### Command

| _CLA_ | _INS_ | _P1_                                                                      | _P2_ |   _Lc_   |     _Le_ |
| ----- |:-----:|:--------------------------------------------------------------------------|------| :------: | -------: |
| E0    |  04   | 00 : return address<br/>01 : display address and confirm before returning | 00   | variable | variable |

##### Input data

| _Description_                 | _Length_ |
|-------------------------------|:--------:|
| An account number to retrieve |    4     |
| Wallet number to retrieve     |    1     |

##### Output data

| _Description_  | _Length_ |
|----------------|:--------:|
| Address length |    1     |
| Address        |    32    |

### SIGN TRANSACTION

#### Description

_This command signs a transaction message_

##### Command

| _CLA_ | _INS_ | _P1_ | _P2_ |   _Lc_   |     _Le_ |
| ----- |:-----:| ---: | ---- | :------: | -------: |
| E0    |  05   |   01 | 00   | variable | variable |

##### Input data

| _Description_                                                                  | _Length_ |
|--------------------------------------------------------------------------------|:--------:|
| An account number to retrieve                                                  |    4     |
| Original wallet number to derive address                                       |    1     |
| Decimals                                                                       |    1     |
| Ticker length                                                                  |    1     |
| Ticker                                                                         | variable |
| Metadata                                                                       |    1     |
| Current wallet number to parse transaction abi (Optional: metadata b'00000001) |    1     |
| Workchain ID (Optional: metadata b'00000010)                                   |    1     |
| Deploy contract address (Optional: metadata b'00000100)                        |    32    |
| Chain ID (Optional: metadata b'00001000)                                       |    4     |
| Serialized transaction                                                         | variable |

##### Output data

| _Description_   | _Length_ |
|-----------------| :------: |
| Address length  |    1     |
| Signature       |    64    |

## Transport protocol

### General transport description

_Ledger APDUs requests and responses are encapsulated using a flexible protocol allowing to fragment large payloads over different underlying transport mechanisms._

The common transport header is defined as follows:

| _Description_                         | _Length_ |
| ------------------------------------- | :------: |
| Communication channel ID (big endian) |    2     |
| Command tag                           |    1     |
| Packet sequence index (big endian)    |    2     |
| Payload                               |   var    |

The Communication channel ID allows commands multiplexing over the same physical link. It is not used for the time being, and should be set to 0101 to avoid compatibility issues with implementations ignoring a leading 00 byte.

The Command tag describes the message content. Use TAG_APDU (0x05) for standard APDU payloads, or TAG_PING (0x02) for a simple link test.

The Packet sequence index describes the current sequence for fragmented payloads. The first fragment index is 0x00.

### APDU Command payload encoding

APDU Command payloads are encoded as follows :

| _Description_            | _Length_ |
| ------------------------ | :------: |
| APDU length (big endian) |    2     |
| APDU CLA                 |    1     |
| APDU INS                 |    1     |
| APDU P1                  |    1     |
| APDU P2                  |    1     |
| APDU data length         |    1     |
| Optional APDU data       |   var    |

APDU payload is encoded according to the APDU case

| Case Number | _Lc_ | _Le_ | Case description                                        |
| ----------- | ---- | ---- | ------------------------------------------------------- |
| 1           | 0    | 0    | No data in either direction - L is set to 00            |
| 2           | 0    | !0   | Input Data present, no Output Data - L is set to Lc     |
| 3           | !0   | 0    | Output Data present, no Input Data - L is set to Le     |
| 4           | !0   | !0   | Both Input and Output Data are present - L is set to Lc |

### APDU Response payload encoding

APDU Response payloads are encoded as follows :

| _Description_                      | _Length_ |
| ---------------------------------- | :------: |
| APDU response length (big endian)  |    2     |
| APDU response data and Status Word |   var    |

### USB mapping

Messages are exchanged with the dongle over HID endpoints over interrupt transfers, with each chunk being 64 bytes long. The HID Report ID is ignored.

### BLE mapping

A similar encoding is used over BLE, without the Communication channel ID.

The application acts as a GATT server defining service UUID D973F2E0-B19E-11E2-9E96-0800200C9A66

When using this service, the client sends requests to the characteristic D973F2E2-B19E-11E2-9E96-0800200C9A66, and gets notified on the characteristic D973F2E1-B19E-11E2-9E96-0800200C9A66 after registering for it.

Requests are encoded using the standard BLE 20 bytes MTU size

## Status Words

The following standard Status Words are returned for all APDUs - some specific Status Words can be used for specific commands and are mentioned in the command description.

##### Status Words

| _SW_ |                   _Description_                    |
| ---- |:--------------------------------------------------:|
| 6700 |                  Incorrect length                  |
| 6982 |  Security status not satisfied (Canceled by user)  |
| 6B0x |                  Invalid request                   |
| 6Fxx | Technical problem (Internal error, please report)  |
| 9000 |            Normal ending of the command            |