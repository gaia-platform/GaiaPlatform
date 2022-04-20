const path = require("path");

module.exports = {
  entry: {
    dataViewer: "./ext-src/data_view/app/index.tsx"
  },
  output: {
    path: path.resolve(__dirname, "dataViewer"),
    filename: "[name].js"
  },
  devtool: "cheap-module-source-map",
  externals: {
    vscode: 'commonjs vscode'
  },
  resolve: {
    extensions: [".js", ".ts", ".tsx", ".json", ".css"]
  },
  module: {
    rules: [
      {
        test: /\.(ts|tsx)$/,
        loader: "ts-loader",
        options: {}
      },
      {
        test: /\.css$/,
        use: [
          {
            loader: "style-loader"
          },
          {
            loader: "css-loader"
          }
        ]
      },
      {
        test: /\.svg$/i,
        issuer: /\.[jt]sx?$/,
        use: ['@svgr/webpack'],
      },
    ]
  },
  performance: {
    hints: false
  }
};