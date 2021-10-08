#!/usr/bin/env node

// Disables code splitting into chunks.
// Creates build directory with bundled javscript files for webview.

const rewire = require("rewire");
const defaults = rewire("react-scripts/scripts/build.js");
let config = defaults.__get__("config");

config.optimization.splitChunks = {
  cacheGroups: {
    default: false
  }
};

config.optimization.runtimeChunk = false;
