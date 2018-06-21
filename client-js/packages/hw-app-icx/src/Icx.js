/********************************************************************************
 *   Ledger Node JS API
 *   (c) 2016-2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/
//@flow

// FIXME drop:
import { splitPath, foreach } from "./utils";
import type Transport from "@ledgerhq/hw-transport";

/**
 * ICON API
 *
 * @example
 * import Icx from "@ledgerhq/hw-app-icx";
 * const icx = new Icx(transport)
 */
export default class Icx {
  transport: Transport<*>;

  constructor(transport: Transport<*>) {
    this.transport = transport;
    transport.decorateAppAPIMethods(
      this,
      [
        "getAddress",
        "signTransaction",
        "getAppConfiguration"
      ],
      "ICON"
    );
    transport.setDebugMode(true);
  }

  /**
   * Returns public key and ICON address for a given BIP 32 path.
   * @param path a path in BIP 32 format
   * @option boolDisplay optionally enable or not the display
   * @option boolChaincode optionally enable or not the chaincore request
   * @return an object with a publickey(hexa string), address(string) and 
   *  (optionally) chaincode(hexa string)
   * @example
   * icx.getAddress("44'/4801368'/0'/0'/0").then(o => o.address)
   */
  getAddress(
    path: string,
    boolDisplay?: boolean,
    boolChaincode?: boolean
  ): Promise<{
    publicKey: string,
    address: string,
    chainCode?: string
  }> {
    let paths = splitPath(path);
    let buffer = new Buffer(1 + paths.length * 4);
    buffer[0] = paths.length;
    paths.forEach((element, index) => {
      buffer.writeUInt32BE(element, 1 + 4 * index);
    });
    return this.transport
      .send(
        0xe0,
        0x02,
        boolDisplay ? 0x01 : 0x00, 
        boolChaincode ? 0x01 : 0x00, 
        buffer
      )
      .then(response => {
        let result = {};
        let publicKeyLength = response[0];
        result.publicKey = response.slice(1, 1 + publicKeyLength).toString("hex");
        let addressLength = response[1 + publicKeyLength];
        result.address = response.slice(1 + publicKeyLength + 1, 1 + publicKeyLength + 1 + addressLength);
        if (boolChaincode) {
          result.chainCode = response.slice(-32).toString("hex");
        }
        return result;
      });
  }

  /**
   * Signs a transaction and returns signed message given the raw transaction
   * and the BIP 32 path of the account to sign
   * @param path a path in BIP 32 format
   * @param rawTxAscii raw transaction data to sign in ASCII string format
   * @return an object with a base64 encoded signature and hash in hexa string
   * @example
   * icx.signTransaction("44'/4801368'/0'/0'/0'",
   *     "icx_sendTransaction.fee.0x2386f26fc10000." +
   *     "from.hxc9ecad30b05a0650a337452fce031e0c60eacc3a.nonce.0x3." +
   *     "to.hx4c5101add2caa6a920420cf951f7dd7c7df6ca24.value.0xde0b6b3a7640000")
   *   .then(result => ...)
   */
  signTransaction(
    path: string,
    rawTxAscii: string
  ): Promise<{
    signedRawTxBase64: string,
    hashHex: string
  }> {
    let paths = splitPath(path);
    let offset = 0;
    let rawTx = new Buffer(rawTxAscii);
    let toSend = [];
    let response;
    while (offset !== rawTx.length) {
      let maxChunkSize = offset === 0 ? 150 - 1 - paths.length * 4 - 4: 150;
      let chunkSize =
        offset + maxChunkSize > rawTx.length
          ? rawTx.length - offset
          : maxChunkSize;
      let buffer = new Buffer(
        offset === 0 ? 1 + paths.length * 4 + 4 + chunkSize : chunkSize
      );
      if (offset === 0) {
        buffer[0] = paths.length;
        paths.forEach((element, index) => {
          buffer.writeUInt32BE(element, 1 + 4 * index);
        });
        buffer.writeUInt32BE(rawTx.length, 1 + 4 * paths.length);
        rawTx.copy(buffer, 1 + 4 * paths.length + 4, offset, offset + chunkSize);
      } else {
        rawTx.copy(buffer, 0, offset, offset + chunkSize);
      }
      toSend.push(buffer);
      offset += chunkSize;
    }
    return foreach(toSend, (data, i) =>
      this.transport
        .send(0xe0, 0x04, i === 0 ? 0x00 : 0x80, 0x00, data)
        .then(apduResponse => {
          response = apduResponse;
        })
    ).then(() => {
      let result = {};
      // r, s, v are aligned sequencially
      result.signedRawTxBase64 = 
        btoa(response.slice(0, 32 + 32 + 1).toString("hex"));
      result.hashHex = 
        response.slice(32 + 32 + 1, 32 + 32 + 1 + 32).toString("hex");
      return result;
    });
  }

  /**
   * Returns the application configurations such as versions.
   * @return  major/minor/patch versions of Icon application
   */
  getAppConfiguration(): Promise<{
    majorVersion: number,
    minorVersion: number,
    patchVersion: number
  }> {
    console.log("getAppConfiguration()");
    return this.transport
      .send(0xe0, 0x06, 0x00, 0x00)
      .then(response => {
    console.log("returning getAppConfiguration()");
    console.log("returning getAppConfiguration()" + response[0]);
        let result = {};
        result.majorVersion = response[0];
        result.minorVersion = response[1];
        result.patchVersion = response[2];
        return result;
      });
  }
}
