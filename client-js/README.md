It is the javascript version of client library to communicatie with Ledger Nano /
Nano S / Blue applications.

# Installation Guide

## Prerequisite

### Build Environment
* yarn

### Test Environment
* Ledger device with ICON application installed

## Getting Started

### Preparation
1. Connect Ledger Nano S device on a USB port
1. Make sure Ledger Nano S is properly set up and ICON application is installed 
and started

### Build and test
1. Simply install dependencies, build, and start a simple web server with simple
test-purpose ICON web app
    ```
    $ yarn
    $ yarn test
    ```

1. Access "https://[IP]:9966" on web browser and test

    > It uses chrome built-in extension for U2F so use Chrome web browser

    > Allow web browser to access site of untrusted certificate 
which a simple web server BUDO uses. U2F communication is only allowed for HTTPS

### Build scripts

#### Install dependencies
```bash
yarn
```

#### Build
Build all packages
```bash
yarn build
```

#### Watch
Watch all packages change. Very useful during development to build only file that changes.
```bash
yarn watch
```

#### Lint
Lint all packages
```bash
yarn lint
```

#### Run tests
Build and make it ready to test at once.
```
yarn test
```

# API 
Refer to API document

# Examples
```js
import Transport from "@ledgerhq/hw-transport-u2f";
import AppBtc from "@ledgerhq/hw-app-icx";

const getIcxAddress = async () => {
  const transport = await TransportU2F.create();
  const icx = new Icx(transport);
  const result = await icx.getAddress("44'/4801368'/0'/0'/0'", true, true);
  return result.address;
};
getIcxAddress().then(a => console.log(a));
```

# TODO

* Adds ICON Ledger app test cases
* Enhance ICON Ledger app sample web page
