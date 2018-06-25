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
  onBtcGetAddress = async () => {
    try {
      this.setState({ error: null });
      const transport = await TransportU2F.create();
      transport.setDebugMode(true);
      const btc = new Btc(transport);
      const { bitcoinAddress } = await btc.getWalletPublicKey("44'/0'/0'/0'");
      this.setState({ result: bitcoinAddress });
    } catch (error) {
      this.setState({ error });
    }
  };

  onGetAddress = async () => {
    try {
      this.setState({ error: null });
      const icx = new Icx(await TransportU2F.create());
      const { publicKey, address, chainCode } = await icx.getAddress("44'/4801368'/0'/0'/0'", false, true);
      const resultText = "[publicKey=" + publicKey + "],[address=" + address + "],[chainCode=" + chainCode + "]";
      this.setState({ result: resultText });
    } catch (error) {
      this.setState({ error });
    }
  };
  onSignTransaction = async () => {
    try {
      this.setState({ error: null });
      const icx = new Icx(await TransportU2F.create());
      const path = "44'/4801368'/0'/0'/0'";
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
  onGetAppConfiguration = async () => {
    try {
      this.setState({ error: null });
      const icx = new Icx(await TransportU2F.create());
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
      this.setState({ error: null });
      const icx = new Icx(await TransportU2F.create());
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
      if (failMessage.length > 0) {
        this.setState({ error: "FAIL: " + failMessage });
      }
      else {
        this.setState({ result: "PASS" });
      }
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransaction = async() => {
    try {
      this.setState({ error: null });
      const icx = new Icx(await TransportU2F.create());
      await icx.setTestPrivateKey("ac7d849f0f232c3b01818d43b0323e31d4aec933aeb7681595215ea98ab70c20");
      const rawTx =
        "icx_sendTransaction.free.0x2386f26fc10000." +
        "from.hx5f1238f18eb87fc23a9d5fb1bb89250889108f24.timestamp.1529629661544678." +
        "to.hx3690276e8703d6d96b54d171bfd265dd8d60c016.value.0xde0b6b3a7640000";
      const { signedRawTxBase64, hashHex } = await icx.signTransaction("0'", rawTx);

      let isSameSignature = false;
      let isSameHash = false;
      const answerSignature = btoa("e888e700d8b49eee479d16a6d5374b56ab6243de38751e4fc3f779f21a48c5d45e89e351fc1a6e7536ecf1e35f15b8309ec6fefc4356d66fd16c2845c838302a01");
      const answerHash = "3c296ae34bf73c32fdfb208f0b2b185daf33928e65141caed3aae8d75cade5f6";
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
      if (failMessage.length > 0) {
        this.setState({ error: "FAIL: " + failMessage });
      }
      else {
        this.setState({ result: resultText });
      }
    } catch (error) {
      this.setState({ error });
    }
  };
  onTestSignTransactionLongData = async() => {
    try {
      this.setState({ error: null });
      const icx = new Icx(await TransportU2F.create());
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
      const answerSignature = btoa("08ed94735506cddba0fe935bc1246b7408915cd1eaf7fc9203fea754a0bbdae562e5c3960195d226abcb0e272ead200cf5fdebd0583dc715207db8818cea386000");
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
      if (failMessage.length > 0) {
        this.setState({ error: "FAIL: " + failMessage });
      }
      else {
        this.setState({ result: resultText });
      }
    } catch (error) {
      this.setState({ error });
    }
  };
  // TODO: Added clear button because I don't know how to change result text
  // to "waiting..." before the result comes out.
  // TODO: Couldn't find how to show multi line text for the result. Now it
  // just display it a sinlge long line.
  onClear = async() => {
    this.setState({ error: null });
    this.setState({ result: null });
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
                  <li>check tx info on Ledger display and confirm</li>
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
                  verify public key and address from getAddress()
                </button>
                <ul>
                  <li>check if the address on display is 'hxfa1602a01d0ca2c2256f1a508be6498df801a5b2' and confirm</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransaction}>
                  verify signature and hash from signTransaction()
                </button>
                <ul>
                  <li>check tx info on Ledger display and confirm</li>
                </ul>
              </li>
            </ul>
            <ul>
              <li>
                <button className="action" onClick={this.onTestSignTransactionLongData}>
                  verify a long data (>2K) from signTransaction()
                </button>
                <ul>
                  <li>check tx info on Ledger display and confirm</li>
                </ul>
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
