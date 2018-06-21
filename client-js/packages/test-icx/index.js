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
      const transport = await TransportU2F.create();
      transport.setDebugMode(true);
      const icx = new Icx(transport);
      const { publicKey, address, chainCode } = await icx.getAddress("44'/4801368'/0'/0'/0'", true, true);
      const resultText = "[publicKey=" + publicKey + "],[address=" + address + "],[chainCode=" + chainCode + "]";
      this.setState({ result: resultText });
    } catch (error) {
      this.setState({ error });
    }
  };
  onSignTransaction = async () => {
    try {
      this.setState({ error: null });
      const transport = await TransportU2F.create();
      const icx = new Icx(transport);
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
      const transport = await TransportU2F.create();
      const icx = new Icx(transport);
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
        <p>
          <button onClick={this.onClear}>
            clearssssssss result
          </button>
        </p>
        <p>
          <button onClick={this.onBtcGetAddress}>
            reference Btc.getAddress()
          </button>
        </p>
        <p>
          <button onClick={this.onGetAddress}>
            test getAddress()
          </button>
        </p>
        <p>
          <button onClick={this.onSignTransaction}>
            test signTransaction()
          </button>
        </p>
        <p>
          <button onClick={this.onGetAppConfiguration}>
            test getAppConfiguration()
          </button>
        </p>
        <p>
          result:
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
