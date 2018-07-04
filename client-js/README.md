It is the javascript version of client library to communicatie with Ledger Nano /
Nano S / Blue applications.

# Installation Guide

## Prerequisite

### Build Environment
* yarn
* Node.js

### Test Environment
* Ledger device with ICON application installed

## Getting Started

### Preparation
1. Connect Ledger Nano S device on a USB port
1. Make sure Ledger Nano S set up and ICON application installed and started

### Build and test
1. Simply install dependencies, build, and start a simple web server with simple
test-purpose ICON web app
    ```
    $ cd client-js
    $ yarn
    $ yarn build
    $ yarn test
    ```

1. Access "https://[IP]:9966" on web browser and test

    > It uses chrome built-in extension for U2F so use Chrome web browser

    > Allow web browser to access site of untrusted certificate 
which a simple web server BUDO uses. U2F communication is only allowed on HTTPS

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
Launch simple web server with ICON test web app
```
yarn test
```

#### Clean 
Clean all build output
```
yarn clean
```

#### Documentation 
Generate API documents at ./packages/documentation-website
```
yarn documentation
```

# API 
Refer to API document under ./packages/documentation-website/public/docs/index.html

# Examples
```js
import Transport from "@ledgerhq/hw-transport-u2f";
import Icx from "@ledgerhq/hw-app-icx";

const getIcxAddress = async () => {
  const transport = await TransportU2F.create();
  transport.setDebugMode(true);         // if you want to print log
  transport.setExchangeTimeout(60000);  // Set if you want to change U2F timeout. default: 30 sec

  const icx = new Icx(transport);
  const result = await icx.getAddress("44'/4801368'/0'/0'/0'", true, true);
  return result.address;
};
getIcxAddress().then(a => console.log(a));
```

# Remarks

## Integration with Chrome Extension
JS library uses chrome pre-built U2F extension "CryptTokenExtension" which allows
https web page as the trusted origin, and chrome extension can't use
directly U2F extension. Therefore chrome extension needs to create an iframe 
which pulls https web page integrating with this library and communicate
through Message Channel. 

Refer to https://bugs.chromium.org/p/chromium/issues/detail?id=823736
for the reason U2F extension doesn't allow chrome extension origin.

# TODO

* Show API documents directly from this page
