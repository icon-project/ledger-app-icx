import "./index.css";
import React, { Component } from "react";
import ReactDOM from "react-dom";
import TransportU2F from "@ledgerhq/hw-transport-u2f";
import Icx from "@ledgerhq/hw-app-icx";
import Btc from "@ledgerhq/hw-app-btc";

class App extends Component {
  state = {
    result: null,
    error: null
  };

  clear = () => {
    this.setState({ result: null });
    this.setState({ error: null });
  };
  showProcessing = () => {
    this.setState({ error: null });
    this.setState({ result: "processing..." });
  };
  showTestResult = (failMessage: string) => {
    if (failMessage.length > 0) {
      this.setState({ error: "FAIL: " + failMessage });
    }
    else {
      this.setState({ result: "PASS" });
    }
  }

  hexToBase64 = (hexString: string) => {
    return btoa(hexString.match(/\w{2}/g).map(function(a) {
      return String.fromCharCode(parseInt(a, 16));
    }).join("")); 
  }

  onBtcGetAddress = async () => {
    try {
      this.showProcessing();
      const transport = await TransportU2F.create();
      transport.setDebugMode(true);
      const btc = new Btc(transport);
      const { bitcoinAddress } = await btc.getWalletPublicKey("44'/0'/0'/0");
      this.setState({ result: bitcoinAddress });
    } catch (error) {
      this.setState({ error });
    }
  };

  createIcx = async (timeout?: number = 30000): Icx => {
    const transport = await TransportU2F.create();
    transport.setDebugMode(true);
    transport.setExchangeTimeout(timeout);
    return new Icx(transport);
  };

  onGetAddress = async () => {
    try {
      this.showProcessing();
      const icx = await this.createIcx();
      const { publicKey, address, chainCode } = await icx.getAddress("44'/4801368'/0'");
      const resultText = "[publicKey=" + publicKey + "],[address=" + address + "],[chainCode=" + chainCode + "]";
      this.setState({ result: resultText });
    } catch (error) {
      this.setState({ error });
    }
  };
  onSignTransaction = async () => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      const path = "44'/4801368'/0'";
      const rawTx = 
        "icx_sendTransaction.fee.0x2386f26fc10000." +
        "from.hxc9ecad30b05a0650a337452fce031e0c60eacc3a.nonce.0x3." +
        "to.hx4c5101add2caa6a920420cf951f7dd7c7df6ca24.value.0xde0b6b3a7640000";
      const { signedRawTxBase64, hashHex } = 
        await icx.signTransaction(path, rawTx);
      const resultText = "[signature=" + signedRawTxBase64 + "],[hash=" + hashHex + "]";
      this.setState({ result: resultText });
    } catch (error) {
      this.setState({ error });
    }
  };
  onSignTransactionV3 = async () => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      const path = "44'/4801368'/0'";
      const rawTx =
        "icx_sendTransaction." +
        "from.hxc9ecad30b05a0650a337452fce031e0c60eacc3a.nonce.0x3." +
        "stepLimit.0xff." +
        "to.hx4c5101add2caa6a920420cf951f7dd7c7df6ca24.value.0xde0b6b3a7640000." +
        "version.0x3";
      const { signedRawTxBase64, hashHex } =
        await icx.signTransaction(path, rawTx);
      const resultText = "[signature=" + signedRawTxBase64 + "],[hash=" + hashHex + "]";
      this.setState({ result: resultText });
    } catch (error) {
      this.setState({ error });
    }
  };
  onGetAppConfiguration = async () => {
    try {
      this.showProcessing();
      const icx = await this.createIcx();
      const {
        majorVersion,
        minorVersion,
        patchVersion
      } = await icx.getAppConfiguration();
      const resultText =
        "[majorVer=" + majorVersion + "],[minorVer=" + minorVersion +
        "],[patchVer=" + patchVersion + "]";
      this.setState({ result: resultText });
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestGetAddress = async() => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      await icx.setTestPrivateKey("23498dc21b9ee52e63e8d6566e0911ac255a38d3fcbc68a51e6b298520b72d6e");
      const { publicKey, address, chainCode } = await icx.getAddress("0'", true, false);

      let isSamePublicKey = false;
      let isSameAddress = false;
      const answerPublicKey = "0498cd4a92d7d491e3552d6abafe96e4add9f4f068bf385325e0dab902baad1602a150ea36b36974fcc7535f24e6715270277848c8421deb5586f611bdd7551f72";
      const answerAddress = "hxfa1602a01d0ca2c2256f1a508be6498df801a5b2";
      if (publicKey == answerPublicKey) {
        isSamePublicKey = true;
      }
      if (address == answerAddress) {
        isSameAddress = true;
      }
      let failMessage = "";
      if (!isSamePublicKey) {
        failMessage += "wrong public key ('" + publicKey + "' should be '" + answerPublicKey + "'.)" ;
      }
      else if (!isSameAddress) {
        failMessage += "wrong address ('" + address + "' should be '" + answerAddress + "'.)" ;
      }
      this.showTestResult(failMessage);
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransaction = async() => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      await icx.setTestPrivateKey("c8e2edf81129f07720ed5ae36311316fb84ac6f7ddbc0f175f9df5848ad431ed");
      const rawTx =
        "icx_sendTransaction.fee.0x2386f26fc10000" + 
        ".from.hx36f93789103fa3b5da3d40c13d08c2ca2457ca86.timestamp.1530259517236639" + 
        ".to.hx30d027ff0b2e52645861d85efb04896127ec166e.value.0xde0b6b3a7640000";
      const { signedRawTxBase64, hashHex } = await icx.signTransaction("0'", rawTx);

      let isSameSignature = false;
      let isSameHash = false;
      const answerSignature = this.hexToBase64("efc780adffc77b4fad0a18a3df5203128e69494e3abfc5f113e6d16b32ab0fd9204e97cdbcd61cd70997138ae74312b5b04945e38d67cfc2a9352af28b3599f300");
      const answerHash = "72f26c66527755484d4a9877457b7092827cad9ece8685ce392ebeccf17babad";
      if (signedRawTxBase64 == answerSignature) {
        isSameSignature = true;
      }
      if (hashHex == answerHash) {
        isSameHash = true;
      }
      let failMessage = "";
      if (!isSameSignature) {
        failMessage += "wrong signature ('" + signedRawTxBase64 + "' should be '" + answerSignature + "'.)" ;
      }
      else if (!isSameHash) {
        failMessage += "wrong hash ('" + hashHex + "' should be '" + answerHash + "'.)" ;
      }
      this.showTestResult(failMessage);
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransactionLongData = async() => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      await icx.setTestPrivateKey("e0b18a065bb0bd663e71aff34d1bd44dbf746113968533058440eca75b061c9e");
      // I don't know it's a right JSON format of transaction data
      let rawTx = "icx_sendTransaction.amount.ox1234.content.0x";
      for (let i = 0; i < 200; i++) {
        rawTx += "1867291283973610982301923812873419826abcdef91827319263187263a732" // 64 
      }
      rawTx += ".contentType.application/zip.dataType.deploy." +
        "fee.0x2386f26fc10000.from.hx6ad6f9a5adada3e484861b52378990bd6f473450." +
        "stepLimit.0x14.timestamp.1529897548414400." +
        "to.hx609c1c454528bae228514ceccec0c0939637a3fb.value.0xde0b6b3a7640000";
      const { signedRawTxBase64, hashHex } = await icx.signTransaction("0'", rawTx);

      let isSameSignature = false;
      let isSameHash = false;
      const answerSignature = this.hexToBase64("08ed94735506cddba0fe935bc1246b7408915cd1eaf7fc9203fea754a0bbdae562e5c3960195d226abcb0e272ead200cf5fdebd0583dc715207db8818cea386000");
      const answerHash = "0651e37662eab417666e8dd664b6f62a8c42e9cf9c8eaababb6727cc203d0e18";
      if (signedRawTxBase64 == answerSignature) {
        isSameSignature = true;
      }
      if (hashHex == answerHash) {
        isSameHash = true;
      }
      let failMessage = "";
      if (!isSameSignature) {
        failMessage += "wrong signature ('" + signedRawTxBase64 + "' should be '" + answerSignature + "'.)" ;
      }
      else if (!isSameHash) {
        failMessage += "wrong hash ('" + hashHex + "' should be '" + answerHash + "'.)" ;
      }
      this.showTestResult(failMessage);
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransactionMultiLevelParams= async () => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      const path = "44'/4801368'/0'";
      const rawTx =
        "icx_sendTransaction.data." + 
        "{method.transfer.params." +
        "{_to.hx5be671cf246b6eb9438914494cbab57e2641d37a._value.0xde0b6b3a7640000}}." +
        "dataType.call.from.hx772d8fa231eef810b9e39270963f97dacc69f2a5.nid.0x3." +
        "stepLimit.0x2b750.timestamp.0x573e910b5cb48." +
        "to.cxe382845b0fa8d00d748f7b29c9ee7369eee20897.value.0x0.version.0x3";
      const { signedRawTxBase64, hashHex } =
        await icx.signTransaction(path, rawTx);
      this.showTestResult("");
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransactionNoValue= async () => {
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      const path = "44'/4801368'/0'";
      const rawTx =
        "icx_sendTransaction.data.{method.transfer.params.{_to.hxb2d0d07dcdd887a8b60e3069d87e91a312947adc._value.0x1bc16d674ec80000}}.dataType.call.from.hx772d8fa231eef810b9e39270963f97dacc69f2a5.nid.0x1.stepLimit.0x30d40.timestamp.0x5815c9daa8a18.to.cxc248ee72f58f7ec0e9a382379d67399f45b596c7.version.0x3"
      const { signedRawTxBase64, hashHex } =
        await icx.signTransaction(path, rawTx);
      this.showTestResult("");
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransactionBigValue= async () => {
    // Current display buffer is 47, so test value should be within int256 range
    // in loop and longer than 47 decimal numbers in ICX.
    try {
      this.showProcessing();
      const icx = await this.createIcx(90000);
      const path = "44'/4801368'/0'";
      const rawTx =
        "icx_sendTransaction.data." + 
        "{method.transfer.params." +
        "{_to.hx5be671cf246b6eb9438914494cbab57e2641d37a._value.0xde0b6b3a7640000}}." +
        "dataType.call.from.hx772d8fa231eef810b9e39270963f97dacc69f2a5.nid.0x3." +
        "stepLimit.0x2b750.timestamp.0x573e910b5cb48." +
        "to.cxe382845b0fa8d00d748f7b29c9ee7369eee20897.value.0xde0b6b3a76400000000000000000000000000000.version.0x3";
      const { signedRawTxBase64, hashHex } =
        await icx.signTransaction(path, rawTx);
      this.showTestResult("It should occur an error of code 0x6a80.");
    } catch (error) {
      if (error.statusCode == 0x6a80) {
        this.showTestResult("");
      } else {
        this.setState({ error });
      }
    }
  };
  onClear = async() => {
    this.clear();
  };

  render() {
    const { result, error } = this.state;
    return (
      <div>
        <ul>
          <li>simple function call
            <ul>
              <li>
                <button className="action" onClick={this.onGetAddress}>
                  getAddress()
                </button>
              </li>
              <li>
                <button className="action" onClick={this.onSignTransaction}>
                  signTransaction()
                </button>
                <ul>
                <li>check tx info on Ledger display and confirm. amount=1, fee=0x01</li>
                </ul>
              </li>
              <li>
                <button className="action" onClick={this.onSignTransactionV3}>
                  signTransactionV3()
                </button>
                <ul>
                  <li>check tx info on Ledger display and confirm. amount=1, stepLimit=255</li>
                </ul>
              </li>
              <li>
                <button className="action" onClick={this.onGetAppConfiguration}>
                  getAppConfiguration()
                </button>
              </li>
            </ul>
          </li>
        </ul>
        <ul>
          <li>ledger app test cases
            <ul>
              <li>
                <button className="action" onClick={this.onTestGetAddress}>
                  getAddress(): verify public key and address
                </button>
                <ul>
                  <li>check if the address on display is 'hxfa1602a01d0ca2c2256f1a508be6498df801a5b2' and confirm</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransaction}>
                  signTransaction(): verify signature and hash
                </button>
                <ul>
                  <li>check tx info on Ledger display and confirm</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransactionLongData}>
                  signTransaction(): verify a long data (>2K)
                </button>
                <ul>
                  <li>check tx info on Ledger display and confirm</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransactionMultiLevelParams}>
                  signTransaction(): multi-level params
                </button>
                <ul>
                  <li>check tx info on Ledger display and confirm</li>
                  <li>NOTE: It doesn't check the signature validity</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransactionNoValue}>
                  signTransaction(): display test with tx of no amount
                </button>
                <ul>
                  <li>check if tx info on Ledger display doesn't have wrong amount such as "ICX StepLimit" and confirm</li>
                  <li>NOTE: It doesn't check the signature validity</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransactionBigValue}>
                  signTransaction(): error test with a big amount out of display buffer
                </button>
              </li>
            </ul>
          </li>
        </ul>
        <ul>
          <li>reference function
            <ul>
              <li>
                <button className="action" onClick={this.onBtcGetAddress}>
                  reference Btc.getAddress()
                </button>
                <ul>
                  <li>NOTE: make sure BTC app is running</li>
                </ul>
              </li>
            </ul>
          </li>
        </ul>
        <p>
          RESULT: <button className="action" onClick={this.onClear}>clear</button>
          <p>
          {error ? (
            <code className="error">{error.toString()}</code>
          ) : (
            <code className="result">{result}</code>
          )}
          </p>
        </p>
      </div>
    );
  }
}

ReactDOM.render(<App />, document.getElementById("root"));
