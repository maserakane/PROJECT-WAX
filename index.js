import { SessionKit } from "@wharfkit/session";
import { WebRenderer } from "@wharfkit/web-renderer";
import { WalletPluginCloudWallet } from "@wharfkit/wallet-plugin-cloudwallet";

// Create an instance of WebRenderer
const webRenderer = new WebRenderer();

// Create an instance of SessionKit
const sessionKit = new SessionKit({
    appName: "appname",
    chains: [
        {
            id:
                "73e4385a2708e6d7048834fbc1079f2fabb17b3c125b146af438971e90716c4d",
            url: "https://jungle4.greymass.com",
        },
    ],
    ui: webRenderer,
    walletPlugins: [
        new WalletPluginCloudWallet({
            supportedChains: [
                // Specify supported chains here
                "1064487b3cd1a897ce03ae5b6a865651747e2e152090f99c1d19d44e01aea5a4", // WAX (Mainnet)
            ],
            url: "https://www.mycloudwallet.com",
            autoUrl: "https://idm-api.mycloudwallet.com/v1/accounts/auto-accept",
            loginTimeout: 300000, // 5 minutes
        }),
    ],
});

// Export 'sessionKit' if needed for other parts of your application
console.log(sessionKit)
export default sessionKit;
